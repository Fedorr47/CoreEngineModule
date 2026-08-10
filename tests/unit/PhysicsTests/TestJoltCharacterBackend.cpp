#include <gtest/gtest.h>

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

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