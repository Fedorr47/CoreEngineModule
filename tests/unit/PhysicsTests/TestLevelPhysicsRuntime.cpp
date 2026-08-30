#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"
#include "Physics/LevelPhysicsRuntime.h"
#include "App/GameplayPhysicsObstacleQuery.h"

#include <string>
#include <type_traits>
#include <limits>
#include <cmath>

namespace
{
    [[nodiscard]] rendern::LevelNode StaticNode()
    {
        return {
            .name = "Floor",
            .transform = { .position = { 0.0f, -0.5f, 0.0f } },
            .physicsBody = rendern::LevelPhysicsBodyDef{
                .shape = physics::BoxShapeDescriptor{ .halfExtents = { 10.0f, 0.5f, 10.0f } },
                .motionType = physics::PhysicsMotionType::Static
            }
        };
    }

    [[nodiscard]] rendern::LevelNode DynamicNode()
    {
        return {
            .name = "Sphere",
            .transform = { .position = { 0.0f, 5.0f, 0.0f } },
            .physicsBody = rendern::LevelPhysicsBodyDef{
                .shape = physics::SphereShapeDescriptor{ .radius = 0.5f },
                .motionType = physics::PhysicsMotionType::Dynamic
            }
        };
    }
    
    [[nodiscard]] rendern::LevelNode KinematicBoxNode()
    {
        auto node = StaticNode();
        node.name = "KinematicBox";
        node.transform.position = {};
        node.physicsBody->shape = physics::BoxShapeDescriptor{ .halfExtents = { 2.0f, 0.25f, 0.25f } };
        node.physicsBody->motionType = physics::PhysicsMotionType::Kinematic;
        return node;
    }

    [[nodiscard]] bool OrientationsEquivalent(
        const mathUtils::Vec3& eulerDegrees, const mathUtils::Vec4& quaternion)
    {
        const auto expected = mathUtils::EulerDegreesZYXToQuat(eulerDegrees);
        const auto actual = mathUtils::NormalizeQuat(quaternion);
        return std::fabs(mathUtils::DotQuat(expected, actual)) > 1.0f - 1.0e-5f;
    }

    class LevelPhysicsRuntime : public testing::Test
    {
    protected:
        void SetUp() override
        {
            threadAffinity::ResetOwnerThreadRegistry();
            threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
            ASSERT_TRUE(joltRuntime.Initialize());
            ASSERT_TRUE(world.Initialize());
        }

        void TearDown() override
        {
            runtime.Shutdown();
            world.Shutdown();
            joltRuntime.Shutdown();
            threadAffinity::ResetOwnerThreadRegistry();
        }

        physics::JoltRuntime joltRuntime;
        physics::JoltPhysicsWorld world{ joltRuntime };
        physics::LevelPhysicsRuntime runtime{ world };
        rendern::LevelAsset level;
        rendern::LevelInstance levelInstance;
        rendern::Scene scene;
        std::string error;
    };
}

static_assert(!std::is_copy_constructible_v<physics::LevelPhysicsRuntime>);
static_assert(!std::is_move_constructible_v<physics::LevelPhysicsRuntime>);

TEST_F(LevelPhysicsRuntime, EnterGameCreatesBindings)
{
    level.nodes = { StaticNode(), DynamicNode() };
    EXPECT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    EXPECT_TRUE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 2u);
}

TEST_F(LevelPhysicsRuntime, SteeringPlaygroundAuthoredObstacleIsQueryableAsStaticWorld)
{
    level = rendern::LoadLevelAssetFromJson("levels/ai_steering_playground.level.json");
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    appRuntime::GameplayPhysicsObstacleQuery query{world};
    rendern::GameplayObstacleProbeHit hit{};
    ASSERT_TRUE(query.Probe({
        .origin = {-2.0f, 0.9f, -6.0f},
        .direction = {1.0f, 0.0f, 0.0f},
        .maximumDistance = 4.0f}, hit));
    EXPECT_TRUE(std::isfinite(hit.distance));
    EXPECT_GT(hit.distance, 0.0f);
    EXPECT_LT(hit.distance, 4.0f);
}

TEST_F(LevelPhysicsRuntime, NodesWithoutPhysicsAreSkipped)
{
    level.nodes = { rendern::LevelNode{ .name = "VisualOnly" }, DynamicNode() };
    EXPECT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    EXPECT_EQ(runtime.GetBindingCount(), 1u);
}

TEST_F(LevelPhysicsRuntime, EnterGameRejectsParentedPhysicsNode)
{
    auto node = DynamicNode();
    node.parent = 0;
    level.nodes = { rendern::LevelNode{ .name = "Parent" }, node };
    EXPECT_FALSE(runtime.EnterGame(level, levelInstance, scene, error));
    EXPECT_NE(error.find("root-level"), std::string::npos);
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);
}

TEST_F(LevelPhysicsRuntime, EnterGameAcceptsRotatedStaticPhysicsNode)
{
    auto node = KinematicBoxNode();
    node.physicsBody->motionType = physics::PhysicsMotionType::Static;
    node.transform.rotationDegrees.z = 90.0f;
    level.nodes = { node };
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;

    const auto hit = world.RayCastClosest({
        .origin = { 0.0f, 3.0f, 0.0f },
        .direction = { 0.0f, -1.0f, 0.0f },
        .maxDistance = 4.0f
    });
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->position.y, 2.0f, 1.0e-3f);
}

TEST_F(LevelPhysicsRuntime, EnterGameRejectsMatrixAuthoredPhysicsNode)
{
    auto node = DynamicNode();
    node.transform.useMatrix = true;
    level.nodes = { node };
    EXPECT_FALSE(runtime.EnterGame(level, levelInstance, scene, error));
    EXPECT_NE(error.find("matrix-authored transform"), std::string::npos);
    EXPECT_FALSE(runtime.IsActive());
}

TEST_F(LevelPhysicsRuntime, ValidationFailureDoesNotCreateEarlierBodies)
{
    auto invalid = DynamicNode();
    invalid.physicsBody->shape = physics::SphereShapeDescriptor{ .radius = 0.0f };
    level.nodes = { StaticNode(), invalid };
    EXPECT_FALSE(runtime.EnterGame(level, levelInstance, scene, error));
    EXPECT_EQ(runtime.GetBindingCount(), 0u);
    EXPECT_FALSE(runtime.IsActive());
}

TEST_F(LevelPhysicsRuntime, FailedSecondBodyCreationRollsBackEarlierBody)
{
    auto invalidNode = DynamicNode();
    invalidNode.transform.position.y = std::numeric_limits<float>::infinity();
    level.nodes = { StaticNode(), invalidNode };

    EXPECT_FALSE(runtime.EnterGame(level, levelInstance, scene, error));
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);

    const physics::PhysicsBodyHandle handle = world.CreateBody({
        .shape = physics::SphereShapeDescriptor{ .radius = 1.0f },
        .motionType = physics::PhysicsMotionType::Dynamic
    });
    EXPECT_TRUE(world.IsBodyValid(handle));
    EXPECT_TRUE(world.DestroyBody(handle));
}

TEST_F(LevelPhysicsRuntime, DynamicRotationSynchronizesAndLeaveGameRestoresAuthoredTransform)
{
    level.nodes = { DynamicNode() };
    level.nodes[0].transform.rotationDegrees = { 10.0f, 20.0f, 30.0f };
    const mathUtils::Vec3 authoredPosition = level.nodes[0].transform.position;
    const mathUtils::Vec3 authoredRotation = level.nodes[0].transform.rotationDegrees;
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    const mathUtils::Vec3 simulatedRotation{ -25.0f, 40.0f, 70.0f };
    ASSERT_TRUE(runtime.RequestDynamicTeleport(0, {
        .position = { 2.0f, 3.0f, 4.0f },
        .rotationQuaternion = mathUtils::EulerDegreesZYXToQuat(simulatedRotation)
    }));
    ASSERT_TRUE(runtime.SynchronizeBeforePhysics(level, error)) << error;
    ASSERT_TRUE(runtime.SynchronizeAfterPhysics(level, levelInstance, scene, error)) << error;
    EXPECT_TRUE(OrientationsEquivalent(
        level.nodes[0].transform.rotationDegrees,
        mathUtils::EulerDegreesZYXToQuat(simulatedRotation)));
    EXPECT_TRUE(runtime.LeaveGame(level, levelInstance, scene, error)) << error;
    EXPECT_EQ(level.nodes[0].transform.position, authoredPosition);
    EXPECT_EQ(level.nodes[0].transform.rotationDegrees, authoredRotation);
    EXPECT_EQ(levelInstance.GetNodeWorldPosition(0), authoredPosition);
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);
}

TEST_F(LevelPhysicsRuntime, KinematicSynchronizationForwardsRotation)
{
    level.nodes = { KinematicBoxNode() };
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    level.nodes[0].transform.rotationDegrees.z = 90.0f;
    ASSERT_TRUE(runtime.SynchronizeBeforePhysics(level, error)) << error;
    ASSERT_EQ(world.Update(1.0f / 60.0f), 1u);

    const auto hit = world.RayCastClosest({
        .origin = { 0.0f, 3.0f, 0.0f },
        .direction = { 0.0f, -1.0f, 0.0f },
        .maxDistance = 4.0f
    });
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->position.y, 2.0f, 1.0e-3f);
}

TEST_F(LevelPhysicsRuntime, ShutdownDestroysBindingsAndLeavesWorldUsable)
{
    level.nodes = { DynamicNode() };
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    runtime.Shutdown();
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);

    const physics::PhysicsBodyHandle handle = world.CreateBody({
        .shape = physics::SphereShapeDescriptor{ .radius = 1.0f },
        .motionType = physics::PhysicsMotionType::Dynamic
    });
    EXPECT_TRUE(world.IsBodyValid(handle));
    EXPECT_TRUE(world.DestroyBody(handle));
}