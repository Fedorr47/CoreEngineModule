#include <gtest/gtest.h>

#include <utility>

#include "TestSupport/TestThreadAffinity.h"

import core;

#include "App/Development/AppDevelopmentScenarioRuntime.h"

namespace
{
    void AddNode(rendern::LevelAsset& level, const char* name)
    {
        rendern::LevelNode node{};
        node.name = name; node.alive = true; node.visible = true;
        level.nodes.push_back(std::move(node));
    }

    appDevelopment::ScenarioContext MakeContext(
    rendern::GameplayRuntime& runtime,
    rendern::LevelAsset& level,
    rendern::LevelInstance& instance,
    rendern::Scene& scene)
    {
        return {
            runtime,
            level,
            instance,
            scene,
            rendern::GameplayRuntimeMode::Editor,
            nullptr,
            nullptr
        };
    }
}

TEST(AppDevelopmentScenarioRuntime, DetectsSupportedScenarioConventionsAndCapabilities)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelInstance instance{}; rendern::Scene scene{};

    auto detect = [&](rendern::LevelAsset& level) {
        rendern::GameplayRuntime runtime{}; runtime.Initialize(level, instance, scene);
        auto context = MakeContext(runtime, level, instance, scene);
        appDevelopment::AppDevelopmentScenarioRuntime development{};
        development.OnLevelLoaded(context);
        const auto kind = development.GetActiveKind();
        const auto view = development.GetView(context);
        runtime.Shutdown();
        return std::pair{kind, view};
    };

    rendern::LevelAsset movement{};
    AddNode(movement, "AI_Move_Agent"); AddNode(movement, "AI_Move_Point_0"); AddNode(movement, "AI_Move_Point_1");
    EXPECT_EQ(detect(movement).first, appDevelopment::ScenarioKind::AIMovement);
    const auto movementView = detect(movement).second;
    EXPECT_TRUE(movementView.canReset);
    EXPECT_STREQ(movementView.resetLabel, "Reset Route");

    rendern::LevelAsset step{};
    AddNode(step, "NPC_Step_Start"); AddNode(step, "RouteTarget");
    const auto [stepKind, stepView] = detect(step);
    EXPECT_EQ(stepKind, appDevelopment::ScenarioKind::AIPhysicsStep);
    EXPECT_TRUE(stepView.canStart);
    EXPECT_TRUE(stepView.active);
    EXPECT_TRUE(stepView.canReset);
    EXPECT_TRUE(stepView.canStop);
    EXPECT_FALSE(stepView.commandsEnabled);
    
    rendern::LevelAsset jump{};
    AddNode(jump, "JumpTraversalAgent"); AddNode(jump, "JumpRouteStart");
    AddNode(jump, "JumpTraversalEntry");
    AddNode(jump, "JumpTakeoff"); AddNode(jump, "JumpLanding");
    AddNode(jump, "JumpPostLanding"); AddNode(jump, "JumpRouteFinish");
    const auto [jumpKind, jumpView] = detect(jump);
    EXPECT_EQ(jumpKind, appDevelopment::ScenarioKind::AIJumpTraversal);
    EXPECT_STREQ(jumpView.startLabel, "Start Route");
    EXPECT_TRUE(jumpView.canStart);
    EXPECT_TRUE(jumpView.canReset);
    EXPECT_TRUE(jumpView.canStop);

    rendern::LevelAsset agentSize{}; agentSize.name = "NavigationSmallLargePassage";
    AddNode(agentSize, "SMALL NPC"); AddNode(agentSize, "LARGE NPC");
    EXPECT_EQ(detect(agentSize).first, appDevelopment::ScenarioKind::NavigationAgentSize);
    const auto agentSizeView = detect(agentSize).second;
    EXPECT_TRUE(agentSizeView.canReset);
    EXPECT_STREQ(agentSizeView.resetLabel, "Reset Scenario");

    rendern::LevelAsset none{};
    EXPECT_EQ(detect(none).first, appDevelopment::ScenarioKind::None);
}

TEST(AppDevelopmentScenarioRuntime, OwnsAndResetsAgentSizeSetup)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{}; level.name = "NavigationSmallLargePassage";
    AddNode(level, "ScenarioObserver_Player"); AddNode(level, "SMALL NPC"); AddNode(level, "LARGE NPC");
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    rendern::GameplayUpdateContext gameplayContext{};
    gameplayContext.mode = rendern::GameplayRuntimeMode::Editor;
    gameplayContext.levelAsset = &level;
    gameplayContext.levelInstance = &instance;
    gameplayContext.scene = &scene;
    ASSERT_NE(runtime.SpawnNodeBoundEntity(gameplayContext, 0, true), rendern::kNullEntity);

    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    const rendern::GameplayWorld& world = runtime.GetWorld();
    rendern::EntityHandle small{rendern::kNullEntity}, large{rendern::kNullEntity};
    for (const auto entity : runtime.GetNodeBoundEntities())
    {
        const auto* link = world.TryGetNodeLink(entity);
        if (link && link->nodeIndex == 1) small = entity;
        if (link && link->nodeIndex == 2) large = entity;
    }
    ASSERT_NE(small, rendern::kNullEntity); ASSERT_NE(large, rendern::kNullEntity);
    EXPECT_FALSE(world.HasPlayerControlled(small)); EXPECT_FALSE(world.HasPlayerControlled(large));
    EXPECT_TRUE(world.HasAI(small)); EXPECT_TRUE(world.HasAI(large));
    ASSERT_NE(world.TryGetCharacterPhysicalSettings(small), nullptr);
    ASSERT_NE(world.TryGetCharacterPhysicalSettings(large), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetCharacterPhysicalSettings(small)->radius, 0.2f);
    EXPECT_FLOAT_EQ(world.TryGetCharacterPhysicalSettings(large)->radius, 0.7f);

    development.Reset();
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::None);
    EXPECT_FALSE(development.GetView(context).active);
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, AdaptsAIMovementStartStopAndAuthoritativeStatus)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    AddNode(level, "AI_Move_Agent");
    AddNode(level, "AI_Move_Point_0");
    level.nodes.back().transform.position = {-1.0f, 0.0f, 0.0f};
    AddNode(level, "AI_Move_Point_1");
    level.nodes.back().transform.position = {1.0f, 0.0f, 0.0f};
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);

    context.gameplayMode = rendern::GameplayRuntimeMode::Game;
    rendern::GameplayUpdateContext gameplayContext{};
    gameplayContext.mode = context.gameplayMode;
    gameplayContext.levelAsset = &level;
    gameplayContext.levelInstance = &instance;
    gameplayContext.scene = &scene;
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameplayContext); runtime.PostPhysicsUpdate(gameplayContext);

    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    ASSERT_STREQ(development.GetView(context).statuses[0].value, "Running");
    auto* transform = runtime.GetWorld().TryGetTransform(runtime.GetNodeBoundEntities().back());
    ASSERT_NE(transform, nullptr);
    transform->position = {6.0f, 0.0f, 0.0f};
    development.Execute(appDevelopment::ScenarioCommand::Stop, context);
    EXPECT_STREQ(development.GetView(context).statuses[0].value, "Cancelled");
    EXPECT_FLOAT_EQ(transform->position.x, 6.0f);
    development.Execute(appDevelopment::ScenarioCommand::Reset, context);
    EXPECT_STREQ(development.GetView(context).statuses[0].value, "NotStarted");
    EXPECT_FLOAT_EQ(transform->position.x, -1.0f);
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, AgentSizeResetRestoresLevelTransformsAndPhysicalSettings)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{}; level.name = "NavigationSmallLargePassage";
    AddNode(level, "SMALL NPC"); level.nodes.back().transform.position = {-4.0f, 0.0f, 1.0f};
    AddNode(level, "LARGE NPC"); level.nodes.back().transform.position = {-4.0f, 0.0f, -1.0f};
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    rendern::EntityHandle small{rendern::kNullEntity};
    rendern::EntityHandle large{rendern::kNullEntity};
    for (const auto entity : runtime.GetNodeBoundEntities())
    {
        const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
        if (link && link->nodeIndex == 0) small = entity;
        if (link && link->nodeIndex == 1) large = entity;
    }
    ASSERT_NE(small, rendern::kNullEntity); ASSERT_NE(large, rendern::kNullEntity);
    runtime.GetWorld().TryGetTransform(small)->position = {10.0f, 0.0f, 0.0f};
    runtime.GetWorld().TryGetTransform(large)->position = {20.0f, 0.0f, 0.0f};
    context.gameplayMode = rendern::GameplayRuntimeMode::Game;
    const auto gameViewWithoutNavigation = development.GetView(context);
    EXPECT_TRUE(gameViewWithoutNavigation.commandsEnabled);
    EXPECT_FALSE(gameViewWithoutNavigation.canStart);
    EXPECT_TRUE(gameViewWithoutNavigation.canReset);
    rendern::GameplayUpdateContext gameContext{.mode = context.gameplayMode, .levelAsset = &level,
        .levelInstance = &instance, .scene = &scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameContext); runtime.PostPhysicsUpdate(gameContext);
    development.Execute(appDevelopment::ScenarioCommand::Reset, context);
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(small)->position, level.nodes[0].transform.position);
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(large)->position, level.nodes[1].transform.position);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetCharacterPhysicalSettings(small)->radius, 0.2f);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetCharacterPhysicalSettings(large)->radius, 0.7f);
    const auto view = development.GetView(context);
    EXPECT_STREQ(view.statuses[0].value, "NotStarted");
    EXPECT_STREQ(view.statuses[1].value, "NotStarted");
    runtime.Shutdown();
}