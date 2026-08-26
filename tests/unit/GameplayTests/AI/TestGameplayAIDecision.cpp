#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

import core;

using namespace rendern;

namespace
{
    GameplayUpdateContext Context(LevelAsset& level, LevelInstance& instance, Scene& scene,
        const GameplayRuntimeMode mode)
    {
        return {.deltaSeconds=1.0f/60.0f, .mode=mode, .levelAsset=&level,
            .levelInstance=&instance, .scene=&scene};
    }

    EntityHandle FindNodeEntity(GameplayRuntime& runtime, const LevelAsset& level,
        const std::string_view name)
    {
        for (const EntityHandle entity : runtime.GetNodeBoundEntities())
        {
            const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
            if (link != nullptr && link->nodeIndex >= 0 &&
                level.nodes[static_cast<std::size_t>(link->nodeIndex)].name == name)
            {
                return entity;
            }
        }
        return kNullEntity;
    }
    
    void Tick(GameplayRuntime& runtime, const GameplayUpdateContext& game)
    {
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(game);
    }
}

TEST(GameplayAIDecision, AccessKeyDefinitionPlansOneShotCoinsAndSemanticPurchase)
{
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph({0,0,0},
        {-6,0.35f,-3}, {6,0.35f,1}, {-5,0.35f,6}, {0,0.35f,-7}, {0,0.08f,10});
    const auto configuredDefinition = ai_access_key_detail::BuildDefinition(graph);
    ASSERT_TRUE(configuredDefinition.has_value());
    const GameplayGOAPDecisionDefinition& definition = *configuredDefinition;
    const auto buyKey = std::ranges::find_if(definition.actions,
        [](const AIActionDefinition& action) { return action.actionId == kAIBuyKeyActionId; });
    ASSERT_NE(buyKey, definition.actions.end());
    AIAgentWorldState atShop{};
    atShop.SetFact(kGOAPAtAccessKeyShopFact, true);
    for (std::int32_t coins = 0; coins < kAccessKeyPrice; ++coins)
    {
        atShop.SetIntegerFact(kGOAPCoinCountFact, coins);
        EXPECT_FALSE(AreNumericConditionsSatisfied(atShop, buyKey->numericPreconditions));
    }
    atShop.SetIntegerFact(kGOAPCoinCountFact, kAccessKeyPrice);
    EXPECT_TRUE(AreNumericConditionsSatisfied(atShop, buyKey->numericPreconditions));
    AIAgentWorldState awayFromShop = atShop;
    awayFromShop.SetFact(kGOAPAtAccessKeyShopFact, false);
    EXPECT_FALSE(AreFactConditionsSatisfied(awayFromShop, buyKey->preconditions));

    AIAgentWorldState initial{};
    initial.SetFact(kGOAPAtStartFact, true);
    initial.SetFact(kGOAPCoinAAvailableFact, true);
    initial.SetFact(kGOAPCoinBAvailableFact, true);
    initial.SetFact(kGOAPCoinCAvailableFact, true);
    const auto plan = FindAIPlan(initial, definition.goals.front().goal, definition.actions);
    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 5u);
    EXPECT_EQ(plan->steps[0], (AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC)}));
    EXPECT_EQ(plan->steps[1], (AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::CoinC,
        ai_access_key_detail::SpatialLocation::CoinA)}));
    EXPECT_EQ(plan->steps[2], (AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop)}));
    EXPECT_EQ(plan->steps[3].actionId, kAIBuyKeyActionId);
    EXPECT_EQ(plan->steps[4], (AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::AccessKeyShop,
            ai_access_key_detail::SpatialLocation::Goal)}));

    AIAgentWorldState predicted = initial;
    for (const AIPlanStep& step : plan->steps)
    {
        const auto action = std::ranges::find_if(definition.actions,
            [&](const AIActionDefinition& candidate)
            {
                return candidate.actionId == step.actionId && candidate.contextId == step.contextId;
            });
        ASSERT_NE(action, definition.actions.end());
        EXPECT_TRUE(AreFactConditionsSatisfied(predicted, action->preconditions));
        EXPECT_TRUE(AreNumericConditionsSatisfied(predicted, action->numericPreconditions));
        ApplyFactEffects(predicted, action->effects);
        EXPECT_TRUE(ApplyNumericEffects(predicted, action->numericEffects));
        const std::array spatialFacts{kGOAPAtStartFact, kGOAPAtCoinAFact,
            kGOAPAtCoinBFact, kGOAPAtCoinCFact, kGOAPAtAccessKeyShopFact,
            kGOAPAtGoalFact};
        EXPECT_EQ(std::ranges::count_if(spatialFacts, [&](const AIWorldFactId fact)
            { return predicted.IsFactSet(fact); }), 1);
    }
    EXPECT_TRUE(predicted.IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_FALSE(predicted.IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_TRUE(predicted.IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_TRUE(predicted.IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_TRUE(predicted.IsFactSet(kGOAPAtDestinationFact));
    EXPECT_EQ(predicted.GetIntegerFact(kGOAPCoinCountFact), 0);
}

TEST(GameplayAIDecision, AccessKeyReplansThroughRemainingAvailableCoin)
{
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph({0,0,0},
        {-6,0.35f,-3}, {6,0.35f,1}, {-5,0.35f,6}, {0,0.35f,-7}, {0,0.08f,10});
    const auto definition = ai_access_key_detail::BuildDefinition(graph);
    ASSERT_TRUE(definition.has_value());
    AIAgentWorldState observed{};
    observed.SetFact(kGOAPAtCoinCFact,true);
    observed.SetFact(kGOAPCoinCCollectedFact,true);
    observed.SetFact(kGOAPCoinAAvailableFact,false);
    observed.SetFact(kGOAPCoinBAvailableFact,true);
    observed.SetFact(kGOAPCoinCAvailableFact,false);
    observed.SetIntegerFact(kGOAPCoinCountFact,1);

    const auto plan=FindAIPlan(observed,definition->goals.front().goal,definition->actions);
    ASSERT_TRUE(plan.has_value()); ASSERT_EQ(plan->steps.size(),4u);
    EXPECT_EQ(plan->steps[0],(AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinB)}));
    EXPECT_EQ(plan->steps[1],(AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop)}));
    EXPECT_EQ(plan->steps[2].actionId,kAIBuyKeyActionId);
    EXPECT_EQ(plan->steps[3],(AIPlanStep{kAIMoveToActionId,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::AccessKeyShop,
            ai_access_key_detail::SpatialLocation::Goal)}));
}

TEST(GameplayAIDecision, AccessKeyMoveCostsComeFromHypotheticalRouteSource)
{
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph({0,0,0},
        {-6,0.35f,-3}, {6,0.35f,1}, {-5,0.35f,6}, {0,0.35f,-7}, {0,0.08f,10});
    const auto definition = ai_access_key_detail::BuildDefinition(graph);
    ASSERT_TRUE(definition.has_value());
    const auto cost = [&](const ai_access_key_detail::SpatialLocation source,
        const ai_access_key_detail::SpatialLocation target)
    {
        const auto action = std::ranges::find_if(definition->actions,
            [&](const AIActionDefinition& candidate)
            {
                return candidate.contextId == ai_access_key_detail::MoveContext(source, target);
            });
        EXPECT_NE(action, definition->actions.end());
        return action == definition->actions.end() ? -1.0f : action->baseCost;
    };
    EXPECT_NE(cost(ai_access_key_detail::SpatialLocation::Start,
                  ai_access_key_detail::SpatialLocation::CoinC),
        cost(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinC));
    EXPECT_NE(cost(ai_access_key_detail::SpatialLocation::Start,
                  ai_access_key_detail::SpatialLocation::CoinA), 1.0f);
    const float selectedCoinRoute =
        cost(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);
    
    const float startAToBRoute =
        cost(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);
    const float startAToCRoute =
    cost(ai_access_key_detail::SpatialLocation::Start,
        ai_access_key_detail::SpatialLocation::CoinA) +
    cost(ai_access_key_detail::SpatialLocation::CoinA,
        ai_access_key_detail::SpatialLocation::CoinC) +
    cost(ai_access_key_detail::SpatialLocation::CoinC,
        ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startBToARoute =
        cost(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startBToCRoute =
        cost(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startCToBRoute =
        cost(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    EXPECT_LT(selectedCoinRoute, startAToBRoute);
    EXPECT_LT(selectedCoinRoute, startAToCRoute);
    EXPECT_LT(selectedCoinRoute, startBToARoute);
    EXPECT_LT(selectedCoinRoute, startBToCRoute);
    EXPECT_LT(selectedCoinRoute, startCToBRoute);

    // Protects route-cost selection from accidentally falling back to context-id ordering.
    EXPECT_LT(ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::Start,
                  ai_access_key_detail::SpatialLocation::CoinA).value,
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC).value);

    GameplayRouteGraph disconnected = graph;
    disconnected.edges.clear();
    EXPECT_FALSE(ai_access_key_detail::BuildDefinition(disconnected).has_value());
}

TEST(GameplayAIDecision, BuyKeyRuntimeRequiresCoinsAndRuntimeConfirmation)
{
    GameplayWorld world{};
    const EntityHandle agent = world.CreateEntity();
    world.AddTransform(agent, {.position={0.0f, 0.0f, 0.0f}});
    const EntityHandle key = world.CreateEntity();
    constexpr mathUtils::Vec3 keyPosition{10.0f, 0.0f, 0.0f};
    world.AddTransform(key, {.position=keyPosition});
    AIAgentWorldState observed{};
    std::vector<GameplayWorldEvent> events;
    ai_access_key_detail::BuyKeyActionRuntime insufficient{
        observed, world, events, key, {0.0f, 0.0f, 0.0f}};
    const AIActionRuntimeContext context{agent, kAIBuyKeyActionId};
    EXPECT_EQ(insufficient.Start(context), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(observed.IsFactSet(kGOAPHasAccessKeyFact));

    observed.SetIntegerFact(kGOAPCoinCountFact, kAccessKeyPrice);
    ai_access_key_detail::BuyKeyActionRuntime tooFar{
        observed, world, events, key, keyPosition};
    EXPECT_EQ(tooFar.Start(context), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(events.empty());
    EXPECT_EQ(observed.GetIntegerFact(kGOAPCoinCountFact), kAccessKeyPrice);
    EXPECT_FALSE(observed.IsFactSet(kGOAPHasAccessKeyFact));

    world.TryGetTransform(agent)->position=keyPosition;
    ai_access_key_detail::BuyKeyActionRuntime purchase{
        observed, world, events, key, keyPosition};
    EXPECT_EQ(purchase.Start(context), AIActionRuntimeResult::Succeeded);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events.front().type, GameplayWorldEventType::AccessKeyPurchased);
    // Runtime confirmation is produced without directly applying planner effects.
    EXPECT_EQ(observed.GetIntegerFact(kGOAPCoinCountFact), kAccessKeyPrice);
    EXPECT_FALSE(observed.IsFactSet(kGOAPHasAccessKeyFact));
}

TEST(GameplayAIDecision, AuthoredCoinsAndAccessKeyUsePhysicalObservationAndCancelCleanly)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    EXPECT_EQ(level.developmentScenario, "development/ai_goap_access_key.scenario.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    const EntityHandle player=runtime.GetControlledEntity();
    EntityHandle agent=FindNodeEntity(runtime,level,"GOAP_Agent");
    if (agent == kNullEntity)
    {
        const auto node = std::ranges::find_if(level.nodes,
            [](const LevelNode& value) { return value.name == "GOAP_Agent"; });
        ASSERT_NE(node,level.nodes.end());
        agent=runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(),node)),false);
    }
    ASSERT_NE(player,kNullEntity); ASSERT_NE(agent,kNullEntity); EXPECT_NE(player,agent);
    for (const std::string_view coinName : {"GOAP_Coin_A", "GOAP_Coin_B", "GOAP_Coin_C"})
    {
        EntityHandle coin = FindNodeEntity(runtime, level, coinName);
        if (coin == kNullEntity)
        {
            const auto node = std::ranges::find_if(level.nodes, [&](const LevelNode& value)
                { return value.name == coinName; });
            ASSERT_NE(node, level.nodes.end());
            coin = runtime.SpawnNodeBoundEntity(game,
                static_cast<int>(std::distance(level.nodes.begin(), node)), false);
        }
        ASSERT_NE(coin, kNullEntity);
        runtime.GetWorld().SetPickup(coin, {});
    }
    EntityHandle keyEntity = FindNodeEntity(runtime, level, "GOAP_Access_Key");
    if (keyEntity == kNullEntity)
    {
        const auto node = std::ranges::find_if(level.nodes,
            [](const LevelNode& value) { return value.name == "GOAP_Access_Key"; });
        ASSERT_NE(node, level.nodes.end());
        keyEntity = runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(), node)), false);
    }
    ASSERT_NE(keyEntity, kNullEntity);
    const int keyNode = runtime.GetWorld().TryGetNodeLink(keyEntity)->nodeIndex;
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(keyNode));
    if (!runtime.GetWorld().HasAI(agent)) { runtime.GetWorld().AddAI(agent); }
    auto moveToRequests=ai_access_key_detail::BuildMoveToProvider(runtime.GetWorld(),
         ai_access_key_detail::BuildRouteGraph({0,0,0}, {-6,0.35f,-3}, {6,0.35f,1},
             {-5,0.35f,6}, {0,0.35f,-7}, {0,0.08f,10}));
    const auto resolveMove = [&](const ai_access_key_detail::SpatialLocation source,
        const ai_access_key_detail::SpatialLocation target)
    {
        return moveToRequests.ResolveRequest(
            AIActionRuntimeContext{.agentEntity=agent, .actionId=kAIMoveToActionId,
                .contextId=ai_access_key_detail::MoveContext(source, target)});
    }; 
    runtime.GetWorld().TryGetTransform(agent)->position={6,0.35f,1};
    EXPECT_FALSE(resolveMove(ai_access_key_detail::SpatialLocation::CoinA,
        ai_access_key_detail::SpatialLocation::CoinC).has_value());
    runtime.GetWorld().TryGetTransform(agent)->position={-6,0.35f,-3};
    const auto coinCRequest = resolveMove(ai_access_key_detail::SpatialLocation::CoinA,
        ai_access_key_detail::SpatialLocation::CoinC);
    ASSERT_TRUE(coinCRequest.has_value());
    EXPECT_EQ(coinCRequest->startNodeId, GameplayRouteNodeId{2u});
    EXPECT_EQ(coinCRequest->goalNodeId, GameplayRouteNodeId{4u});
    EXPECT_NE(ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::Start,
        ai_access_key_detail::SpatialLocation::CoinC),
        ai_access_key_detail::MoveContext(ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinC));
    
    EXPECT_FALSE(moveToRequests.ResolveRequest(
        AIActionRuntimeContext{.agentEntity=agent, .actionId=kAIMoveToActionId,
            .contextId=AIActionContextId{99u}}).has_value());
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,0};
    ASSERT_TRUE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    EXPECT_FALSE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    
    Tick(runtime,game);
    const AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_TRUE(facts->IsFactSet(kGOAPAtStartFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtCoinAFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtCoinBFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtCoinCFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtAccessKeyShopFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtGoalFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinAAvailableFact));
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinBAvailableFact));
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinCAvailableFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    const EntityHandle coinAEntity=FindNodeEntity(runtime,level,"GOAP_Coin_A");
    ASSERT_NE(coinAEntity,kNullEntity);
    GameplayPickupComponent* coinAPickup=runtime.GetWorld().TryGetPickup(coinAEntity);
    ASSERT_NE(coinAPickup,nullptr);
    coinAPickup->collected=true;
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinAAvailableFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(kGOAPCoinCountFact),0);
    coinAPickup->collected=false;
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,10};
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtDestinationFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={-5,0.35f,6};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(kGOAPCoinCountFact), 1);
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={-6,0.35f,-3};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(facts->GetIntegerFact(kGOAPCoinCountFact), 2);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(keyNode));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_EQ(facts->GetIntegerFact(kGOAPCoinCountFact), 0);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(keyNode));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.08f,10};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPAtDestinationFact));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::Succeeded);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::Succeeded);

    EXPECT_TRUE(runtime.DestroyNodeBoundEntity(agent));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(agent),nullptr);
}

TEST(GameplayAIDecision, MissingMovementContractRejectsStartWithoutPartialState)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    GameplayWorld& world=runtime.GetWorld();
    const EntityHandle incomplete=world.CreateEntity();
    world.AddTransform(incomplete,{}); world.AddAI(incomplete);
    EXPECT_FALSE(runtime.StartAIDecision(incomplete,kAccessKeyAIDecisionId));
    EXPECT_EQ(runtime.GetAIDecisionStatus(incomplete),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(incomplete),nullptr);
}

TEST(GameplayAIDecision, FailedStartLeavesNoActiveDecision)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level{}; LevelNode node{}; node.alive=true; node.name="Agent";
    level.nodes.push_back(node); LevelInstance instance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    
    auto editor=Context(level,instance,scene,GameplayRuntimeMode::Editor);
    const EntityHandle agent=runtime.SpawnNodeBoundEntity(editor,0,false);
    ASSERT_NE(agent,kNullEntity);
    runtime.GetWorld().AddAI(agent);
        // Enter Game mode first so this test exercises production decision
    // creation failure rather than merely the Editor-mode start guard.
    auto game=Context(level,instance,scene,GameplayRuntimeMode::Game);
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
        ASSERT_TRUE(runtime.GetWorld().HasTransform(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterCommand(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterMotor(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterMovementState(agent));
    
    EXPECT_FALSE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(agent),nullptr);
}

TEST(GameplayAIDecision, AccessKeyMapsOnlyMatchingAgentAndCoinEvents)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    GameplayWorld& world=runtime.GetWorld();
    const auto spawnNamed = [&](const std::string_view name)
    {
        if (const EntityHandle existing=FindNodeEntity(runtime,level,name); existing != kNullEntity)
            return existing;
        const auto node=std::ranges::find_if(level.nodes,
            [&](const LevelNode& value) { return value.name == name; });
        EXPECT_NE(node,level.nodes.end());
        return runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(),node)),false);
    };
    const EntityHandle agent=spawnNamed("GOAP_Agent");
    const EntityHandle coinA=spawnNamed("GOAP_Coin_A");
    const EntityHandle coinB=spawnNamed("GOAP_Coin_B");
    ASSERT_NE(agent,kNullEntity); ASSERT_NE(coinA,kNullEntity); ASSERT_NE(coinB,kNullEntity);
    const EntityHandle coinC=spawnNamed("GOAP_Coin_C");
    const EntityHandle key=spawnNamed("GOAP_Access_Key");
    ASSERT_NE(coinC,kNullEntity); ASSERT_NE(key,kNullEntity);
    world.AddPickup(coinA); world.AddPickup(coinB); world.AddPickup(coinC);
    world.AddAI(agent);
    const EntityHandle otherAgent=world.CreateEntity();
    world.AddTransform(otherAgent,{}); world.AddAI(otherAgent);
    GameplayTraversalLinkRegistry links;
    GameplayTraversalExecutorRegistry executors;
    auto decision=CreateAccessKeyAIDecision(agent,level,world,links,executors);
    ASSERT_NE(decision,nullptr);
    AISystem ai;

    world.TryGetTransform(agent)->position = world.TryGetTransform(coinA)->position;
    decision->Update(ai, GameplayAIObservationContext{world, {}});
    EXPECT_TRUE(decision->GetObservedState().IsFactSet(kGOAPAtCoinAFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPAtStartFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(decision->GetObservedState().GetIntegerFact(kGOAPCoinCountFact), 0);
    const std::array coinBEvent{GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
        agent,coinB}};
    decision->Update(ai,GameplayAIObservationContext{world,coinBEvent});
    EXPECT_TRUE(decision->GetObservedState().IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(decision->GetObservedState().GetIntegerFact(kGOAPCoinCountFact), 1);
    decision->Update(ai,GameplayAIObservationContext{world,coinBEvent});
    EXPECT_EQ(decision->GetObservedState().GetIntegerFact(kGOAPCoinCountFact), 1);

    const std::array otherAgentCoinAEvent{GameplayWorldEvent{
        GameplayWorldEventType::PickupCollected,otherAgent,coinA}};
    decision->Update(ai,GameplayAIObservationContext{world,otherAgentCoinAEvent});
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    
    AIAgentWorldState& observed =
        const_cast<AIAgentWorldState&>(decision->GetObservedState());
    constexpr std::int32_t initialCoinCount = 6;
    observed.SetIntegerFact(kGOAPCoinCountFact, initialCoinCount);
    const std::array purchaseEvent{GameplayWorldEvent{
        GameplayWorldEventType::AccessKeyPurchased,agent,key}};
    decision->Update(ai,GameplayAIObservationContext{world,purchaseEvent});
    EXPECT_EQ(observed.GetIntegerFact(kGOAPCoinCountFact),
        initialCoinCount - kAccessKeyPrice);
    EXPECT_TRUE(observed.IsFactSet(kGOAPHasAccessKeyFact));
    
    decision->Update(ai,GameplayAIObservationContext{world,purchaseEvent});
    EXPECT_EQ(observed.GetIntegerFact(kGOAPCoinCountFact),
        initialCoinCount - kAccessKeyPrice);
    EXPECT_TRUE(observed.IsFactSet(kGOAPHasAccessKeyFact));
}