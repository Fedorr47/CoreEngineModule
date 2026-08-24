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
    
    rendern::EntityHandle FindNodeEntity(rendern::GameplayRuntime& runtime,
        const rendern::LevelAsset& level, const std::string_view name)
    {
        for (const rendern::EntityHandle entity : runtime.GetNodeBoundEntities())
        {
            const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
            if (link != nullptr && link->nodeIndex >= 0 &&
                level.nodes[static_cast<std::size_t>(link->nodeIndex)].name == name)
            {
                return entity;
            }
        }
        return rendern::kNullEntity;
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
    EXPECT_EQ(detect(goap).first, appDevelopment::ScenarioKind::None);

    rendern::LevelAsset agentSize{}; agentSize.name = "NavigationSmallLargePassage";
    AddNode(agentSize, "SMALL NPC"); AddNode(agentSize, "LARGE NPC");
    // Historical node names and level names are not an activation mechanism.
    EXPECT_EQ(detect(agentSize).first, appDevelopment::ScenarioKind::None);

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

TEST(AppDevelopmentScenarioRuntime, LoadsAuthoredAccessKeyAndResetRecreatesCleanDecision)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_goap_access_key_development.level.json");
    rendern::LevelInstance instance = harness.Instantiate(level);
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    auto context = MakeContext(runtime, level, instance, harness.GetScene());
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
    EXPECT_EQ(development.GetView(context).title,
        std::string_view("Stage 7 GOAP: Physical Coin Pickups"));

    context.gameplayMode = rendern::GameplayRuntimeMode::Game;
    rendern::GameplayUpdateContext gameContext{.mode=rendern::GameplayRuntimeMode::Game,
        .levelAsset=&level, .levelInstance=&instance, .scene=&harness.GetScene()};
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    runtime.PostPhysicsUpdate(gameContext);
    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    const rendern::EntityHandle agent=FindNodeEntity(runtime,level,"GOAP_Agent");
    const rendern::EntityHandle coinAEntity=FindNodeEntity(runtime,level,"GOAP_Coin_A");
    ASSERT_NE(agent,rendern::kNullEntity);
    ASSERT_NE(coinAEntity,rendern::kNullEntity);
    auto* coinA=runtime.GetWorld().TryGetPickup(coinAEntity);
    ASSERT_NE(coinA,nullptr);
    const int coinANode=runtime.GetWorld().TryGetNodeLink(coinAEntity)->nodeIndex;
    auto* agentTransform=runtime.GetWorld().TryGetTransform(agent);
    ASSERT_NE(agentTransform,nullptr);
    const auto coinAPosition=runtime.GetWorld().TryGetTransform(coinAEntity)->position;
    const rendern::AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    
    // Proximity alone is not observation: suppress pickup production at Coin A.
    coinA->collected=true;
    agentTransform->position=coinAPosition;
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    EXPECT_TRUE(runtime.GetCurrentWorldEvents().empty());
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPCoinACollectedFact));

    coinA->collected=false;
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    ASSERT_EQ(runtime.GetCurrentWorldEvents().size(),1u);
    EXPECT_EQ(runtime.GetCurrentWorldEvents()[0].instigator,agent);
    EXPECT_EQ(runtime.GetCurrentWorldEvents()[0].subject,coinAEntity);
    EXPECT_TRUE(coinA->collected);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_TRUE(facts->IsFactSet(rendern::kGOAPCoinACollectedFact));
    
    development.Reset(context);
    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_FALSE(coinA->collected);
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_TRUE(runtime.GetCurrentWorldEvents().empty());
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPHasAccessKeyFact));
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPAtDestinationFact));
    runtime.Shutdown();
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

TEST(AppDevelopmentScenarioRuntime, LoadsNavigationAgentSizeOnlyFromExplicitScenarioReference)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/navigation_small_large_passage.level.json");
    ASSERT_EQ(level.developmentScenario, "development/navigation_agent_size.scenario.json");
    rendern::LevelInstance instance{};
    rendern::Scene scene{};
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    auto context = MakeContext(runtime, level, instance, scene);
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
    EXPECT_STREQ(development.GetView(context).title, "CR-445 Navigation Agent Size");
    EXPECT_FALSE(development.GetView(context).canStart);


    rendern::EntityHandle small{rendern::kNullEntity};
    rendern::EntityHandle large{rendern::kNullEntity};
    for (const auto entity : runtime.GetNodeBoundEntities())
    {
        const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
        if (link != nullptr && link->nodeIndex == 1)
        {
            small = entity;
        }
        if (link != nullptr && link->nodeIndex == 2)
        {
            large = entity;
        }
    }
    
    ASSERT_NE(small, rendern::kNullEntity);
    ASSERT_NE(large, rendern::kNullEntity);
    ASSERT_NE(runtime.GetWorld().TryGetCharacterPhysicalSettings(small), nullptr);
    ASSERT_NE(runtime.GetWorld().TryGetCharacterPhysicalSettings(large), nullptr);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetCharacterPhysicalSettings(small)->radius, 0.2f);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetCharacterPhysicalSettings(large)->radius, 0.7f);
    EXPECT_EQ(development.GetView(context).statusCount, 2u);
    EXPECT_STREQ(development.GetView(context).statuses[0].value, "NotStarted");
    EXPECT_STREQ(development.GetView(context).statuses[1].value, "NotStarted");
    development.Reset(context);
    
    runtime.Shutdown();
}