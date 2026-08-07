#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

namespace
{
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;

    [[nodiscard]] physics::PhysicsBodyDescriptor FloorDescriptor()
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 10.0f, 0.5f, 10.0f } },
            .transform = { .position = { 0.0f, -0.5f, 0.0f } },
            .motionType = physics::PhysicsMotionType::Static
        };
    }

    [[nodiscard]] physics::PhysicsBodyDescriptor SphereDescriptor()
    {
        return {
            .shape = physics::SphereShapeDescriptor{ .radius = 0.5f },
            .transform = { .position = { 0.0f, 5.0f, 0.0f } },
            .motionType = physics::PhysicsMotionType::Dynamic
        };
    }

    class JoltFallingBodyTest : public testing::Test
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

TEST(JoltFallingBodyReadback, ReadBeforeInitializeReturnsNullopt)
{
    physics::JoltRuntime runtime;
    physics::JoltPhysicsWorld world{ runtime };
    EXPECT_EQ(world.GetBodyTransform(physics::PhysicsBodyHandle{ 0u, 1u }), std::nullopt);
    EXPECT_EQ(world.GetLinearVelocity(physics::PhysicsBodyHandle{ 0u, 1u }), std::nullopt);
}

TEST_F(JoltFallingBodyTest, InvalidHandleReturnsNullopt)
{
    EXPECT_EQ(world.GetBodyTransform(physics::InvalidPhysicsBodyHandle), std::nullopt);
    EXPECT_EQ(world.GetLinearVelocity(physics::InvalidPhysicsBodyHandle), std::nullopt);
}

TEST_F(JoltFallingBodyTest, DestroyedHandleReturnsNullopt)
{
    const auto handle = world.CreateBody(SphereDescriptor());
    ASSERT_TRUE(world.DestroyBody(handle));
    EXPECT_EQ(world.GetBodyTransform(handle), std::nullopt);
    EXPECT_EQ(world.GetLinearVelocity(handle), std::nullopt);
}

TEST_F(JoltFallingBodyTest, StaleHandleCannotReadReplacementBody)
{
    const auto staleHandle = world.CreateBody(SphereDescriptor());
    ASSERT_TRUE(world.DestroyBody(staleHandle));
    const auto currentHandle = world.CreateBody(SphereDescriptor());
    ASSERT_EQ(staleHandle.index, currentHandle.index);
    EXPECT_EQ(world.GetBodyTransform(staleHandle), std::nullopt);
    EXPECT_EQ(world.GetLinearVelocity(staleHandle), std::nullopt);
    EXPECT_TRUE(world.GetBodyTransform(currentHandle).has_value());
    EXPECT_TRUE(world.GetLinearVelocity(currentHandle).has_value());
    EXPECT_TRUE(world.DestroyBody(currentHandle));
}

TEST_F(JoltFallingBodyTest, Smoke_CreateSimulateReadAndDestroyBodies)
{
    const auto floorHandle = world.CreateBody(FloorDescriptor());
    const auto sphereHandle = world.CreateBody(SphereDescriptor());
    ASSERT_TRUE(world.IsBodyValid(floorHandle));
    ASSERT_TRUE(world.IsBodyValid(sphereHandle));
    const auto initialTransform = world.GetBodyTransform(sphereHandle);
    ASSERT_TRUE(initialTransform.has_value());

    for (int stepIndex = 0; stepIndex < 20; ++stepIndex)
    {
        ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    }

    const auto movedTransform = world.GetBodyTransform(sphereHandle);
    ASSERT_TRUE(movedTransform.has_value());
    EXPECT_LT(movedTransform->position.y, initialTransform->position.y);
    EXPECT_TRUE(world.IsBodyValid(floorHandle));
    EXPECT_TRUE(world.IsBodyValid(sphereHandle));
    EXPECT_TRUE(world.DestroyBody(sphereHandle));
    EXPECT_TRUE(world.DestroyBody(floorHandle));
    EXPECT_FALSE(world.IsBodyValid(sphereHandle));
    EXPECT_FALSE(world.IsBodyValid(floorHandle));
}

TEST_F(JoltFallingBodyTest, DynamicSphereFallsOntoStaticFloorAndComesToRest)
{
    constexpr int SimulationStepCount = 300;
    constexpr float RestingHeightTolerance = 0.03f;
    constexpr float HorizontalTolerance = 0.001f;
    constexpr float VelocityTolerance = 0.01f;

    const auto floorHandle = world.CreateBody(FloorDescriptor());
    const auto sphereHandle = world.CreateBody(SphereDescriptor());
    ASSERT_TRUE(world.IsBodyValid(floorHandle));
    ASSERT_TRUE(world.IsBodyValid(sphereHandle));
    const auto initialTransform = world.GetBodyTransform(sphereHandle);
    ASSERT_TRUE(initialTransform.has_value());

    for (int stepIndex = 0; stepIndex < SimulationStepCount; ++stepIndex)
    {
        ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    }

    const auto finalTransform = world.GetBodyTransform(sphereHandle);
    const auto finalVelocity = world.GetLinearVelocity(sphereHandle);
    ASSERT_TRUE(finalTransform.has_value());
    ASSERT_TRUE(finalVelocity.has_value());
    EXPECT_LT(finalTransform->position.y, initialTransform->position.y);
    EXPECT_GT(finalTransform->position.y, 0.45f);
    EXPECT_NEAR(finalTransform->position.y, 0.5f, RestingHeightTolerance);
    EXPECT_NEAR(finalTransform->position.x, 0.0f, HorizontalTolerance);
    EXPECT_NEAR(finalTransform->position.z, 0.0f, HorizontalTolerance);
    EXPECT_TRUE(mathUtils::IsFinite(*finalVelocity));
    EXPECT_NEAR(finalVelocity->y, 0.0f, VelocityTolerance);

    EXPECT_TRUE(world.DestroyBody(sphereHandle));
    EXPECT_TRUE(world.DestroyBody(floorHandle));
    EXPECT_FALSE(world.IsBodyValid(sphereHandle));
    EXPECT_FALSE(world.IsBodyValid(floorHandle));
}