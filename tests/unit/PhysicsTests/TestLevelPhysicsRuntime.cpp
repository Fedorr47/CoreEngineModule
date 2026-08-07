#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"
#include "Physics/LevelPhysicsRuntime.h"

#include <string>
#include <type_traits>
#include <limits>

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

TEST_F(LevelPhysicsRuntime, EnterGameRejectsRotatedPhysicsNode)
{
    auto node = DynamicNode();
    node.transform.rotationDegrees.y = 1.0f;
    level.nodes = { node };
    EXPECT_FALSE(runtime.EnterGame(level, levelInstance, scene, error));
    EXPECT_NE(error.find("zero authored rotation"), std::string::npos);
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);
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

TEST_F(LevelPhysicsRuntime, LeaveGameRestoresAuthoredPosition)
{
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;
    level.nodes = { DynamicNode() };
    const mathUtils::Vec3 authoredPosition = level.nodes[0].transform.position;
    ASSERT_TRUE(runtime.EnterGame(level, levelInstance, scene, error)) << error;
    for (int stepIndex = 0; stepIndex < 20; ++stepIndex)
    {
        ASSERT_EQ(world.Update(FixedDeltaSeconds), 1u);
    }
    ASSERT_TRUE(runtime.Synchronize(level, levelInstance, scene, error)) << error;
    EXPECT_LT(level.nodes[0].transform.position.y, authoredPosition.y);
    EXPECT_TRUE(runtime.LeaveGame(level, levelInstance, scene, error)) << error;
    EXPECT_EQ(level.nodes[0].transform.position, authoredPosition);
    EXPECT_EQ(levelInstance.GetNodeWorldPosition(0), authoredPosition);
    EXPECT_FALSE(runtime.IsActive());
    EXPECT_EQ(runtime.GetBindingCount(), 0u);
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