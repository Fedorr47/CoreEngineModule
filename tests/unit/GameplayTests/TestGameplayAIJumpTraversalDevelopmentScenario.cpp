#include <gtest/gtest.h>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    void AddNode(LevelAsset& level, const char* name, const mathUtils::Vec3 position)
    {
        LevelNode node{};
        node.name = name;
        node.alive = true;
        node.visible = true;
        node.transform.position = position;
        level.nodes.push_back(node);
    }

    LevelAsset MakeLevel()
    {
        LevelAsset level{};
        AddNode(level, "JumpTraversalAgent", {0.0f, 0.0f, -6.0f});
        AddNode(level, "JumpRouteStart", {0.0f, 0.0f, -6.0f});
        AddNode(level, "JumpTraversalEntry", {0.0f, 0.0f, -2.0f});
        AddNode(level, "JumpTakeoff", {0.0f, 0.0f, -0.35f});
        AddNode(level, "JumpLanding", {0.0f, 0.0f, 1.35f});
        AddNode(level, "JumpPostLanding", {0.0f, 0.0f, 4.0f});
        AddNode(level, "JumpRouteFinish", {0.0f, 0.0f, 7.5f});
        return level;
    }

    GameplayUpdateContext EnterGame(GameplayRuntime& runtime, LevelAsset& level,
        LevelInstance& instance, Scene& scene)
    {
        GameplayUpdateContext context{.mode = GameplayRuntimeMode::Game, .levelAsset = &level,
            .levelInstance = &instance, .scene = &scene};
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(context);
        runtime.PostPhysicsUpdate(context);
        return context;
    }
}

TEST(GameplayAIJumpTraversalDevelopmentScenario, DetectsOnlyCompleteNamedScenario)
{
    LevelAsset level = MakeLevel();
    EXPECT_TRUE(IsGameplayAIJumpTraversalDevelopmentScenario(level));
    level.nodes.back().alive = false;
    EXPECT_FALSE(IsGameplayAIJumpTraversalDevelopmentScenario(level));
}

TEST(GameplayAIJumpTraversalDevelopmentScenario, StartRegistersAuthoredLinkAndResetAllowsRestart)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level = MakeLevel();
    LevelInstance instance{};
    Scene scene{};
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    const GameplayUpdateContext context = EnterGame(runtime, level, instance, scene);
    GameplayAIJumpTraversalDevelopmentScenarioState state{};
    PrepareGameplayAIJumpTraversalDevelopmentScenario(state, level);

    ASSERT_EQ(StartGameplayAIJumpTraversalDevelopmentScenario(runtime, context),
        AIActionExecutionStatus::Running);
    const auto link = runtime.FindGameplayTraversalLink(kAIJumpTraversalDevelopmentLinkHandle);
    ASSERT_TRUE(link.has_value());
    EXPECT_EQ(link->traversalTypeId, kJumpTraversalTypeId);
    EXPECT_EQ(link->jump.takeoffPosition, mathUtils::Vec3(0.0f, 0.0f, -0.35f));
    EXPECT_EQ(link->jump.landingPosition, mathUtils::Vec3(0.0f, 0.0f, 1.35f));
    EXPECT_FLOAT_EQ(link->jump.verticalSpeed, 5.5f);
    EXPECT_FLOAT_EQ(link->jump.takeoffTolerance, 0.20f);
    EXPECT_FLOAT_EQ(link->jump.landingHorizontalTolerance, 0.55f);
    EXPECT_FLOAT_EQ(link->jump.landingVerticalTolerance, 0.30f);

    const EntityHandle agent = runtime.GetNodeBoundEntities().front();
    GameplayWorld& world = runtime.GetWorld();
    world.TryGetTransform(agent)->position = {3.0f, 2.0f, 1.0f};
    level.nodes.front().transform.position = {99.0f, 99.0f, 99.0f};
    GameplayActionComponent* action = world.TryGetAction(agent);
    ASSERT_NE(action, nullptr);
    action->current = GameplayActionKind::Jump;
    action->pending = {.kind = GameplayActionKind::Jump, .source = GameplayActionRequestSource::Script, .priority = 200};
    action->buffered = {.kind = GameplayActionKind::LightAttack, .source = GameplayActionRequestSource::Combat, .priority = 10};
    action->busy = true;
    action->pendingDispatched = true;

    ASSERT_EQ(ResetGameplayAIJumpTraversalDevelopmentScenario(runtime, level, state), agent);
    ASSERT_NE(agent, kNullEntity);
    EXPECT_FALSE(runtime.FindGameplayTraversalLink(kAIJumpTraversalDevelopmentLinkHandle).has_value());
    EXPECT_EQ(runtime.GetAIActionStatus(agent), AIActionExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(agent)->position, mathUtils::Vec3(0.0f, 0.0f, -6.0f));
    EXPECT_EQ(action->current, GameplayActionKind::None);
    EXPECT_FALSE(HasGameplayPendingActionRequest(*action));
    EXPECT_FALSE(HasGameplayBufferedActionRequest(*action));
    EXPECT_FALSE(action->busy);
    EXPECT_FALSE(action->pendingDispatched);
    EXPECT_EQ(StartGameplayAIJumpTraversalDevelopmentScenario(runtime, context),
        AIActionExecutionStatus::Running);
    runtime.Shutdown();
}

TEST(GameplayAIJumpTraversalDevelopmentScenario, RouteStartsTraversalAtEntryButKeepsPreciseTakeoffMetadata)
{
    LevelAsset level = MakeLevel();
    const auto route = BuildGameplayAIJumpTraversalDevelopmentRoute(level);
    ASSERT_TRUE(route.has_value());
    ASSERT_EQ(route->points.size(), 5u);
    ASSERT_EQ(route->segmentAnnotations.size(), 4u);
    EXPECT_EQ(route->points[0].worldPosition, mathUtils::Vec3(0.0f, 0.0f, -6.0f));
    EXPECT_EQ(route->points[1].worldPosition, mathUtils::Vec3(0.0f, 0.0f, -2.0f));
    EXPECT_EQ(route->points[2].worldPosition, mathUtils::Vec3(0.0f, 0.0f, 1.35f));
    EXPECT_FALSE(route->segmentAnnotations[0].traversalLink.has_value());
    ASSERT_TRUE(route->segmentAnnotations[1].traversalLink.has_value());
    EXPECT_EQ(*route->segmentAnnotations[1].traversalLink, kAIJumpTraversalDevelopmentLinkHandle);
    EXPECT_NE(route->points[1].worldPosition, level.nodes[3].transform.position);
}