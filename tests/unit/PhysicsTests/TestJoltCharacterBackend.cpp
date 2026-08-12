#include <gtest/gtest.h>

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <cmath>
#include <limits>

namespace
{
    [[nodiscard]] physics::PhysicsCharacterDescriptor CharacterDescriptor()
    {
        return {
            .collider = { .radius = 0.5f, .cylinderHeight = 1.0f },
            .position = { 3.0f, 5.0f, -2.0f },
            .maximumSlopeAngleDegrees = 45.0f,
            .maximumStepHeight = 0.3f,
            .mass = 80.0f,
            .maximumSpeed = 6.0f
        };
    }

    [[nodiscard]] physics::PhysicsBodyDescriptor FloorDescriptor()
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 5.0f, 0.5f, 5.0f } },
            .transform = { .position = { 0.0f, -0.5f, 0.0f } },
            .motionType = physics::PhysicsMotionType::Static
        };
    }
    
    [[nodiscard]] physics::PhysicsBodyDescriptor StaticBox(
        const mathUtils::Vec3& position,
        const mathUtils::Vec3& halfExtents,
        const float rotationDegrees = 0.0f)
    {
        const float halfAngle = mathUtils::DegToRad(rotationDegrees) * 0.5f;
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = halfExtents },
            .transform = {
                .position = position,
                .rotationQuaternion = { 0.0f, 0.0f, std::sin(halfAngle), std::cos(halfAngle) }
            },
            .motionType = physics::PhysicsMotionType::Static
        };
    }

    void ExpectPosition(const mathUtils::Vec3& actual, const mathUtils::Vec3& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
    }

    class JoltCharacterBackendTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            threadAffinity::ResetOwnerThreadRegistry();
            threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
            ASSERT_TRUE(runtime.Initialize());
            ASSERT_TRUE(world.Initialize());
        }

        void TearDown() override
        {
            world.Shutdown();
            runtime.Shutdown();
            threadAffinity::ResetOwnerThreadRegistry();
        }

        physics::JoltRuntime runtime;
        physics::JoltPhysicsWorld world{ runtime };
    };
    
    void SimulateSteps(physics::JoltPhysicsWorld& world, const int count)
    {
        for (int step = 0; step < count; ++step)
        {
            EXPECT_EQ(world.Update(static_cast<float>(FixedDeltaSec60)), 1u);
        }
    }

    void SettleCharacter(
        physics::JoltPhysicsWorld& world,
        const physics::PhysicsCharacterHandle character)
    {
        SimulateSteps(world, 90);
        const auto ground = world.GetCharacterGroundState(character);
        ASSERT_TRUE(ground.has_value());
        ASSERT_TRUE(ground->bIsWalkable);
    }
}

TEST(JoltCharacterBackend, RequiresInitializedWorld)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld world(runtime);
    EXPECT_EQ(world.CreateCharacter(CharacterDescriptor()), physics::InvalidPhysicsCharacterHandle);
}

TEST_F(JoltCharacterBackendTest, RejectsInvalidDescriptor)
{
    auto descriptor = CharacterDescriptor();
    descriptor.collider.radius = 0.0f;
    EXPECT_EQ(world.CreateCharacter(descriptor), physics::InvalidPhysicsCharacterHandle);
}

TEST_F(JoltCharacterBackendTest, SupportsLifetimePositionVelocityAndTeleport)
{
    const auto first = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(first.IsValid());
    EXPECT_TRUE(world.IsCharacterValid(first));
    const auto position = world.GetCharacterPosition(first);
    ASSERT_TRUE(position.has_value());
    ExpectPosition(*position, { 3.0f, 5.0f, -2.0f });
    const auto velocity = world.GetCharacterVelocity(first);
    ASSERT_TRUE(velocity.has_value());
    ExpectPosition(*velocity, {});

    EXPECT_TRUE(world.TeleportCharacter(first, { -4.0f, 8.0f, 2.0f }));
    const auto teleported = world.GetCharacterPosition(first);
    ASSERT_TRUE(teleported.has_value());
    ExpectPosition(*teleported, { -4.0f, 8.0f, 2.0f });

    EXPECT_FALSE(world.TeleportCharacter(first, {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f }));
    EXPECT_FALSE(world.TeleportCharacter(first, {
        std::numeric_limits<float>::infinity(), 0.0f, 0.0f }));
    const auto positionAfterRejectedTeleport = world.GetCharacterPosition(first);
    ASSERT_TRUE(positionAfterRejectedTeleport.has_value());
    ExpectPosition(*positionAfterRejectedTeleport, { -4.0f, 8.0f, 2.0f });

    EXPECT_TRUE(world.DestroyCharacter(first));
    EXPECT_FALSE(world.DestroyCharacter(first));
    EXPECT_FALSE(world.IsCharacterValid(first));
    EXPECT_FALSE(world.GetCharacterPosition(first).has_value());
    EXPECT_FALSE(world.GetCharacterVelocity(first).has_value());
    EXPECT_FALSE(world.TeleportCharacter(first, { 1.0f, 2.0f, 3.0f }));

    const auto second = world.CreateCharacter(CharacterDescriptor());
    EXPECT_EQ(second.index, first.index);
    EXPECT_NE(second.generation, first.generation);
    EXPECT_FALSE(world.IsCharacterValid(first));
    EXPECT_TRUE(world.IsCharacterValid(second));
}

TEST_F(JoltCharacterBackendTest, ShutdownGenerationCannotBeResurrected)
{
    const auto oldHandle = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(oldHandle.IsValid());
    world.Shutdown();
    ASSERT_TRUE(world.Initialize());
    const auto newHandle = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(newHandle.IsValid());
    EXPECT_FALSE(world.IsCharacterValid(oldHandle));
    EXPECT_TRUE(world.IsCharacterValid(newHandle));
    if (newHandle.index == oldHandle.index)
    {
        EXPECT_NE(newHandle.generation, oldHandle.generation);
    }
}

TEST_F(JoltCharacterBackendTest, CharacterAndStaticFloorHaveIndependentLifetimes)
{
    const auto floor = world.CreateBody(FloorDescriptor());
    ASSERT_TRUE(world.IsBodyValid(floor));
    const auto character = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(world.IsCharacterValid(character));
    const auto position = world.GetCharacterPosition(character);
    ASSERT_TRUE(position.has_value());
    ExpectPosition(*position, { 3.0f, 5.0f, -2.0f });

    EXPECT_TRUE(world.DestroyBody(floor));
    EXPECT_TRUE(world.IsCharacterValid(character));
    const auto replacementFloor = world.CreateBody(FloorDescriptor());
    ASSERT_TRUE(world.IsBodyValid(replacementFloor));
    EXPECT_TRUE(world.DestroyCharacter(character));
    EXPECT_TRUE(world.IsBodyValid(replacementFloor));
}

TEST_F(JoltCharacterBackendTest, FallsLandsAndReportsRegisteredGround)
{
    const auto floor = world.CreateBody(FloorDescriptor());
    ASSERT_TRUE(floor.IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 4.0f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());

    SimulateSteps(world, 180);

    const auto position = world.GetCharacterPosition(character);
    const auto velocity = world.GetCharacterVelocity(character);
    const auto ground = world.GetCharacterGroundState(character);
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(velocity.has_value());
    ASSERT_TRUE(ground.has_value());
    EXPECT_NEAR(position->y, 1.0f, 0.08f);
    EXPECT_NEAR(velocity->y, 0.0f, 0.2f);
    EXPECT_TRUE(ground->bIsSupported);
    EXPECT_TRUE(ground->bIsWalkable);
    EXPECT_EQ(ground->body, floor);
    EXPECT_EQ(ground->surface, physics::DefaultSurfaceType);
}

TEST_F(JoltCharacterBackendTest, DesiredHorizontalVelocityMovesOnlyOnFixedStepsAndIsClamped)
{
    ASSERT_TRUE(world.CreateBody(FloorDescriptor()).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    descriptor.maximumSpeed = 2.0f;
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SimulateSteps(world, 10);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 20.0f, 100.0f, 0.0f }));
    const auto before = world.GetCharacterPosition(character);
    ASSERT_TRUE(before.has_value());
    EXPECT_EQ(world.Update(static_cast<float>(FixedDeltaSec60 * 0.5)), 0u);
    const auto positionWithoutStep = world.GetCharacterPosition(character);
    ASSERT_TRUE(positionWithoutStep.has_value());
    ExpectPosition(*positionWithoutStep, *before);
    SimulateSteps(world, 60);

    const auto after = world.GetCharacterPosition(character);
    const auto velocity = world.GetCharacterVelocity(character);
    const auto ground = world.GetCharacterGroundState(character);
    ASSERT_TRUE(after.has_value());
    ASSERT_TRUE(velocity.has_value());
    ASSERT_TRUE(ground.has_value());
    EXPECT_GT(after->x - before->x, 1.5f);
    EXPECT_LT(after->x - before->x, 2.2f);
    EXPECT_LE(std::sqrt(velocity->x * velocity->x + velocity->z * velocity->z), 2.01f);
    EXPECT_TRUE(ground->bIsWalkable);
}

TEST_F(JoltCharacterBackendTest, MotionObservationAccumulatesEveryFixedStepUntilConsumed)
{
    ASSERT_TRUE(world.CreateBody(FloorDescriptor()).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SimulateSteps(world, 10);
    ASSERT_TRUE(world.ConsumeCharacterMotionObservation(character).has_value());
    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 2.0f, 0.0f, 0.0f }));

    const auto before = world.GetCharacterPosition(character);
    ASSERT_TRUE(before.has_value());
    ASSERT_EQ(world.Update(static_cast<float>(FixedDeltaSec60 * 2.0)), 2u);
    const auto after = world.GetCharacterPosition(character);
    const auto observation = world.ConsumeCharacterMotionObservation(character);
    ASSERT_TRUE(after.has_value());
    ASSERT_TRUE(observation.has_value());
    EXPECT_EQ(observation->stepCount, 2u);
    float accumulatedX = 0.0f;
    for (std::uint32_t step = 0u; step < observation->stepCount; ++step)
    {
        accumulatedX += observation->steps[step].displacement.x;
    }
    EXPECT_NEAR(accumulatedX, after->x - before->x, 0.0001f);

    const auto consumed = world.ConsumeCharacterMotionObservation(character);
    ASSERT_TRUE(consumed.has_value());
    EXPECT_EQ(consumed->stepCount, 0u);
}

TEST_F(JoltCharacterBackendTest, MultiStepObservationRetainsProgressBeforeFinalWallStop)
{
    ASSERT_TRUE(world.CreateBody(StaticBox(
        { 0.0f, -0.5f, 0.0f }, { 8.0f, 0.5f, 8.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox(
        { 1.0f, 2.0f, 0.0f }, { 0.25f, 2.0f, 8.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);
    ASSERT_TRUE(world.ConsumeCharacterMotionObservation(character).has_value());
    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 6.0f, 0.0f, 0.0f }));

    ASSERT_EQ(world.Update(static_cast<float>(FixedDeltaSec60 * 4.0)), 4u);
    const auto latestVelocity = world.GetCharacterVelocity(character);
    const auto observation = world.ConsumeCharacterMotionObservation(character);
    ASSERT_TRUE(latestVelocity.has_value());
    ASSERT_TRUE(observation.has_value());
    EXPECT_EQ(observation->stepCount, 4u);
    EXPECT_GT(observation->steps[0].displacement.x, 0.01f);
    EXPECT_LT(std::abs(observation->steps[observation->stepCount - 1u].displacement.x), 0.001f);
    EXPECT_LT(std::abs(latestVelocity->x), 0.05f);
}

TEST_F(JoltCharacterBackendTest, RejectsInvalidIntentAndStaleHandles)
{
    const auto stale = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(stale.IsValid());
    EXPECT_FALSE(world.SetCharacterDesiredVelocity(stale, {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f }));
    EXPECT_FALSE(world.SetCharacterDesiredVelocity(stale, {
        0.0f, 0.0f, std::numeric_limits<float>::infinity() }));
    EXPECT_TRUE(world.DestroyCharacter(stale));
    const auto replacement = world.CreateCharacter(CharacterDescriptor());
    ASSERT_TRUE(replacement.IsValid());
    EXPECT_FALSE(world.SetCharacterDesiredVelocity(stale, { 1.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.GetCharacterGroundState(stale).has_value());
    EXPECT_TRUE(world.SetCharacterDesiredVelocity(replacement, {}));
}

TEST_F(JoltCharacterBackendTest, CollidesWithAndSlidesAlongWall)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ 0.0f, -0.5f, 0.0f }, { 8.0f, 0.5f, 8.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox({ 1.0f, 2.0f, 0.0f }, { 0.25f, 2.0f, 8.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { -1.0f, 1.05f, -3.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 4.0f, 0.0f, 4.0f }));
    SimulateSteps(world, 90);

    const auto position = world.GetCharacterPosition(character);
    ASSERT_TRUE(position.has_value());
    EXPECT_LT(position->x, 0.32f);
    EXPECT_GT(position->z, 1.0f);
}

TEST_F(JoltCharacterBackendTest, TraversesWalkableSlope)
{
    constexpr float slopeDegrees = 20.0f;
    constexpr float slopeCenterHeight = 1.133f;
    ASSERT_TRUE(world.CreateBody(StaticBox({ -6.0f, -0.5f, 0.0f }, { 2.0f, 0.5f, 3.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox(
        { 0.0f, slopeCenterHeight, 0.0f }, { 4.0f, 0.25f, 3.0f }, slopeDegrees)).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { -5.0f, 1.05f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 3.0f, 0.0f, 0.0f }));
    SimulateSteps(world, 120);

    const auto position = world.GetCharacterPosition(character);
    const auto ground = world.GetCharacterGroundState(character);
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(ground.has_value());
    EXPECT_GT(position->x, 0.0f);
    EXPECT_GT(position->y, 1.8f);
    EXPECT_TRUE(ground->bIsWalkable);
}

TEST_F(JoltCharacterBackendTest, RejectsSlopeAboveConfiguredLimit)
{
    constexpr float slopeDegrees = 60.0f;
    constexpr float slopeCenterHeight = 3.34f;

    ASSERT_TRUE(world.CreateBody(
        StaticBox(
            { -4.0f, -0.5f, 0.0f },
            { 2.0f, 0.5f, 3.0f }))
        .IsValid());

    ASSERT_TRUE(world.CreateBody(
        StaticBox(
            { 0.0f, slopeCenterHeight, 0.0f },
            { 4.0f, 0.25f, 3.0f },
            slopeDegrees))
        .IsValid());

    auto descriptor = CharacterDescriptor();
    descriptor.position = { -3.5f, 1.05f, 0.0f };
    descriptor.maximumSlopeAngleDegrees = 45.0f;

    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());

    SettleCharacter(world, character);

    const auto settledPosition = world.GetCharacterPosition(character);
    ASSERT_TRUE(settledPosition.has_value());

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(
        character,
        { 3.0f, 0.0f, 0.0f }));

    float maximumObservedHeight = settledPosition->y;

    for (int step = 0; step < 120; ++step)
    {
        SimulateSteps(world, 1);

        const auto position = world.GetCharacterPosition(character);
        ASSERT_TRUE(position.has_value());

        maximumObservedHeight =
            std::max(maximumObservedHeight, position->y);
    }

    const auto finalPosition = world.GetCharacterPosition(character);
    ASSERT_TRUE(finalPosition.has_value());

    // A surface above the configured slope limit must block ordinary uphill traversal.
    EXPECT_LT(finalPosition->x, -1.0f);

    // The character must not gain meaningful elevation by treating the steep surface as walkable ground.
    EXPECT_LT(maximumObservedHeight, 1.8f);
}

TEST_F(JoltCharacterBackendTest, TraversesStepWithinMaximumHeight)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ 0.0f, -0.5f, 0.0f }, { 8.0f, 0.5f, 3.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox({ 3.0f, 0.125f, 0.0f }, { 2.0f, 0.125f, 3.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 2.0f, 0.0f, 0.0f }));
    SimulateSteps(world, 90);

    const auto position = world.GetCharacterPosition(character);
    ASSERT_TRUE(position.has_value());
    EXPECT_GT(position->x, 2.0f);
    EXPECT_NEAR(position->y, 1.25f, 0.08f);
}

TEST_F(JoltCharacterBackendTest, RejectsStepAboveMaximumHeight)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ 0.0f, -0.5f, 0.0f }, { 8.0f, 0.5f, 3.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox({ 3.0f, 0.4f, 0.0f }, { 2.0f, 0.4f, 3.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 2.0f, 0.0f, 0.0f }));
    SimulateSteps(world, 150);

    const auto position = world.GetCharacterPosition(character);
    ASSERT_TRUE(position.has_value());
    EXPECT_LT(position->x, 0.58f);
    EXPECT_LT(position->y, 1.15f);
}

TEST_F(JoltCharacterBackendTest, ZeroStepHeightDisablesStepsButPreservesGrounding)
{
    constexpr float stepHeight = 0.25f;

    ASSERT_TRUE(world.CreateBody(
        StaticBox(
            { 0.0f, -0.5f, 0.0f },
            { 8.0f, 0.5f, 3.0f }))
        .IsValid());

    ASSERT_TRUE(world.CreateBody(
        StaticBox(
            { 3.0f, stepHeight * 0.5f, 0.0f },
            { 2.0f, stepHeight * 0.5f, 3.0f }))
        .IsValid());

    auto descriptor = CharacterDescriptor();

    // Keep the same total capsule height while making the vertical riser
    // taller than the lower spherical cap. This prevents ordinary collision
    // sliding from rolling the capsule onto the step.
    descriptor.collider.radius = 0.2f;
    descriptor.collider.cylinderHeight = 1.6f;
    descriptor.position = { 0.0f, 1.05f, 0.0f };
    descriptor.maximumStepHeight = 0.0f;

    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());

    SettleCharacter(world, character);

    const auto initialGround = world.GetCharacterGroundState(character);
    ASSERT_TRUE(initialGround.has_value());

    // Disabling stair traversal must not disable ordinary grounding.
    EXPECT_TRUE(initialGround->bIsSupported);
    EXPECT_TRUE(initialGround->bIsWalkable);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(
        character,
        { 2.0f, 0.0f, 0.0f }));

    SimulateSteps(world, 150);

    const auto position = world.GetCharacterPosition(character);
    ASSERT_TRUE(position.has_value());

    // With stair walking disabled, the character must remain blocked
    // in front of the vertical riser.
    EXPECT_LT(position->x, 0.85f);

    // The character must remain on the approach floor instead of climbing the step.
    EXPECT_LT(position->y, 1.15f);
}

TEST_F(JoltCharacterBackendTest, ZeroStepHeightStillFollowsSmallDownwardGroundChange)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ -2.0f, -0.4f, 0.0f }, { 2.0f, 0.5f, 3.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox({ 2.0f, -0.5f, 0.0f }, { 2.0f, 0.5f, 3.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { -1.0f, 1.15f, 0.0f };
    descriptor.maximumStepHeight = 0.0f;
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 1.0f, 0.0f, 0.0f }));
    SimulateSteps(world, 120);

    const auto position = world.GetCharacterPosition(character);
    const auto ground = world.GetCharacterGroundState(character);
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(ground.has_value());
    EXPECT_GT(position->x, 0.5f);
    EXPECT_NEAR(position->y, 1.0f, 0.08f);
    EXPECT_TRUE(ground->bIsWalkable);
}

TEST_F(JoltCharacterBackendTest, WalksOffLedgeFallsAndLandsAgain)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ -1.0f, 1.5f, 0.0f }, { 2.0f, 0.5f, 3.0f })).IsValid());
    ASSERT_TRUE(world.CreateBody(StaticBox({ 3.0f, -0.5f, 0.0f }, { 6.0f, 0.5f, 3.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { -1.5f, 3.05f, 0.0f };
    descriptor.maximumSpeed = 3.0f;
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    SettleCharacter(world, character);
    ASSERT_TRUE(world.GetCharacterGroundState(character)->bIsSupported);

    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 3.0f, 0.0f, 0.0f }));
    bool becameUnsupported = false;
    bool descended = false;
    bool landed = false;
    for (int step = 0; step < 180; ++step)
    {
        SimulateSteps(world, 1);
        const auto position = world.GetCharacterPosition(character);
        const auto ground = world.GetCharacterGroundState(character);
        ASSERT_TRUE(position.has_value());
        ASSERT_TRUE(ground.has_value());
        becameUnsupported = becameUnsupported || !ground->bIsSupported;
        descended = descended || position->y < 2.5f;
        landed = landed || (becameUnsupported && ground->bIsWalkable && position->y < 1.2f);
    }
    EXPECT_TRUE(becameUnsupported);
    EXPECT_TRUE(descended);
    EXPECT_TRUE(landed);
}

TEST_F(JoltCharacterBackendTest, VerticalWallContactDoesNotProvideGroundSupport)
{
    ASSERT_TRUE(world.CreateBody(StaticBox({ 0.0f, 2.0f, 0.0f }, { 0.25f, 4.0f, 3.0f })).IsValid());
    auto descriptor = CharacterDescriptor();
    descriptor.position = { -0.8f, 5.0f, 0.0f };
    const auto character = world.CreateCharacter(descriptor);
    ASSERT_TRUE(character.IsValid());
    ASSERT_TRUE(world.SetCharacterDesiredVelocity(character, { 3.0f, 0.0f, 0.0f }));

    SimulateSteps(world, 60);

    const auto position = world.GetCharacterPosition(character);
    const auto ground = world.GetCharacterGroundState(character);
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(ground.has_value());
    EXPECT_LT(position->y, 3.5f);
    EXPECT_FALSE(ground->bIsSupported);
    EXPECT_FALSE(ground->bIsWalkable);
}