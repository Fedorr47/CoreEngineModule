#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

#include "TestSupport/TestThreadAffinity.h"

import core;

#include "TestSupport/AccessKeyAssetSymbols.h"

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
    
    void TickFrame(rendern::GameplayRuntime& runtime,
        appDevelopment::AppDevelopmentScenarioRuntime& development,
        appDevelopment::ScenarioContext& scenarioContext,
        const rendern::GameplayUpdateContext& gameContext)
    {
        // Match AppLifecycle: scenario mutations run before the gameplay update.
        development.Update(scenarioContext);
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(gameContext);
        runtime.PostPhysicsUpdate(gameContext);
    }

    template<typename Predicate>
    bool TickUntil(rendern::GameplayRuntime& runtime,
        appDevelopment::AppDevelopmentScenarioRuntime& development,
        appDevelopment::ScenarioContext& scenarioContext,
        const rendern::GameplayUpdateContext& gameContext,
        Predicate predicate,
        const std::size_t maxFrames)
    {
        for (std::size_t frame=0;frame<maxFrames;++frame)
        {
            TickFrame(runtime,development,scenarioContext,gameContext);
            if (predicate())
            {
                return true;
            }
        }
        return false;
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
    rendern::GameplayRuntime runtime{
        rendern::MakeDefaultGameplayAIDecisionFactories()};
    runtime.Initialize(level, instance, harness.GetScene());
    auto context = MakeContext(runtime, level, instance, harness.GetScene());
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetActiveKind(), appDevelopment::ScenarioKind::DataDriven);
    EXPECT_EQ(development.GetView(context).title,
    std::string_view("Stage 9B GOAP: Route-Aware Access-Key Planning"));

    context.gameplayMode = rendern::GameplayRuntimeMode::Game;
    rendern::GameplayUpdateContext gameContext{.mode=rendern::GameplayRuntimeMode::Game,
        .levelAsset=&level, .levelInstance=&instance, .scene=&harness.GetScene()};
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    runtime.PostPhysicsUpdate(gameContext);
    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    const rendern::EntityHandle agent=FindNodeEntity(runtime,level,"GOAP_Agent");
    const rendern::EntityHandle coinAEntity=FindNodeEntity(runtime,level,"GOAP_Coin_A");
    const rendern::EntityHandle keyEntity=FindNodeEntity(runtime,level,"GOAP_Access_Key");
    ASSERT_NE(agent,rendern::kNullEntity);
    ASSERT_NE(coinAEntity,rendern::kNullEntity);
    ASSERT_NE(keyEntity,rendern::kNullEntity);
    auto* coinA=runtime.GetWorld().TryGetPickup(coinAEntity);
    ASSERT_NE(coinA,nullptr);
    const int coinANode=runtime.GetWorld().TryGetNodeLink(coinAEntity)->nodeIndex;
    const int keyNode=runtime.GetWorld().TryGetNodeLink(keyEntity)->nodeIndex;
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(keyNode));
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
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));

    coinA->collected=false;
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    ASSERT_EQ(runtime.GetCurrentWorldEvents().size(),1u);
    EXPECT_EQ(runtime.GetCurrentWorldEvents()[0].instigator,agent);
    EXPECT_EQ(runtime.GetCurrentWorldEvents()[0].subject,coinAEntity);
    EXPECT_TRUE(coinA->collected);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    
    development.Execute(appDevelopment::ScenarioCommand::Reset, context);
    development.Execute(appDevelopment::ScenarioCommand::Start, context);
    facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_FALSE(coinA->collected);
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(keyNode));
    EXPECT_TRUE(runtime.GetCurrentWorldEvents().empty());
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact), 0);
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPHasAccessKeyFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPAtDestinationFact));
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, Stage10ReplansAfterPlannedCoinBecomesUnavailableAndReachesGoal)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelAsset level=rendern::LoadLevelAssetFromJson(
        "levels/ai_goap_access_key_replanning_development.level.json");
    EXPECT_EQ(level.developmentScenario,
        "development/ai_goap_access_key_replanning.scenario.json");
    rendern::LevelInstance instance=harness.Instantiate(level);
    rendern::GameplayRuntime runtime{
        rendern::MakeDefaultGameplayAIDecisionFactories()};
    runtime.Initialize(level,instance,harness.GetScene());
    auto context=MakeContext(runtime,level,instance,harness.GetScene());
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetActiveKind(),appDevelopment::ScenarioKind::DataDriven);
    EXPECT_EQ(development.GetView(context).title,
        std::string_view("Stage 10 GOAP: Dynamic Access-Key Replanning"));

    context.gameplayMode=rendern::GameplayRuntimeMode::Game;
    rendern::GameplayUpdateContext gameContext{.deltaSeconds=1.0f/60.0f,
        .mode=rendern::GameplayRuntimeMode::Game,
        .levelAsset=&level,.levelInstance=&instance,.scene=&harness.GetScene()};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameContext); runtime.PostPhysicsUpdate(gameContext);
    development.Execute(appDevelopment::ScenarioCommand::Start,context);
    const rendern::EntityHandle agent=FindNodeEntity(runtime,level,"GOAP_Agent");
    const rendern::EntityHandle coinAEntity=FindNodeEntity(runtime,level,"GOAP_Coin_A");
    const rendern::EntityHandle coinBEntity=FindNodeEntity(runtime,level,"GOAP_Coin_B");
    const rendern::EntityHandle coinCEntity=FindNodeEntity(runtime,level,"GOAP_Coin_C");
    const rendern::EntityHandle keyEntity=FindNodeEntity(runtime,level,"GOAP_Access_Key");
    const auto goalNode=std::ranges::find_if(level.nodes,
        [](const rendern::LevelNode& node) { return node.alive && node.name=="GOAP_Final_Goal"; });
    ASSERT_NE(agent,rendern::kNullEntity); ASSERT_NE(coinAEntity,rendern::kNullEntity);
    ASSERT_NE(coinBEntity,rendern::kNullEntity); ASSERT_NE(coinCEntity,rendern::kNullEntity);
    ASSERT_NE(keyEntity,rendern::kNullEntity); ASSERT_NE(goalNode,level.nodes.end());
    rendern::GameplayPickupComponent* coinA=runtime.GetWorld().TryGetPickup(coinAEntity);
    rendern::GameplayPickupComponent* coinB=runtime.GetWorld().TryGetPickup(coinBEntity);
    rendern::GameplayPickupComponent* coinC=runtime.GetWorld().TryGetPickup(coinCEntity);
    ASSERT_NE(coinA,nullptr); ASSERT_NE(coinB,nullptr); ASSERT_NE(coinC,nullptr);
    EXPECT_FALSE(coinA->collected); EXPECT_FALSE(coinB->collected); EXPECT_FALSE(coinC->collected);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),rendern::AIPlanExecutionStatus::ReadyToStartStep);
    const rendern::AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinAAvailableFact));
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinBAvailableFact));
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinCAvailableFact));
    const int coinANode=runtime.GetWorld().TryGetNodeLink(coinAEntity)->nodeIndex;
    const int coinBNode=runtime.GetWorld().TryGetNodeLink(coinBEntity)->nodeIndex;
    const int coinCNode=runtime.GetWorld().TryGetNodeLink(coinCEntity)->nodeIndex;
    const int keyNode=runtime.GetWorld().TryGetNodeLink(keyEntity)->nodeIndex;

    // The conditional update must remain a no-op while Start -> Coin C is active.
    development.Update(context);
    EXPECT_FALSE(coinA->collected); EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameContext); runtime.PostPhysicsUpdate(gameContext);
    runtime.GetWorld().TryGetTransform(agent)->position=
        runtime.GetWorld().TryGetTransform(coinCEntity)->position;
    ASSERT_TRUE(TickUntil(runtime,development,context,gameContext,[&]()
        { return facts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact); },8u))
        << "Coin C was not collected";
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),1);
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact));
    EXPECT_FALSE(coinA->collected); EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));

    TickFrame(runtime,development,context,gameContext);
    EXPECT_TRUE(coinA->collected); EXPECT_FALSE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinAAvailableFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),1);

    // Start the replacement C -> B step, then let physical pickup observation confirm B.
    TickFrame(runtime,development,context,gameContext);
    runtime.GetWorld().TryGetTransform(agent)->position=
        runtime.GetWorld().TryGetTransform(coinBEntity)->position;
    ASSERT_TRUE(TickUntil(runtime,development,context,gameContext,[&]()
        { return facts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact); },8u))
        << "Replacement route did not collect Coin B";
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),2);

    TickFrame(runtime,development,context,gameContext);
    runtime.GetWorld().TryGetTransform(agent)->position=
        runtime.GetWorld().TryGetTransform(keyEntity)->position;
    ASSERT_TRUE(TickUntil(runtime,development,context,gameContext,[&]()
        { return facts->IsFactSet(access_key_test::kGOAPHasAccessKeyFact); },8u))
        << "Access Key purchase did not complete";
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),0);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(keyNode));

    TickFrame(runtime,development,context,gameContext);
    runtime.GetWorld().TryGetTransform(agent)->position=goalNode->transform.position;
    ASSERT_TRUE(TickUntil(runtime,development,context,gameContext,[&]()
        { return facts->IsFactSet(access_key_test::kGOAPAtDestinationFact); },8u))
        << "Final goal did not complete";
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),rendern::AIPlanExecutionStatus::Succeeded);
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPHasAccessKeyFact));
    EXPECT_FALSE(facts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact));
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),0);

    development.Execute(appDevelopment::ScenarioCommand::Reset,context);
    EXPECT_FALSE(coinA->collected); EXPECT_FALSE(coinB->collected); EXPECT_FALSE(coinC->collected);
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinBNode));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinCNode));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(keyNode));
    development.Execute(appDevelopment::ScenarioCommand::Start,context);
    facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    development.Update(context);
    EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinAAvailableFact));
    EXPECT_FALSE(coinA->collected); EXPECT_TRUE(instance.IsNodeRuntimeVisible(coinANode));
    runtime.Shutdown();
}

TEST(AppDevelopmentScenarioRuntime, Stage11TwoAgentsResolveContestedCoinThroughReservationAndReplanning)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelAsset level=rendern::LoadLevelAssetFromJson(
        "levels/ai_goap_access_key_reservation_development.level.json");
    EXPECT_EQ(level.developmentScenario,
        "development/ai_goap_access_key_reservation.scenario.json");
    rendern::LevelInstance instance=harness.Instantiate(level);
    rendern::GameplayRuntime runtime{
        rendern::MakeDefaultGameplayAIDecisionFactories()};
    runtime.Initialize(level,instance,harness.GetScene());
    auto context=MakeContext(runtime,level,instance,harness.GetScene());
    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);
    EXPECT_EQ(development.GetView(context).title,
        std::string_view("Stage 11 GOAP: Multi-Agent Resource Reservation"));
    context.gameplayMode=rendern::GameplayRuntimeMode::Game;
    rendern::GameplayUpdateContext gameContext{.deltaSeconds=1.0f/60.0f,
        .mode=rendern::GameplayRuntimeMode::Game,.levelAsset=&level,
        .levelInstance=&instance,.scene=&harness.GetScene()};
    TickFrame(runtime,development,context,gameContext);
    development.Execute(appDevelopment::ScenarioCommand::Start,context);

    const rendern::EntityHandle agentA=FindNodeEntity(runtime,level,"GOAP_Agent_A");
    const rendern::EntityHandle agentB=FindNodeEntity(runtime,level,"GOAP_Agent_B");
    const rendern::EntityHandle coinA=FindNodeEntity(runtime,level,"GOAP_Coin_A");
    const rendern::EntityHandle coinB=FindNodeEntity(runtime,level,"GOAP_Coin_B");
    const rendern::EntityHandle coinC=FindNodeEntity(runtime,level,"GOAP_Coin_C");
    ASSERT_NE(agentA,rendern::kNullEntity); ASSERT_NE(agentB,rendern::kNullEntity);
    ASSERT_NE(agentA,agentB); ASSERT_NE(coinA,rendern::kNullEntity);
    ASSERT_NE(coinB,rendern::kNullEntity); ASSERT_NE(coinC,rendern::kNullEntity);
    const rendern::EntityHandle firstAgent=std::min(agentA,agentB);
    const rendern::EntityHandle secondAgent=std::max(agentA,agentB);
    ASSERT_LT(firstAgent,secondAgent);
    auto& world=runtime.GetWorld();
    for (const rendern::EntityHandle coin : {coinA,coinB,coinC})
    {
        ASSERT_NE(world.TryGetPickup(coin),nullptr);
        EXPECT_TRUE(world.HasInteractionPoint(coin));
        EXPECT_FALSE(world.TryGetPickup(coin)->collected);
        EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coin),rendern::kNullEntity);
    }
    const auto assertCleanFacts=[&](const rendern::EntityHandle agent)
    {
        const rendern::AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
        ASSERT_NE(facts,nullptr);
        EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinAAvailableFact));
        EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinBAvailableFact));
        EXPECT_TRUE(facts->IsFactSet(access_key_test::kGOAPCoinCAvailableFact));
        EXPECT_EQ(facts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),0);
        EXPECT_EQ(runtime.GetAIDecisionStatus(agent),
            rendern::AIPlanExecutionStatus::ReadyToStartStep);
    };
    assertCleanFacts(agentA); assertCleanFacts(agentB);
    const mathUtils::Vec3 agentABaseline=world.TryGetTransform(agentA)->position;
    const mathUtils::Vec3 agentBBaseline=world.TryGetTransform(agentB)->position;

    TickFrame(runtime,development,context,gameContext);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coinC),firstAgent);
    EXPECT_FALSE(world.TryGetPickup(coinC)->collected);
    const rendern::AIAgentWorldState* firstFacts=runtime.GetAIDecisionObservedState(firstAgent);
    const rendern::AIAgentWorldState* secondFacts=runtime.GetAIDecisionObservedState(secondAgent);
    ASSERT_NE(firstFacts,nullptr); ASSERT_NE(secondFacts,nullptr);
    EXPECT_TRUE(firstFacts->IsFactSet(access_key_test::kGOAPCoinCAvailableFact));
    EXPECT_FALSE(secondFacts->IsFactSet(access_key_test::kGOAPCoinCAvailableFact));
    EXPECT_FALSE(secondFacts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    EXPECT_EQ(secondFacts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),0);

    TickFrame(runtime,development,context,gameContext);
    const rendern::EntityHandle alternateCoin =
        runtime.GetGameplayObjectReservationOwner(coinA)==secondAgent ? coinA : coinB;
    ASSERT_NE(alternateCoin,coinC);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(alternateCoin),secondAgent);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coinC),firstAgent);

    world.TryGetTransform(firstAgent)->position=world.TryGetTransform(coinC)->position;
    world.TryGetTransform(secondAgent)->position=world.TryGetTransform(alternateCoin)->position;
    ASSERT_TRUE(TickUntil(runtime,development,context,gameContext,[&]()
    {
        return firstFacts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact) &&
            secondFacts->GetIntegerFact(access_key_test::kGOAPCoinCountFact)==1;
    },8u));
    EXPECT_EQ(firstFacts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),1);
    EXPECT_EQ(secondFacts->GetIntegerFact(access_key_test::kGOAPCoinCountFact),1);
    EXPECT_TRUE(firstFacts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    EXPECT_FALSE(secondFacts->IsFactSet(access_key_test::kGOAPCoinCCollectedFact));
    if (alternateCoin==coinA)
    {
        EXPECT_TRUE(secondFacts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
        EXPECT_FALSE(firstFacts->IsFactSet(access_key_test::kGOAPCoinACollectedFact));
    }
    else
    {
        EXPECT_TRUE(secondFacts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact));
        EXPECT_FALSE(firstFacts->IsFactSet(access_key_test::kGOAPCoinBCollectedFact));
    }
    EXPECT_TRUE(world.TryGetPickup(coinC)->collected);
    EXPECT_TRUE(world.TryGetPickup(alternateCoin)->collected);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coinC),rendern::kNullEntity);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(alternateCoin),rendern::kNullEntity);

    development.Execute(appDevelopment::ScenarioCommand::Stop,context);
    for (const rendern::EntityHandle coin : {coinA,coinB,coinC})
    {
        EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coin),rendern::kNullEntity);
    }
    development.Execute(appDevelopment::ScenarioCommand::Reset,context);
    for (const rendern::EntityHandle coin : {coinA,coinB,coinC})
    {
        EXPECT_FALSE(world.TryGetPickup(coin)->collected);
        EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coin),rendern::kNullEntity);
    }
    EXPECT_EQ(world.TryGetTransform(agentA)->position,agentABaseline);
    EXPECT_EQ(world.TryGetTransform(agentB)->position,agentBBaseline);
    development.Execute(appDevelopment::ScenarioCommand::Start,context);
    TickFrame(runtime,development,context,gameContext);
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coinC),firstAgent);
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

TEST(AppDevelopmentScenarioRuntime, LoadsAccessKeyJumpAndStartsProductionDecision)
{
    InlineThreadOwnerRolesGuard guard{};

    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_goap_access_key_jump_development.level.json");

    ASSERT_EQ(
        level.developmentScenario,
        "development/ai_goap_access_key_jump.scenario.json");

    rendern::LevelInstance instance = harness.Instantiate(level);

    rendern::GameplayRuntime runtime{
        rendern::MakeDefaultGameplayAIDecisionFactories()};

    runtime.Initialize(level, instance, harness.GetScene());

    auto context = MakeContext(
        runtime,
        level,
        instance,
        harness.GetScene());

    appDevelopment::AppDevelopmentScenarioRuntime development{};
    development.OnLevelLoaded(context);

    context.gameplayMode = rendern::GameplayRuntimeMode::Game;

    rendern::GameplayUpdateContext gameContext{
        .mode = rendern::GameplayRuntimeMode::Game,
        .levelAsset = &level,
        .levelInstance = &instance,
        .scene = &harness.GetScene()
    };

    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    runtime.PostPhysicsUpdate(gameContext);

    ASSERT_EQ(
        runtime.GetCurrentMode(),
        rendern::GameplayRuntimeMode::Game);

    development.Execute(
        appDevelopment::ScenarioCommand::Start,
        context);

    const rendern::EntityHandle agent =
        FindNodeEntity(runtime, level, "GOAP_Agent");

    ASSERT_NE(agent, rendern::kNullEntity);

    EXPECT_EQ(
        runtime.GetAIDecisionStatus(agent),
        rendern::AIPlanExecutionStatus::ReadyToStartStep);

    development.Execute(
        appDevelopment::ScenarioCommand::Stop,
        context);

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