#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <string>
#include <vector>

import core;

#include "TestSupport/GameplayGOAPAssetFixture.h"

using namespace rendern;
using goap_asset_test::Fixture;

namespace
{
    void Observe(Fixture& fixture, std::vector<std::unique_ptr<IGameplayGOAPObservation>>& observers,
        AIAgentWorldState& facts, std::span<const GameplayWorldEvent> events = {})
    {
        for (auto& observer : observers)
        {
            observer->Observe(fixture.world, fixture.agent, events, facts);
        }
    }
    void RegisterJump(Fixture& fixture)
    {
        ASSERT_TRUE(fixture.links.Register({.handle = GameplayTraversalLinkHandle{9470001u},
            .traversalTypeId = kJumpTraversalTypeId, .targetEntity = fixture.Role("landing").entity,
            .jump = {.takeoffPosition = fixture.Role("takeoff").position,
                .landingPosition = fixture.Role("landing").position, .verticalSpeed = 5.5f,
                .takeoffTolerance = .2f, .landingHorizontalTolerance = .55f, .landingVerticalTolerance = .3f}}));
    }
}

TEST(GameplayGOAPResources, CreditsOnlyMatchingAgentSubjectAndAcknowledgesOnce)
{
    Fixture fixture;
    auto observers = fixture.Observers();
    AIAgentWorldState facts;
    const auto coin = fixture.Role("coinA").entity;
    const auto shop = fixture.Role("shop").entity;
    const auto resource = fixture.compiled.FindIntegerFact("coins").value();
    const auto collected = fixture.compiled.FindBooleanFact("coinACollected").value();
    const auto purchased = fixture.compiled.FindBooleanFact("hasAccessKey").value();
    const auto other = fixture.world.CreateEntity();
    fixture.world.AddAI(other);
    const std::array ignored{GameplayWorldEvent{GameplayWorldEventType::PickupCollected, other, coin},
        GameplayWorldEvent{GameplayWorldEventType::PickupCollected, fixture.agent, other}};
    Observe(fixture, observers, facts, ignored);
    EXPECT_EQ(facts.GetIntegerFact(resource), 0);
    const std::array credit{GameplayWorldEvent{GameplayWorldEventType::PickupCollected, fixture.agent, coin}};
    Observe(fixture, observers, facts, credit);
    Observe(fixture, observers, facts, credit);
    EXPECT_EQ(facts.GetIntegerFact(resource), 1);
    EXPECT_TRUE(facts.IsFactSet(collected));
    facts.SetIntegerFact(resource, 4);
    const std::array wrongPurchase{GameplayWorldEvent{GameplayWorldEventType::ResourcePurchased, other, shop},
        GameplayWorldEvent{GameplayWorldEventType::ResourcePurchased, fixture.agent, other}};
    Observe(fixture, observers, facts, wrongPurchase);
    EXPECT_FALSE(facts.IsFactSet(purchased));
    const std::array purchase{GameplayWorldEvent{GameplayWorldEventType::ResourcePurchased, fixture.agent, shop}};
    Observe(fixture, observers, facts, purchase);
    Observe(fixture, observers, facts, purchase);
    EXPECT_TRUE(facts.IsFactSet(purchased));
    EXPECT_EQ(facts.GetIntegerFact(resource), 2);
}

TEST(GameplayGOAPResources, RejectsOverflowWithoutAcknowledgingCredit)
{
    Fixture fixture;
    auto observers = fixture.Observers();
    AIAgentWorldState facts;
    const auto resource = fixture.compiled.FindIntegerFact("coins").value();
    facts.SetIntegerFact(resource, std::numeric_limits<std::int32_t>::max());
    const std::array credit{GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
        fixture.agent, fixture.Role("coinA").entity}};
    Observe(fixture, observers, facts, credit);
    EXPECT_EQ(facts.GetIntegerFact(resource), std::numeric_limits<std::int32_t>::max());
    EXPECT_FALSE(facts.IsFactSet(fixture.compiled.FindBooleanFact("coinACollected").value()));
}

TEST(GameplayGOAPResources, AvailabilityDistinguishesSelfReservationFromOtherAgent)
{
    Fixture fixture("access_key_reserved");
    auto observers = fixture.Observers();
    AIAgentWorldState facts;
    const auto coin = fixture.Role("coinA").entity;
    const auto available = fixture.compiled.FindBooleanFact("coinAAvailable").value();
    const auto other = fixture.world.CreateEntity();
    fixture.world.AddAI(other);
    Observe(fixture, observers, facts);
    EXPECT_TRUE(facts.IsFactSet(available));
    ASSERT_TRUE(fixture.reservations.TryReserve(fixture.world, coin, fixture.agent));
    Observe(fixture, observers, facts);
    EXPECT_TRUE(facts.IsFactSet(available));
    ASSERT_TRUE(fixture.reservations.Release(coin, fixture.agent));
    ASSERT_TRUE(fixture.reservations.TryReserve(fixture.world, coin, other));
    Observe(fixture, observers, facts);
    EXPECT_FALSE(facts.IsFactSet(available));
    ASSERT_TRUE(fixture.reservations.Release(coin, other));
    Observe(fixture, observers, facts);
    EXPECT_TRUE(facts.IsFactSet(available));
    fixture.world.TryGetPickup(coin)->collected = true;
    Observe(fixture, observers, facts);
    EXPECT_FALSE(facts.IsFactSet(available));
}

TEST(GameplayGOAPResources, KeepsOneConfirmedLocationDuringTransitAndGatesDestination)
{
    Fixture fixture;
    auto observers = fixture.Observers();
    AIAgentWorldState facts;
    const std::array names{"atStart", "atCoinA", "atCoinB", "atCoinC", "atAccessKeyShop", "atGoal"};
    const std::array roles{"start", "coinA", "coinB", "coinC", "shop", "goal"};
    for (std::size_t selected = 0; selected < roles.size(); ++selected)
    {
        fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role(roles[selected]).position;
        Observe(fixture, observers, facts);
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            EXPECT_EQ(facts.IsFactSet(fixture.compiled.FindBooleanFact(names[index]).value()), index == selected);
        }
    }
    const auto goal = fixture.compiled.FindBooleanFact("atDestination").value();
    EXPECT_FALSE(facts.IsFactSet(goal));
    facts.SetFact(fixture.compiled.FindBooleanFact("hasAccessKey").value());
    Observe(fixture, observers, facts);
    EXPECT_TRUE(facts.IsFactSet(goal));
    fixture.world.TryGetTransform(fixture.agent)->position = {100, 0, 100};
    Observe(fixture, observers, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.compiled.FindBooleanFact("atGoal").value()));
    EXPECT_TRUE(facts.IsFactSet(goal));
}

TEST(GameplayGOAPResources, PurchaseChecksFundsDistanceAndConfirmsByEvent)
{
    Fixture fixture;
    auto provider = fixture.Capability("purchase");
    AIAgentWorldState facts;
    std::vector<GameplayWorldEvent> events;
    auto binding = provider->CreateBinding(facts, events);
    const AIActionRuntimeContext context{fixture.agent, kAIPurchaseActionId,
        fixture.compiled.FindActionContext("buy_key").value()};
    const auto resource = fixture.compiled.FindIntegerFact("coins").value();
    const auto purchased = fixture.compiled.FindBooleanFact("hasAccessKey").value();
    auto runtime = binding->CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Failed);
    facts.SetIntegerFact(resource, 2);
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(events.empty());
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("shop").position;
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Succeeded);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events.front().type, GameplayWorldEventType::ResourcePurchased);
    EXPECT_FALSE(facts.IsFactSet(purchased));
    EXPECT_EQ(facts.GetIntegerFact(resource), 2);
    auto observers = fixture.Observers();
    Observe(fixture, observers, facts, events);
    Observe(fixture, observers, facts, events);
    EXPECT_TRUE(facts.IsFactSet(purchased));
    EXPECT_EQ(facts.GetIntegerFact(resource), 0);
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Failed);
    EXPECT_EQ(events.size(), 1u);
}

TEST(GameplayGOAPResources, DependencyOrderingMakesReceiptVisibleToGoalInSameUpdate)
{
    Fixture fixture;
    std::ranges::reverse(fixture.behavior.observations);
    auto decision = fixture.Create();
    ASSERT_NE(decision, nullptr);
    auto* inspection = dynamic_cast<IGameplayGOAPInspection*>(decision.get());
    ASSERT_NE(inspection, nullptr);
    auto& facts = const_cast<AIAgentWorldState&>(inspection->GetObservedState());
    facts.SetIntegerFact(fixture.compiled.FindIntegerFact("coins").value(), 2);
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("goal").position;
    const std::array event{GameplayWorldEvent{GameplayWorldEventType::ResourcePurchased,
        fixture.agent, fixture.Role("shop").entity}};
    decision->Update(fixture.ai, {fixture.world, event});
    EXPECT_TRUE(facts.IsFactSet(fixture.compiled.FindBooleanFact("atDestination").value()));
    EXPECT_EQ(decision->GetStatus(), GameplayAIDecisionStatus::Succeeded);
    decision->Cancel(fixture.ai);
}

TEST(GameplayGOAPResources, InvalidReceiptsPriceAndDependenciesRejectBeforeStart)
{
    Fixture fixture;
    auto& ledger = std::get<GameplayAIResourceLedgerAsset>(fixture.behavior.observations.front().parameters);
    ledger.receipts.front().price = 3;
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    ledger.receipts.front().price = 2;
    ledger.receipts.push_back(ledger.receipts.front());
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    ledger.receipts.pop_back();
    auto& purchase = std::get<GameplayAIPurchaseAsset>(fixture.behavior.capabilities.back().parameters);
    purchase.receipt = "unknown";
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    purchase.receipt = "unlock";
    auto& goal = std::get<GameplayAISpatialObservationAsset>(fixture.behavior.observations.back().parameters);
    goal.requiredFacts = {"atDestination"};
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    EXPECT_FALSE(fixture.ai.HasActiveAction(fixture.agent));
}

TEST(GameplayGOAPResources, RouteCostsChooseTwoDistinctCoinsAndReplanWhenUnavailable)
{
    Fixture fixture;
    const auto compiled = fixture.WithRouteCosts();
    AIAgentWorldState facts;
    facts.SetFact(compiled.FindBooleanFact("atStart").value());
    for (auto name : {"coinAAvailable", "coinBAvailable", "coinCAvailable"})
    {
        facts.SetFact(compiled.FindBooleanFact(name).value());
    }
    const auto plan = FindAIPlan(facts, compiled.definition.goals.front().goal, compiled.definition.actions);
    ASSERT_TRUE(plan);
    ASSERT_EQ(plan->steps.size(), 5u);
    EXPECT_EQ(plan->steps[0].contextId, compiled.FindActionContext("start_to_coin_c").value());
    EXPECT_EQ(plan->steps[1].contextId, compiled.FindActionContext("coin_c_to_coin_a").value());
    EXPECT_EQ(plan->steps[3].actionId, kAIPurchaseActionId);
    for (const auto& step : plan->steps)
    {
        const auto action = std::ranges::find(compiled.definition.actions, step.contextId, &AIActionDefinition::contextId);
        ASSERT_NE(action, compiled.definition.actions.end());
        EXPECT_TRUE(AreFactConditionsSatisfied(facts, action->preconditions));
        EXPECT_TRUE(AreNumericConditionsSatisfied(facts, action->numericPreconditions));
        ApplyFactEffects(facts, action->effects);
        EXPECT_TRUE(ApplyNumericEffects(facts, action->numericEffects));
    }
    EXPECT_TRUE(facts.IsFactSet(compiled.FindBooleanFact("coinACollected").value()));
    EXPECT_FALSE(facts.IsFactSet(compiled.FindBooleanFact("coinBCollected").value()));
    EXPECT_TRUE(facts.IsFactSet(compiled.FindBooleanFact("atDestination").value()));
    EXPECT_EQ(facts.GetIntegerFact(compiled.FindIntegerFact("coins").value()), 0);
    facts = {};
    facts.SetFact(compiled.FindBooleanFact("atCoinC").value());
    facts.SetFact(compiled.FindBooleanFact("coinCCollected").value());
    facts.SetFact(compiled.FindBooleanFact("coinBAvailable").value());
    facts.SetIntegerFact(compiled.FindIntegerFact("coins").value(), 1);
    const auto alternative = FindAIPlan(facts, compiled.definition.goals.front().goal, compiled.definition.actions);
    ASSERT_TRUE(alternative);
    EXPECT_EQ(alternative->steps.front().contextId, compiled.FindActionContext("coin_c_to_coin_b").value());
}

TEST(GameplayGOAPResources, MovementChecksSourceAndReservesOnlyConfiguredContexts)
{
    Fixture fixture("access_key_reserved");
    auto capability = fixture.Capability("move_to");
    auto* requests = dynamic_cast<IAIMoveToActionRequestProvider*>(capability.get());
    ASSERT_NE(requests, nullptr);
    const AIActionRuntimeContext context{fixture.agent, kAIMoveToActionId,
        fixture.compiled.FindActionContext("coin_a_to_coin_c").value()};
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("coinB").position;
    EXPECT_FALSE(requests->ResolveRequest(context));
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("coinA").position;
    ASSERT_TRUE(requests->ResolveRequest(context));
    fixture.world.TryGetTransform(fixture.agent)->position += mathUtils::Vec3{0.7f, 0, 0};
    EXPECT_FALSE(requests->ResolveRequest(context));
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("coinA").position;
    AIAgentWorldState facts;
    std::vector<GameplayWorldEvent> events;
    auto binding = capability->CreateBinding(facts, events);
    auto runtime = binding->CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Running);
    EXPECT_TRUE(fixture.reservations.IsReservedBy(fixture.Role("coinC").entity, fixture.agent));
    runtime->Cancel(context);
    EXPECT_FALSE(fixture.reservations.IsReserved(fixture.Role("coinC").entity));
}

TEST(GameplayGOAPResources, JumpGraphRetainsCostsAnnotationsAndRunningRequest)
{
    Fixture fixture("access_key_jump");
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    RegisterJump(fixture);
    auto movement = fixture.Capability("move_to");
    const auto id = fixture.compiled.FindActionContext("access_key_shop_to_goal").value();
    const auto* paths = dynamic_cast<const IGameplayGOAPActionPathProvider*>(movement.get());
    ASSERT_NE(paths, nullptr);
    auto route = paths->BuildDebugRoute(id);
    ASSERT_TRUE(route);
    ASSERT_EQ(route->segmentAnnotations.size(), 3u);
    EXPECT_FALSE(route->segmentAnnotations[0].traversalLink.has_value());
    EXPECT_EQ(route->segmentAnnotations[1].traversalLink, GameplayTraversalLinkHandle{9470001u});
    EXPECT_FALSE(route->segmentAnnotations[2].traversalLink.has_value());
    auto* requests = dynamic_cast<IAIMoveToActionRequestProvider*>(movement.get());
    ASSERT_NE(requests, nullptr);
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("shop").position;
    const auto request = requests->ResolveRequest({fixture.agent, kAIMoveToActionId, id});
    ASSERT_TRUE(request);
    EXPECT_TRUE(request->steeringSettings.wantsRun);
    const auto compiled = fixture.WithRouteCosts();
    const auto action = std::ranges::find(compiled.definition.actions, id, &AIActionDefinition::contextId);
    ASSERT_NE(action, compiled.definition.actions.end());
    const float length = mathUtils::Length(fixture.Role("takeoff").position - fixture.Role("shop").position)
        + mathUtils::Length(fixture.Role("landing").position - fixture.Role("takeoff").position)
        + mathUtils::Length(fixture.Role("goal").position - fixture.Role("landing").position);
    EXPECT_NEAR(action->baseCost, length, .001f);
    auto decision = fixture.Create();
    auto* inspection = dynamic_cast<IGameplayGOAPPathInspection*>(decision.get());
    ASSERT_NE(inspection, nullptr);
    EXPECT_TRUE(inspection->BuildPlannedPathDebugView().routeSteps.empty());
    auto* goap = dynamic_cast<IGameplayGOAPInspection*>(decision.get());
    ASSERT_NE(goap, nullptr);
    auto& facts = const_cast<AIAgentWorldState&>(goap->GetObservedState());
    facts.SetFact(fixture.compiled.FindBooleanFact("hasAccessKey").value());
    decision->Update(fixture.ai, {fixture.world, {}});
    const auto remaining = inspection->BuildPlannedPathDebugView();
    ASSERT_EQ(remaining.routeSteps.size(), 1u);
    EXPECT_EQ(remaining.routeSteps.front().planStepIndex, 0u);
    EXPECT_EQ(remaining.routeSteps.front().contextId, id);
    ASSERT_TRUE(remaining.routeSteps.front().route);
    EXPECT_EQ(remaining.routeSteps.front().route->segmentAnnotations[1].traversalLink,
        GameplayTraversalLinkHandle{9470001u});
    decision->Cancel(fixture.ai);
    fixture.graph.edges.pop_back();
    EXPECT_THROW(fixture.Create(), std::runtime_error);
}

TEST(GameplayGOAPResources, RenamedResourceAndDifferentPriceReuseTheSameRuntime)
{
    Fixture fixture;
    auto& ledger = std::get<GameplayAIResourceLedgerAsset>(fixture.behavior.observations.front().parameters);
    ledger.fact = "tokens";
    ledger.receipts.front().id = "permit";
    ledger.receipts.front().fact = "gatePass";
    ledger.receipts.front().price = 3;
    for (auto& pickup : ledger.pickups)
    {
        pickup.amount = 2;
    }
    std::get<GameplayAIPurchaseAsset>(fixture.behavior.capabilities.back().parameters).receipt = "permit";
    std::get<GameplayAISpatialObservationAsset>(fixture.behavior.observations.back().parameters).requiredFacts = {"gatePass"};
    const auto rename = [](std::string& name)
    {
        if (name == "coins")
        {
            name = "tokens";
        }
        else if (name == "hasAccessKey")
        {
            name = "gatePass";
        }
    };
    for (auto& fact : fixture.definition.facts)
    {
        rename(fact.name);
    }
    for (auto& action : fixture.definition.actions)
    {
        for (auto& condition : action.preconditions)
        {
            rename(condition.fact);
        }
        for (auto& effect : action.effects)
        {
            rename(effect.fact);
        }
        for (auto& condition : action.numericPreconditions)
        {
            rename(condition.fact);
            condition.value = 3;
        }
        for (auto& effect : action.numericEffects)
        {
            rename(effect.fact);
            effect.value = effect.value < 0 ? -3 : 2;
        }
    }
    fixture.Refresh();
    auto decision = fixture.Create();
    ASSERT_NE(decision, nullptr);
    auto* inspection = dynamic_cast<IGameplayGOAPInspection*>(decision.get());
    ASSERT_NE(inspection, nullptr);
    fixture.world.TryGetTransform(fixture.agent)->position = fixture.Role("shop").position;
    const std::array credits{
        GameplayWorldEvent{GameplayWorldEventType::PickupCollected, fixture.agent, fixture.Role("coinA").entity},
        GameplayWorldEvent{GameplayWorldEventType::PickupCollected, fixture.agent, fixture.Role("coinB").entity}};
    decision->Update(fixture.ai, {fixture.world, credits});
    decision->Update(fixture.ai, {fixture.world, {}});
    EXPECT_TRUE(inspection->GetObservedState().IsFactSet(fixture.compiled.FindBooleanFact("gatePass").value()));
    EXPECT_EQ(inspection->GetObservedState().GetIntegerFact(fixture.compiled.FindIntegerFact("tokens").value()), 1);
    decision->Cancel(fixture.ai);
}
