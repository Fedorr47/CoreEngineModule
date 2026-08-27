#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

import core;

using namespace rendern;
using namespace ai_access_key_detail;

namespace
{
    class LifetimeBinding final : public IAIActionBinding
    {
    public:
        explicit LifetimeBinding(bool& destroyed) noexcept : destroyed_(&destroyed) {}
        ~LifetimeBinding() override { *destroyed_ = true; }

        [[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext&) override
        {
            return nullptr;
        }

    private:
        bool* destroyed_{};
    };
    
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

    [[nodiscard]] AIActionContextId ResolvedMoveContext(
        const GameplayGOAPCompiledDefinition& definition,
        const ai_access_key_detail::SpatialLocation source,
        const ai_access_key_detail::SpatialLocation target)
    {
        return ai_access_key_detail::FindMoveContext(definition, source, target).value();
    }
}

TEST(GameplayGOAPDecision, SemanticActionBindingsMustBeComplete)
{
    constexpr AIActionId sharedAction{3u};
    constexpr AIActionId otherAction{7u};
    GameplayGOAPDecisionDefinition definition{};
    definition.actions = {
        AIActionDefinition{.actionId = sharedAction, .contextId = AIActionContextId{10u}},
        AIActionDefinition{.actionId = sharedAction, .contextId = AIActionContextId{11u}},
        AIActionDefinition{.actionId = otherAction, .contextId = AIActionContextId{12u}}};

    GameplayGOAPDecision decision{EntityHandle{1u}, std::move(definition)};
    EXPECT_FALSE(decision.HasCompleteActionBindings());

    bool sharedDestroyed = false;
    ASSERT_TRUE(decision.InstallActionBinding(
        sharedAction, std::make_unique<LifetimeBinding>(sharedDestroyed)));
    EXPECT_FALSE(decision.HasCompleteActionBindings());

    bool otherDestroyed = false;
    ASSERT_TRUE(decision.InstallActionBinding(
        otherAction, std::make_unique<LifetimeBinding>(otherDestroyed)));
    EXPECT_TRUE(decision.HasCompleteActionBindings());
}

TEST(GameplayGOAPDecision, CancelPreservesInstalledCapabilityLifetime)
{
    bool destroyed = false;
    AISystem aiSystem{};
    {
        GameplayGOAPDecision decision{EntityHandle{1u}, {}};
        ASSERT_TRUE(decision.InstallActionBinding(
            AIActionId{3u}, std::make_unique<LifetimeBinding>(destroyed)));

        decision.Cancel(aiSystem);

        EXPECT_FALSE(destroyed);
    }
    EXPECT_TRUE(destroyed);
}

TEST(GameplayGOAPDecision, DuplicateBindingDoesNotReplaceInstalledCapability)
{
    constexpr AIActionId actionId{3u};

    bool firstDestroyed = false;
    bool duplicateDestroyed = false;

    {
        GameplayGOAPDecision decision{EntityHandle{1u}, {}};

        ASSERT_TRUE(decision.InstallActionBinding(
            actionId, std::make_unique<LifetimeBinding>(firstDestroyed)));

        EXPECT_FALSE(decision.InstallActionBinding(
            actionId, std::make_unique<LifetimeBinding>(duplicateDestroyed)));

        // Failed duplicate registration owns nothing. The rejected binding is
        // destroyed immediately while the original capability remains installed.
        EXPECT_FALSE(firstDestroyed);
        EXPECT_TRUE(duplicateDestroyed);
    }

    EXPECT_TRUE(firstDestroyed);
}

TEST(GameplayAIDecision, AccessKeyDefinitionPlansOneShotCoinsAndSemanticPurchase)
{
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph(
        {0, 0, 0}, {-6, 0.35f, -3}, {6, 0.35f, 1}, {-5, 0.35f, 6},
        {0, 0.35f, -7}, {0, 0.08f, 10});

    const auto configuredDefinition = ai_access_key_detail::LoadCompiledDefinition(graph);
    ASSERT_TRUE(configuredDefinition.has_value());

    const GameplayGOAPDecisionDefinition& definition = configuredDefinition->definition;
    const auto buyKey = std::ranges::find_if(
        definition.actions,
        [](const AIActionDefinition& action)
        {
            return action.actionId == kAIBuyKeyActionId;
        });
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

    EXPECT_EQ(plan->steps[0], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *configuredDefinition,
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC)}));
    EXPECT_EQ(plan->steps[1], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *configuredDefinition,
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinA)}));
    EXPECT_EQ(plan->steps[2], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *configuredDefinition,
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop)}));
    EXPECT_EQ(plan->steps[3].actionId, kAIBuyKeyActionId);
    EXPECT_EQ(plan->steps[4], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *configuredDefinition,
            ai_access_key_detail::SpatialLocation::AccessKeyShop,
            ai_access_key_detail::SpatialLocation::Goal)}));

    AIAgentWorldState predicted = initial;
    for (const AIPlanStep& step : plan->steps)
    {
        const auto action = std::ranges::find_if(
            definition.actions,
            [&](const AIActionDefinition& candidate)
            {
                return candidate.actionId == step.actionId &&
                    candidate.contextId == step.contextId;
            });
        ASSERT_NE(action, definition.actions.end());

        EXPECT_TRUE(AreFactConditionsSatisfied(predicted, action->preconditions));
        EXPECT_TRUE(AreNumericConditionsSatisfied(predicted, action->numericPreconditions));
        ApplyFactEffects(predicted, action->effects);
        EXPECT_TRUE(ApplyNumericEffects(predicted, action->numericEffects));

        const std::array spatialFacts{
            kGOAPAtStartFact,
            kGOAPAtCoinAFact,
            kGOAPAtCoinBFact,
            kGOAPAtCoinCFact,
            kGOAPAtAccessKeyShopFact,
            kGOAPAtGoalFact};
        EXPECT_EQ(
            std::ranges::count_if(
                spatialFacts,
                [&](const AIWorldFactId fact)
                {
                    return predicted.IsFactSet(fact);
                }),
            1);
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
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph(
        {0, 0, 0}, {-6, 0.35f, -3}, {6, 0.35f, 1}, {-5, 0.35f, 6},
        {0, 0.35f, -7}, {0, 0.08f, 10});

    const auto definition = ai_access_key_detail::LoadCompiledDefinition(graph);
    ASSERT_TRUE(definition.has_value());

    AIAgentWorldState observed{};
    observed.SetFact(kGOAPAtCoinCFact, true);
    observed.SetFact(kGOAPCoinCCollectedFact, true);
    observed.SetFact(kGOAPCoinAAvailableFact, false);
    observed.SetFact(kGOAPCoinBAvailableFact, true);
    observed.SetFact(kGOAPCoinCAvailableFact, false);
    observed.SetIntegerFact(kGOAPCoinCountFact, 1);

    const auto plan = FindAIPlan(
        observed,
        definition->definition.goals.front().goal,
        definition->definition.actions);
    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 4u);

    EXPECT_EQ(plan->steps[0], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *definition,
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinB)}));
    EXPECT_EQ(plan->steps[1], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *definition,
            ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop)}));
    EXPECT_EQ(plan->steps[2].actionId, kAIBuyKeyActionId);
    EXPECT_EQ(plan->steps[3], (AIPlanStep{
        kAIMoveToActionId,
        ResolvedMoveContext(
            *definition,
            ai_access_key_detail::SpatialLocation::AccessKeyShop,
            ai_access_key_detail::SpatialLocation::Goal)}));
}

TEST(GameplayAIDecision, AccessKeyMoveCostsComeFromHypotheticalRouteSource)
{
    const GameplayRouteGraph graph = ai_access_key_detail::BuildRouteGraph(
        {0, 0, 0}, {-6, 0.35f, -3}, {6, 0.35f, 1}, {-5, 0.35f, 6},
        {0, 0.35f, -7}, {0, 0.08f, 10});

    const auto definition = ai_access_key_detail::LoadCompiledDefinition(graph);
    ASSERT_TRUE(definition.has_value());

    const auto cost = [&](const ai_access_key_detail::SpatialLocation source,
        const ai_access_key_detail::SpatialLocation target)
    {
        const AIActionContextId contextId = ResolvedMoveContext(*definition, source, target);
        const auto action = std::ranges::find_if(
            definition->definition.actions,
            [&](const AIActionDefinition& candidate)
            {
                return candidate.actionId == kAIMoveToActionId &&
                    candidate.contextId == contextId;
            });
        EXPECT_NE(action, definition->definition.actions.end());
        return action == definition->definition.actions.end() ? -1.0f : action->baseCost;
    };

    EXPECT_NE(
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC),
        cost(
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinC));
    EXPECT_NE(
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinA),
        1.0f);

    const float selectedCoinRoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startAToBRoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startAToCRoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startBToARoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::CoinA) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinA,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startBToCRoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    const float startCToBRoute =
        cost(
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinC,
            ai_access_key_detail::SpatialLocation::CoinB) +
        cost(
            ai_access_key_detail::SpatialLocation::CoinB,
            ai_access_key_detail::SpatialLocation::AccessKeyShop);

    EXPECT_LT(selectedCoinRoute, startAToBRoute);
    EXPECT_LT(selectedCoinRoute, startAToCRoute);
    EXPECT_LT(selectedCoinRoute, startBToARoute);
    EXPECT_LT(selectedCoinRoute, startBToCRoute);
    EXPECT_LT(selectedCoinRoute, startCToBRoute);

    GameplayRouteGraph disconnected = graph;
    disconnected.edges.clear();
    EXPECT_FALSE(ai_access_key_detail::LoadCompiledDefinition(disconnected).has_value());
}

TEST(GameplayAIDecision, BuyKeyRuntimeRequiresCoinsAndRuntimeConfirmation)
{
    GameplayWorld world{};
    const EntityHandle agent = world.CreateEntity();
    world.AddTransform(agent, {.position = {0.0f, 0.0f, 0.0f}});

    const EntityHandle key = world.CreateEntity();
    constexpr mathUtils::Vec3 keyPosition{10.0f, 0.0f, 0.0f};
    world.AddTransform(key, {.position = keyPosition});

    // Arbitrary valid ids ensure the runtime does not depend on the AccessKey asset layout.
    constexpr AIWorldFactId hasAccessKeyFact{90u};
    constexpr AIWorldIntegerFactId coinsFact{91u};

    AIAgentWorldState observed{};
    std::vector<GameplayWorldEvent> events;
    const AIActionRuntimeContext context{agent, kAIBuyKeyActionId};

    ai_access_key_detail::BuyKeyActionRuntime insufficient{
        observed,
        world,
        events,
        key,
        {0.0f, 0.0f, 0.0f},
        hasAccessKeyFact,
        coinsFact};
    EXPECT_EQ(insufficient.Start(context), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(events.empty());
    EXPECT_FALSE(observed.IsFactSet(hasAccessKeyFact));

    observed.SetIntegerFact(coinsFact, kAccessKeyPrice);
    ai_access_key_detail::BuyKeyActionRuntime tooFar{
        observed,
        world,
        events,
        key,
        keyPosition,
        hasAccessKeyFact,
        coinsFact};
    EXPECT_EQ(tooFar.Start(context), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(events.empty());
    EXPECT_EQ(observed.GetIntegerFact(coinsFact), kAccessKeyPrice);
    EXPECT_FALSE(observed.IsFactSet(hasAccessKeyFact));

    world.TryGetTransform(agent)->position = keyPosition;
    ai_access_key_detail::BuyKeyActionRuntime purchase{
        observed,
        world,
        events,
        key,
        keyPosition,
        hasAccessKeyFact,
        coinsFact};
    EXPECT_EQ(purchase.Start(context), AIActionRuntimeResult::Succeeded);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events.front().type, GameplayWorldEventType::AccessKeyPurchased);

    // Runtime confirmation is produced without directly applying planner effects.
    EXPECT_EQ(observed.GetIntegerFact(coinsFact), kAccessKeyPrice);
    EXPECT_FALSE(observed.IsFactSet(hasAccessKeyFact));
}

TEST(GameplayAIDecision, AuthoredCoinsAndAccessKeyUsePhysicalObservationAndCancelCleanly)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    EXPECT_EQ(level.developmentScenario, "development/ai_goap_access_key.scenario.json");
    LevelInstance instance = harness.Instantiate(level);
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    auto game = Context(level, instance, harness.GetScene(), GameplayRuntimeMode::Game);
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
    const EntityHandle player = runtime.GetControlledEntity();
    EntityHandle agent = FindNodeEntity(runtime, level, "GOAP_Agent");
    if (agent == kNullEntity)
    {
        const auto node = std::ranges::find_if(level.nodes,
            [](const LevelNode& value) { return value.name == "GOAP_Agent"; });
        ASSERT_NE(node,level.nodes.end());
        agent=runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(),node)),false);
    }
    ASSERT_NE(player, kNullEntity);
    ASSERT_NE(agent, kNullEntity);
    EXPECT_NE(player, agent);
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
    if (!runtime.GetWorld().HasAI(agent))
    {
        runtime.GetWorld().AddAI(agent);
    }

    GameplayRouteGraph moveRouteGraph = ai_access_key_detail::BuildRouteGraph(
        {0, 0, 0}, {-6, 0.35f, -3}, {6, 0.35f, 1}, {-5, 0.35f, 6},
        {0, 0.35f, -7}, {0, 0.08f, 10});
    const auto compiledDefinition =
        ai_access_key_detail::LoadCompiledDefinition(moveRouteGraph);
    ASSERT_TRUE(compiledDefinition.has_value());

    auto resolvedTransitions =
        ai_access_key_detail::ResolveMoveTransitions(*compiledDefinition);
    ASSERT_TRUE(resolvedTransitions.has_value());

    auto moveToRequests = ai_access_key_detail::BuildMoveToProvider(
        runtime.GetWorld(),
        std::move(moveRouteGraph),
        std::move(*resolvedTransitions));

    const auto resolveMove = [&](const ai_access_key_detail::SpatialLocation source,
        const ai_access_key_detail::SpatialLocation target)
    {
        return moveToRequests.ResolveRequest(
            AIActionRuntimeContext{
                .agentEntity = agent,
                .actionId = kAIMoveToActionId,
                .contextId = ResolvedMoveContext(*compiledDefinition, source, target)});
    };

    runtime.GetWorld().TryGetTransform(agent)->position = {6, 0.35f, 1};
    EXPECT_FALSE(resolveMove(ai_access_key_detail::SpatialLocation::CoinA,
        ai_access_key_detail::SpatialLocation::CoinC).has_value());
    runtime.GetWorld().TryGetTransform(agent)->position={-6,0.35f,-3};
    const auto coinCRequest = resolveMove(ai_access_key_detail::SpatialLocation::CoinA,
        ai_access_key_detail::SpatialLocation::CoinC);
    ASSERT_TRUE(coinCRequest.has_value());
    EXPECT_EQ(coinCRequest->startNodeId, GameplayRouteNodeId{2u});
    EXPECT_EQ(coinCRequest->goalNodeId, GameplayRouteNodeId{4u});
    EXPECT_NE(
        ResolvedMoveContext(
            *compiledDefinition,
            ai_access_key_detail::SpatialLocation::Start,
            ai_access_key_detail::SpatialLocation::CoinC),
        ResolvedMoveContext(
            *compiledDefinition,
            ai_access_key_detail::SpatialLocation::CoinA,
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
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance = harness.Instantiate(level);
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    auto game = Context(level, instance, harness.GetScene(), GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    GameplayWorld& world=runtime.GetWorld();
    const EntityHandle incomplete=world.CreateEntity();
    world.AddTransform(incomplete,{}); world.AddAI(incomplete);
    EXPECT_FALSE(runtime.StartAIDecision(incomplete,kAccessKeyAIDecisionId));
    EXPECT_EQ(runtime.GetAIDecisionStatus(incomplete),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(incomplete),nullptr);
}

TEST(GameplayAIDecision, LowerEntityHandleWinsReservedCoinContentionRegardlessOfStartOrder)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance = harness.Instantiate(level);
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    const auto game = Context(level, instance, harness.GetScene(), GameplayRuntimeMode::Game);
    Tick(runtime, game);

    const auto ensureNamedEntity = [&](const std::string_view name)
    {
        if (const EntityHandle existing = FindNodeEntity(runtime, level, name);
            existing != kNullEntity)
        {
            return existing;
        }
        const auto node = std::ranges::find_if(level.nodes,
            [&](const LevelNode& value) { return value.name == name; });
        EXPECT_NE(node, level.nodes.end());
        return runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(), node)), false);
    };
    const EntityHandle lowAgent = ensureNamedEntity("GOAP_Agent");
    const EntityHandle coinA = ensureNamedEntity("GOAP_Coin_A");
    const EntityHandle coinB = ensureNamedEntity("GOAP_Coin_B");
    const EntityHandle coinC = ensureNamedEntity("GOAP_Coin_C");
    (void)ensureNamedEntity("GOAP_Access_Key");
    ASSERT_NE(lowAgent, kNullEntity);
    ASSERT_NE(coinA, kNullEntity);
    ASSERT_NE(coinB, kNullEntity);
    ASSERT_NE(coinC, kNullEntity);
    GameplayWorld& world = runtime.GetWorld();
    world.SetPickup(coinA, {});
    world.SetPickup(coinB, {});
    world.SetPickup(coinC, {});
    world.AddInteractionPoint(coinA, {});
    world.AddInteractionPoint(coinB, {});
    world.AddInteractionPoint(coinC, {});
    if (!world.HasAI(lowAgent))
    {
        world.AddAI(lowAgent);
    }
    world.TryGetTransform(lowAgent)->position = {};

    const EntityHandle highAgent = world.CreateEntity();
    world.AddAI(highAgent);
    world.AddTransform(highAgent, {});
    world.AddCharacterCommand(highAgent, {});
    world.AddCharacterMotor(highAgent, {});
    world.AddCharacterMovementState(highAgent, {});
    ASSERT_LT(lowAgent, highAgent);

    ASSERT_TRUE(runtime.StartAIDecision(highAgent, kAccessKeyAIDecisionId));
    ASSERT_TRUE(runtime.StartAIDecision(lowAgent, kAccessKeyAIDecisionId));
    Tick(runtime, game);

    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(coinC), lowAgent);
}

TEST(GameplayAIDecisionSetup, ValidScenarioResolvesCompleteComposition)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance = harness.Instantiate(level);
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    const auto game = Context(level, instance, harness.GetScene(), GameplayRuntimeMode::Game);
    Tick(runtime, game);

    const auto ensureNamedEntity = [&](const std::string_view name)
    {
        if (const EntityHandle existing = FindNodeEntity(runtime, level, name);
            existing != kNullEntity)
        {
            return existing;
        }
        const auto node = std::ranges::find_if(level.nodes,
            [&](const LevelNode& value) { return value.name == name; });
        EXPECT_NE(node, level.nodes.end());
        return runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(), node)), false);
    };
    const EntityHandle coinA = ensureNamedEntity("GOAP_Coin_A");
    const EntityHandle coinB = ensureNamedEntity("GOAP_Coin_B");
    const EntityHandle coinC = ensureNamedEntity("GOAP_Coin_C");
    const EntityHandle key = ensureNamedEntity("GOAP_Access_Key");
    GameplayWorld& world = runtime.GetWorld();
    world.SetPickup(coinA, {});
    world.SetPickup(coinB, {});
    world.SetPickup(coinC, {});

    const auto setup = BuildAccessKeyDecisionSetup(level, world);

    ASSERT_TRUE(setup.has_value());
    EXPECT_EQ(setup->coinEntities, (std::array{coinA, coinB, coinC}));
    EXPECT_EQ(setup->keyEntity, key);
    EXPECT_FALSE(setup->definition.actions.empty());
    EXPECT_EQ(setup->moveTransitions.size(), ai_access_key_detail::kMoveTransitions.size());

    world.RemovePickup(coinB);
    EXPECT_FALSE(BuildAccessKeyDecisionSetup(level, world).has_value());
    world.SetPickup(coinB, {});
    world.DestroyEntity(key);
    EXPECT_FALSE(BuildAccessKeyDecisionSetup(level, world).has_value());
}

TEST(GameplayAIDecisionSetup, MissingRequiredNamedNodeFailsTransactionally)
{
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    const auto coinB = std::ranges::find_if(level.nodes,
        [](const LevelNode& node) { return node.name == "GOAP_Coin_B"; });
    ASSERT_NE(coinB, level.nodes.end());
    coinB->alive = false;

    GameplayWorld world{};
    EXPECT_FALSE(BuildAccessKeyDecisionSetup(level, world).has_value());
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
    LevelAsset level = LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance = harness.Instantiate(level);
    GameplayRuntime runtime{};
    runtime.Initialize(level, instance, harness.GetScene());
    auto game = Context(level, instance, harness.GetScene(), GameplayRuntimeMode::Game);
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
    world.AddInteractionPoint(coinA, {}); world.AddInteractionPoint(coinB, {});
    world.AddInteractionPoint(coinC, {});
    world.AddAI(agent);
    const EntityHandle otherAgent=world.CreateEntity();
    world.AddTransform(otherAgent,{}); world.AddAI(otherAgent);
    GameplayTraversalLinkRegistry links;
    GameplayTraversalExecutorRegistry executors;
    GameplayObjectReservationSystem reservations;
    auto decision=CreateAccessKeyAIDecision(agent,level,world,links,executors,&reservations);
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
    
    ASSERT_TRUE(reservations.TryReserve(world, coinA, agent));
    decision->Update(ai, GameplayAIObservationContext{world, {}});
    EXPECT_TRUE(decision->GetObservedState().IsFactSet(kGOAPCoinAAvailableFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(decision->GetObservedState().GetIntegerFact(kGOAPCoinCountFact), 1);
    ASSERT_TRUE(reservations.Release(coinA, agent));

    ASSERT_TRUE(reservations.TryReserve(world, coinA, otherAgent));
    decision->Update(ai, GameplayAIObservationContext{world, {}});
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinAAvailableFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_EQ(decision->GetObservedState().GetIntegerFact(kGOAPCoinCountFact), 1);
    ASSERT_TRUE(reservations.Release(coinA, otherAgent));
    decision->Update(ai, GameplayAIObservationContext{world, {}});
    EXPECT_TRUE(decision->GetObservedState().IsFactSet(kGOAPCoinAAvailableFact));

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
    
    // ---------------------------------------------------------------------
    // Runtime-generated purchase event + aliased input/output
    // ---------------------------------------------------------------------
    
    // A fresh decision lets the real BuyKey runtime produce the purchase event.
    // This exercises runtime -> domain observation -> outward event composition
    // rather than injecting AccessKeyPurchased as an input event.
    auto runtimeEventDecision = CreateAccessKeyAIDecision(
        agent, level, world, links, executors, &reservations);
    ASSERT_NE(runtimeEventDecision, nullptr);

    AIAgentWorldState& runtimeEventFacts =
        const_cast<AIAgentWorldState&>(runtimeEventDecision->GetObservedState());
    runtimeEventFacts.SetIntegerFact(kGOAPCoinCountFact, kAccessKeyPrice);

    GameplayTransformComponent* agentTransform = world.TryGetTransform(agent);
    const GameplayTransformComponent* keyTransform = world.TryGetTransform(key);
    ASSERT_NE(agentTransform, nullptr);
    ASSERT_NE(keyTransform, nullptr);
    agentTransform->position = keyTransform->position;

    AISystem runtimeEventAI{};

    // Deliberately use the same vector as both observation input and output.
    // AccessKeyDecision must consume the input span before appending runtime
    // events to the vector, so reallocation cannot invalidate active reads.
    std::vector<GameplayWorldEvent> aliasedEvents{
        GameplayWorldEvent{
            GameplayWorldEventType::PickupCollected,
            otherAgent,
            coinA
        }
    };

    bool purchaseConfirmed = false;

    for (int updateIndex = 0; updateIndex < 4 && !purchaseConfirmed; ++updateIndex)
    {
        // Keep one benign input event for every update. A previous runtime
        // output must not become an accidental input to the next iteration.
        aliasedEvents.resize(1u);

        const bool hadAccessKey =
            runtimeEventFacts.IsFactSet(kGOAPHasAccessKeyFact);

        runtimeEventDecision->Update(
            runtimeEventAI,
            GameplayAIObservationContext{
                world,
                std::span<const GameplayWorldEvent>{aliasedEvents},
                &aliasedEvents
            });

        const bool hasAccessKey =
            runtimeEventFacts.IsFactSet(kGOAPHasAccessKeyFact);

        if (!hadAccessKey && hasAccessKey)
        {
            purchaseConfirmed = true;

            // The purchase is confirmed from the runtime-generated event
            // during the same decision update that emitted it.
            EXPECT_EQ(
                runtimeEventFacts.GetIntegerFact(kGOAPCoinCountFact),
                0);

            EXPECT_EQ(
                std::ranges::count_if(
                    aliasedEvents,
                    [&](const GameplayWorldEvent& event)
                    {
                        return event.type ==
                                   GameplayWorldEventType::AccessKeyPurchased &&
                               event.instigator == agent &&
                               event.subject == key;
                    }),
                1);

            // The original aliased input event is preserved and exactly one
            // new runtime event is appended.
            ASSERT_EQ(aliasedEvents.size(), 2u);
            EXPECT_EQ(aliasedEvents.front().instigator, otherAgent);
            EXPECT_EQ(aliasedEvents.front().subject, coinA);
        }
    }

    EXPECT_TRUE(purchaseConfirmed);
    runtimeEventDecision->Cancel(runtimeEventAI);
    decision->Update(ai,GameplayAIObservationContext{world,purchaseEvent});
    EXPECT_EQ(observed.GetIntegerFact(kGOAPCoinCountFact),
        initialCoinCount - kAccessKeyPrice);
    EXPECT_TRUE(observed.IsFactSet(kGOAPHasAccessKeyFact));
    
    // ---------------------------------------------------------------------
    // Runtime-generated purchase with no outward event sink
    // ---------------------------------------------------------------------
    
    auto nullOutputDecision = CreateAccessKeyAIDecision(
       agent, level, world, links, executors, &reservations);
    ASSERT_NE(nullOutputDecision, nullptr);

    AIAgentWorldState& nullOutputFacts =
        const_cast<AIAgentWorldState&>(nullOutputDecision->GetObservedState());
    nullOutputFacts.SetIntegerFact(kGOAPCoinCountFact, kAccessKeyPrice);

    agentTransform->position = keyTransform->position;

    AISystem nullOutputAI{};
    bool nullOutputPurchaseConfirmed = false;

    for (int updateIndex = 0;
         updateIndex < 4 && !nullOutputPurchaseConfirmed;
         ++updateIndex)
    {
        const bool hadAccessKey =
            nullOutputFacts.IsFactSet(kGOAPHasAccessKeyFact);

        nullOutputDecision->Update(
            nullOutputAI,
            GameplayAIObservationContext{
                world,
                {},
                nullptr
            });

        const bool hasAccessKey =
            nullOutputFacts.IsFactSet(kGOAPHasAccessKeyFact);

        if (!hadAccessKey && hasAccessKey)
        {
            nullOutputPurchaseConfirmed = true;

            EXPECT_EQ(
                nullOutputFacts.GetIntegerFact(kGOAPCoinCountFact),
                0);
        }
    }

    EXPECT_TRUE(nullOutputPurchaseConfirmed);
    nullOutputDecision->Cancel(nullOutputAI);
    
    // ---------------------------------------------------------------------
    // Existing cancellation / legacy reservation fallback coverage
    // ---------------------------------------------------------------------
    
    decision->Cancel(ai);
    world.RemoveInteractionPoint(coinB);
    world.RemoveInteractionPoint(coinC);
    auto legacyDecision = CreateAccessKeyAIDecision(
        agent, level, world, links, executors, &reservations);
    ASSERT_NE(legacyDecision, nullptr);
    ASSERT_TRUE(reservations.TryReserve(world, coinA, otherAgent));
    AISystem legacyAI;
    legacyDecision->Update(legacyAI, GameplayAIObservationContext{world, {}});
    EXPECT_TRUE(legacyDecision->GetObservedState().IsFactSet(kGOAPCoinAAvailableFact));
    EXPECT_FALSE(legacyDecision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
    ASSERT_TRUE(reservations.Release(coinA, otherAgent));
}