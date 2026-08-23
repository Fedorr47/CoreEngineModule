#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

#include "TestSupport/TestThreadAffinity.h"

import core;

#include "App/Development/AppDevelopmentScenarioRuntime.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

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
    EXPECT_EQ(detect(movement).first, appDevelopment::ScenarioKind::None);

    rendern::LevelAsset step{};
    AddNode(step, "NPC_Step_Start"); AddNode(step, "RouteTarget");
    EXPECT_EQ(detect(step).first, appDevelopment::ScenarioKind::None);
    
    rendern::LevelAsset jump{};
    AddNode(jump, "JumpTraversalAgent"); AddNode(jump, "JumpRouteStart");
    AddNode(jump, "JumpTraversalEntry");
    AddNode(jump, "JumpTakeoff"); AddNode(jump, "JumpLanding");
    AddNode(jump, "JumpPostLanding"); AddNode(jump, "JumpRouteFinish");
    EXPECT_EQ(detect(jump).first, appDevelopment::ScenarioKind::None);
    
    rendern::LevelAsset goap{};
    AddNode(goap, "GOAP_Observer_Player"); AddNode(goap, "GOAP_Agent");
    AddNode(goap, "GOAP_Start"); AddNode(goap, "GOAP_Access_Key"); AddNode(goap, "GOAP_Final_Goal");
    EXPECT_EQ(detect(goap).first, appDevelopment::ScenarioKind::AIGOAPAccessKey);

    rendern::LevelAsset agentSize{}; agentSize.name = "NavigationSmallLargePassage";
    AddNode(agentSize, "SMALL NPC"); AddNode(agentSize, "LARGE NPC");
    EXPECT_EQ(detect(agentSize).first, appDevelopment::ScenarioKind::NavigationAgentSize);
    const auto agentSizeView = detect(agentSize).second;
    EXPECT_TRUE(agentSizeView.canReset);
    EXPECT_STREQ(agentSizeView.resetLabel, "Reset Scenario");

    rendern::LevelAsset none{};
    EXPECT_EQ(detect(none).first, appDevelopment::ScenarioKind::None);
}

TEST(AppDevelopmentScenarioRuntime, LoadsRemainingRouteScenariosOnlyAsDataDriven)
{
    InlineThreadOwnerRolesGuard guard{};
    for (const char* path : {"levels/demo.level.with_fsm_test.locomotion.phaseB.json",
                             "levels/ai_character_step_debug.level.json"})
    {
        rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(path);
        ASSERT_FALSE(level.developmentScenario.empty());
        rendern::test::LevelInstantiateHarness harness{};
        rendern::LevelInstance instance = harness.Instantiate(level);
        rendern::Scene& scene = harness.GetScene();
        rendern::GameplayRuntime runtime{};
        runtime.Initialize(level, instance, scene);
        auto context = MakeContext(runtime, level, instance, scene);
        appDevelopment::AppDevelopmentScenarioRuntime development{};
        development.OnLevelLoaded(context);
        EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
        context.gameplayMode = rendern::GameplayRuntimeMode::Game;
        rendern::GameplayUpdateContext gameContext{.mode=rendern::GameplayRuntimeMode::Game,
            .levelAsset=&level, .levelInstance=&instance, .scene=&scene};
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(gameContext);
        runtime.PostPhysicsUpdate(gameContext);
        development.Execute(appDevelopment::ScenarioCommand::Start, context);
        rendern::EntityHandle agent = rendern::kNullEntity;
        const std::string_view agentName = level.name == "DemoLevel" ? "AI_Move_Agent" : "NPC_Step_Start";
        const auto authoredAgent = std::ranges::find_if(level.nodes,
            [&](const rendern::LevelNode& node) { return node.name == agentName; });
        ASSERT_NE(authoredAgent, level.nodes.end());
        const int expectedNode = static_cast<int>(std::distance(level.nodes.begin(), authoredAgent));
        for (const auto entity : runtime.GetNodeBoundEntities())
            if (const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
                link && link->nodeIndex == expectedNode) agent = entity;
        ASSERT_NE(agent, rendern::kNullEntity);
        EXPECT_FALSE(runtime.GetWorld().HasPlayerControlled(agent));
        EXPECT_TRUE(runtime.GetWorld().HasAI(agent));
        EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Running);
        development.Execute(appDevelopment::ScenarioCommand::Stop, context);
        EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Cancelled);
        development.Reset(context);
        runtime.Shutdown();
    }
}

TEST(AppDevelopmentScenarioRuntime, LoadsActualJumpLevelOnlyAsDataDriven)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_jump_traversal_development.level.json");
    ASSERT_EQ(level.developmentScenario, "development/ai_jump_traversal.scenario.json");
    rendern::LevelInstance instance{};
    rendern::Scene scene{};
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
    EXPECT_STREQ(development.GetView(context).title, "CR-447 AI Jump Traversal");
    development.Reset(context);
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, ExplicitInvalidScenarioNeverFallsBackToLegacyDetection)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    AddNode(level, "AI_Move_Agent"); AddNode(level, "AI_Move_Point_0"); AddNode(level, "AI_Move_Point_1");
    level.developmentScenario = "tests/development/does_not_exist.scenario.json";
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    EXPECT_THROW(development.OnLevelLoaded(context), std::runtime_error);
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::None);
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, LoadsRunsAndStopsExplicitDataDrivenScenario)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson("tests/development/basic.level.json");
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelInstance instance = harness.Instantiate(level);
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{}; runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
    EXPECT_TRUE(development.GetView(context).active);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(1));

    context.gameplayMode = rendern::GameplayRuntimeMode::Game;
    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    EXPECT_STREQ(development.GetView(context).statuses[0].value, "Running");
    development.Update(context);
    context.gameplayMode = rendern::GameplayRuntimeMode::Editor;
    development.Update(context);
    EXPECT_STREQ(development.GetView(context).statuses[0].value, "Loaded");
    development.Reset(context);
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(1));
    runtime.Shutdown();
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