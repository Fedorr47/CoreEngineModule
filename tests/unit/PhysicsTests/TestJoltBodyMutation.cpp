#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <limits>

namespace
{
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;
    constexpr float Tolerance = 0.001f;

    [[nodiscard]] physics::PhysicsBodyDescriptor BodyDescriptor(
        const physics::PhysicsMotionType motionType)
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 0.5f, 0.5f, 0.5f } },
            .transform = {},
            .motionType = motionType
        };
    }

    class JoltBodyMutationTest : public testing::Test
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

TEST_F(JoltBodyMutationTest, SetsDynamicLinearVelocity)
{
    const auto body = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    ASSERT_TRUE(world.SetLinearVelocity(body, { 2.0f, 3.0f, 4.0f }));
    const auto velocity = world.GetLinearVelocity(body);
    ASSERT_TRUE(velocity.has_value());
    EXPECT_NEAR(velocity->x, 2.0f, Tolerance);
    EXPECT_NEAR(velocity->y, 3.0f, Tolerance);
    EXPECT_NEAR(velocity->z, 4.0f, Tolerance);
}

TEST_F(JoltBodyMutationTest, ImpulseChangesDynamicVelocity)
{
    const auto body = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    ASSERT_TRUE(world.AddImpulse(body, { 3.0f, 0.0f, 0.0f }));
    ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    const auto velocity = world.GetLinearVelocity(body);
    ASSERT_TRUE(velocity.has_value());
    EXPECT_GT(velocity->x, 0.0f);
}

TEST_F(JoltBodyMutationTest, TeleportImmediatelyChangesDynamicTransform)
{
    const auto body = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    const physics::PhysicsTransform target{
        .position = { 4.0f, 5.0f, 6.0f },
        .rotationQuaternion = { 0.0f, 0.0f, 1.0f, 1.0f }
    };
    ASSERT_TRUE(world.TeleportBody(body, target));
    const auto transform = world.GetBodyTransform(body);
    ASSERT_TRUE(transform.has_value());
    EXPECT_NEAR(transform->position.x, 4.0f, Tolerance);
    EXPECT_NEAR(transform->position.y, 5.0f, Tolerance);
    EXPECT_NEAR(transform->position.z, 6.0f, Tolerance);
    EXPECT_NEAR(transform->rotationQuaternion.z, 0.7071067f, Tolerance);
    EXPECT_NEAR(transform->rotationQuaternion.w, 0.7071067f, Tolerance);
}

TEST_F(JoltBodyMutationTest, KinematicBodyMovesToTargetDuringUpdate)
{
    const auto body = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Kinematic));
    const physics::PhysicsTransform target{ .position = { 1.0f, 2.0f, 3.0f } };
    ASSERT_TRUE(world.MoveKinematic(body, target, FixedDeltaSeconds));

    const auto beforeUpdate = world.GetBodyTransform(body);
    ASSERT_TRUE(beforeUpdate.has_value());
    EXPECT_NEAR(beforeUpdate->position.x, 0.0f, Tolerance);
    ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);

    const auto afterUpdate = world.GetBodyTransform(body);
    ASSERT_TRUE(afterUpdate.has_value());
    EXPECT_NEAR(afterUpdate->position.x, 1.0f, Tolerance);
    EXPECT_NEAR(afterUpdate->position.y, 2.0f, Tolerance);
    EXPECT_NEAR(afterUpdate->position.z, 3.0f, Tolerance);
}

TEST_F(JoltBodyMutationTest, KinematicTeleportIsImmediateRatherThanTargetedMovement)
{
    const auto body = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Kinematic));
    const physics::PhysicsTransform movementTarget{ .position = { 30.0f, 0.0f, 0.0f } };
    ASSERT_TRUE(world.MoveKinematic(body, movementTarget, 0.5f));
    ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    const auto movingTransform = world.GetBodyTransform(body);
    ASSERT_TRUE(movingTransform.has_value());
    EXPECT_GT(movingTransform->position.x, 0.0f);

    const physics::PhysicsTransform teleportTarget{ .position = { -2.0f, 3.0f, 1.0f } };
    ASSERT_TRUE(world.TeleportBody(body, teleportTarget));

    const auto immediateTransform = world.GetBodyTransform(body);
    ASSERT_TRUE(immediateTransform.has_value());
    EXPECT_NEAR(immediateTransform->position.x, -2.0f, Tolerance);
    EXPECT_NEAR(immediateTransform->position.y, 3.0f, Tolerance);
    EXPECT_NEAR(immediateTransform->position.z, 1.0f, Tolerance);
    for (int stepIndex = 0; stepIndex < 3; ++stepIndex)
    {
        ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    }

    const auto transformAfterUpdate = world.GetBodyTransform(body);
    ASSERT_TRUE(transformAfterUpdate.has_value());
    EXPECT_NEAR(transformAfterUpdate->position.x, -2.0f, Tolerance);
    EXPECT_NEAR(transformAfterUpdate->position.y, 3.0f, Tolerance);
    EXPECT_NEAR(transformAfterUpdate->position.z, 1.0f, Tolerance);
}

TEST_F(JoltBodyMutationTest, RejectsOperationsUnsupportedByMotionType)
{
    const auto staticBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Static));
    const auto kinematicBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Kinematic));
    const auto dynamicBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    const physics::PhysicsTransform target{ .position = { 1.0f, 0.0f, 0.0f } };

    EXPECT_FALSE(world.SetLinearVelocity(staticBody, { 1.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.AddImpulse(staticBody, { 1.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.TeleportBody(staticBody, target));
    EXPECT_FALSE(world.MoveKinematic(staticBody, target, FixedDeltaSeconds));
    EXPECT_FALSE(world.SetLinearVelocity(kinematicBody, { 1.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.AddImpulse(kinematicBody, { 1.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.MoveKinematic(dynamicBody, target, FixedDeltaSeconds));
}

TEST_F(JoltBodyMutationTest, InvalidInputsFailSafely)
{
    const float infinity = std::numeric_limits<float>::infinity();
    const physics::PhysicsTransform invalidTransform{ .position = { infinity, 0.0f, 0.0f } };
    EXPECT_FALSE(world.SetLinearVelocity(physics::InvalidPhysicsBodyHandle, {}));
    EXPECT_FALSE(world.AddImpulse(physics::InvalidPhysicsBodyHandle, {}));
    EXPECT_FALSE(world.TeleportBody(physics::InvalidPhysicsBodyHandle, {}));
    EXPECT_FALSE(world.MoveKinematic(
        physics::InvalidPhysicsBodyHandle, {}, FixedDeltaSeconds));

    const auto dynamicBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    const auto kinematicBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Kinematic));
    EXPECT_FALSE(world.SetLinearVelocity(dynamicBody, { infinity, 0.0f, 0.0f }));
    EXPECT_FALSE(world.AddImpulse(dynamicBody, { infinity, 0.0f, 0.0f }));
    EXPECT_FALSE(world.TeleportBody(dynamicBody, invalidTransform));
    EXPECT_FALSE(world.MoveKinematic(kinematicBody, {}, 0.0f));
}

TEST_F(JoltBodyMutationTest, StaleHandleCannotMutateReplacementBody)
{
    const auto staleBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    ASSERT_TRUE(world.DestroyBody(staleBody));
    const auto currentBody = world.CreateBody(BodyDescriptor(physics::PhysicsMotionType::Dynamic));
    ASSERT_EQ(staleBody.index, currentBody.index);

    EXPECT_FALSE(world.SetLinearVelocity(staleBody, { 10.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.AddImpulse(staleBody, { 10.0f, 0.0f, 0.0f }));
    EXPECT_FALSE(world.TeleportBody(
        staleBody, physics::PhysicsTransform{ .position = { 10.0f, 0.0f, 0.0f } }));
    EXPECT_FALSE(world.MoveKinematic(
        staleBody, physics::PhysicsTransform{ .position = { 10.0f, 0.0f, 0.0f } },
        FixedDeltaSeconds));
    const auto transform = world.GetBodyTransform(currentBody);
    const auto velocity = world.GetLinearVelocity(currentBody);
    ASSERT_TRUE(transform.has_value());
    ASSERT_TRUE(velocity.has_value());
    EXPECT_NEAR(transform->position.x, 0.0f, Tolerance);
    EXPECT_NEAR(velocity->x, 0.0f, Tolerance);
}