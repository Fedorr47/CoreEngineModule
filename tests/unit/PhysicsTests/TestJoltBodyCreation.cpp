#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <limits>

namespace
{
    [[nodiscard]] physics::PhysicsBodyDescriptor StaticBox()
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 1.0f, 2.0f, 3.0f } },
            .transform = {},
            .motionType = physics::PhysicsMotionType::Static
        };
    }

    class JoltBodyCreationTest : public testing::Test
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

TEST(JoltBodyCreation, CreateBeforeInitializeReturnsInvalid)
{
    physics::JoltRuntime runtime;
    physics::JoltPhysicsWorld world{ runtime };
    EXPECT_EQ(world.CreateBody(StaticBox()), physics::InvalidPhysicsBodyHandle);
}

TEST_F(JoltBodyCreationTest, RejectsInvalidShape)
{
    const physics::PhysicsBodyHandle handle = world.CreateBody({});
    EXPECT_EQ(handle, physics::InvalidPhysicsBodyHandle);
    EXPECT_FALSE(world.IsBodyValid(handle));
}

TEST_F(JoltBodyCreationTest, RejectsUnsupportedKinematicBody)
{
    auto descriptor = StaticBox();
    descriptor.motionType = physics::PhysicsMotionType::Kinematic;
    EXPECT_EQ(world.CreateBody(descriptor), physics::InvalidPhysicsBodyHandle);
}

TEST_F(JoltBodyCreationTest, RejectsInvalidTransform)
{
    auto descriptor = StaticBox();
    descriptor.transform.position.x = std::numeric_limits<float>::infinity();
    EXPECT_EQ(world.CreateBody(descriptor), physics::InvalidPhysicsBodyHandle);

    descriptor = StaticBox();
    descriptor.transform.rotationQuaternion = {};
    EXPECT_EQ(world.CreateBody(descriptor), physics::InvalidPhysicsBodyHandle);
}

TEST_F(JoltBodyCreationTest, CreatesStaticBoxBody)
{
    const auto handle = world.CreateBody(StaticBox());
    EXPECT_TRUE(handle.IsValid());
    EXPECT_TRUE(world.IsBodyValid(handle));
}

TEST_F(JoltBodyCreationTest, CreatesDynamicSphereBody)
{
    const physics::PhysicsBodyDescriptor descriptor{
        .shape = physics::SphereShapeDescriptor{ .radius = 1.0f },
        .motionType = physics::PhysicsMotionType::Dynamic
    };
    const auto handle = world.CreateBody(descriptor);
    EXPECT_TRUE(handle.IsValid());
    EXPECT_TRUE(world.IsBodyValid(handle));
}

TEST_F(JoltBodyCreationTest, CreatesCapsuleBody)
{
    const physics::PhysicsBodyDescriptor descriptor{
        .shape = physics::CapsuleShapeDescriptor{ .radius = 0.5f, .cylinderHeight = 2.0f }
    };
    EXPECT_TRUE(world.IsBodyValid(world.CreateBody(descriptor)));
}

TEST_F(JoltBodyCreationTest, MultipleBodiesReceiveIndependentHandles)
{
    const auto first = world.CreateBody(StaticBox());
    const auto second = world.CreateBody(StaticBox());
    EXPECT_NE(first, second);
    EXPECT_TRUE(world.IsBodyValid(first));
    EXPECT_TRUE(world.IsBodyValid(second));
}

TEST_F(JoltBodyCreationTest, DestroyBodyInvalidatesHandle)
{
    const auto handle = world.CreateBody(StaticBox());
    EXPECT_TRUE(world.DestroyBody(handle));
    EXPECT_FALSE(world.IsBodyValid(handle));
    EXPECT_FALSE(world.DestroyBody(handle));
}

TEST_F(JoltBodyCreationTest, DestroyRejectsInvalidHandle)
{
    EXPECT_FALSE(world.DestroyBody(physics::InvalidPhysicsBodyHandle));
}

TEST_F(JoltBodyCreationTest, ReusesDestroyedSlotWithNewGeneration)
{
    const auto first = world.CreateBody(StaticBox());
    ASSERT_TRUE(world.DestroyBody(first));
    const auto second = world.CreateBody(StaticBox());
    EXPECT_EQ(first.index, second.index);
    EXPECT_NE(first.generation, second.generation);
    EXPECT_FALSE(world.IsBodyValid(first));
    EXPECT_TRUE(world.IsBodyValid(second));
}

TEST_F(JoltBodyCreationTest, StaleHandleCannotDestroyReplacementBody)
{
    const auto stale = world.CreateBody(StaticBox());
    ASSERT_TRUE(world.DestroyBody(stale));
    const auto current = world.CreateBody(StaticBox());
    EXPECT_FALSE(world.DestroyBody(stale));
    EXPECT_TRUE(world.IsBodyValid(current));
    EXPECT_TRUE(world.DestroyBody(current));
}

TEST(JoltBodyCreation, TwoWorldsHaveIndependentRegistries)
{
    threadAffinity::ResetOwnerThreadRegistry();
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld first{ runtime };
    physics::JoltPhysicsWorld second{ runtime };
    ASSERT_TRUE(first.Initialize());
    ASSERT_TRUE(second.Initialize());
    const auto firstHandle = first.CreateBody(StaticBox());
    const auto secondHandle = second.CreateBody(StaticBox());
    EXPECT_TRUE(first.DestroyBody(firstHandle));
    EXPECT_TRUE(second.IsBodyValid(secondHandle));
    first.Shutdown();
    second.Shutdown();
    runtime.Shutdown();
    threadAffinity::ResetOwnerThreadRegistry();
}

TEST_F(JoltBodyCreationTest, ShutdownCleansUpLiveBodies)
{
    const auto first = world.CreateBody(StaticBox());
    const auto second = world.CreateBody(StaticBox());
    world.Shutdown();
    EXPECT_FALSE(world.IsBodyValid(first));
    EXPECT_FALSE(world.IsBodyValid(second));
    world.Shutdown();
}

TEST_F(JoltBodyCreationTest, OldHandleDoesNotReviveAfterReinitialize)
{
    const auto oldHandle = world.CreateBody(StaticBox());
    world.Shutdown();
    ASSERT_TRUE(world.Initialize());
    const auto newHandle = world.CreateBody(StaticBox());
    EXPECT_FALSE(world.IsBodyValid(oldHandle));
    EXPECT_TRUE(world.IsBodyValid(newHandle));
    EXPECT_NE(oldHandle.generation, newHandle.generation);
}