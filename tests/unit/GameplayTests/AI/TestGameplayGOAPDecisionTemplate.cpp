#include <gtest/gtest.h>

#include <algorithm>
#include <any>
#include <array>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

import core;

#include "TestSupport/GameplayGOAPAssetFixture.h"

using namespace rendern;
using goap_asset_test::Fixture;

namespace
{
    // This extension exists only in the test. No generic parser or variant knows it.
    struct IntegerGateAsset { std::string input, fact; std::int32_t minimum; };
    IntegerGateAsset ParseIntegerGate(const GameplayAIComponentParseContext& input)
    {
        using namespace ai_asset_detail;
        Fields(input.object, {"type", "input", "fact", "minimum"}, input.source, input.location);
        return {String(input.object, "input", input.source, input.location),
            String(input.object, "fact", input.source, input.location),
            static_cast<std::int32_t>(PositiveInteger(input.object, "minimum", INT32_MAX, input.source, input.location))};
    }
    class IntegerGate final : public IGameplayGOAPObservation
    {
    public:
        IntegerGate(const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context)
        {
            const auto& parameters = context.Parameters<IntegerGateAsset>(asset);
            input_ = context.IntegerFact(parameters.input);
            output_ = context.BooleanFact(parameters.fact);
            minimum_ = parameters.minimum;
        }
        std::vector<AIWorldFactId> BooleanOutputs() const override { return {output_}; }
        std::vector<AIWorldIntegerFactId> IntegerInputs() const override { return {input_}; }
        void Observe(const GameplayWorld&, EntityHandle, std::span<const GameplayWorldEvent>, AIAgentWorldState& facts) override
        {
            facts.SetFact(output_, facts.GetIntegerFact(input_) >= minimum_);
        }
        void ObserveActionEvents(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            Observe(world, agent, events, facts);
        }
    private:
        AIWorldIntegerFactId input_;
        AIWorldFactId output_;
        std::int32_t minimum_{};
    };

    void RegisterGate(GameplayGOAPCompositionRegistry& registry)
    {
        ASSERT_TRUE(registry.RegisterObservation<IntegerGateAsset>("test_integer_gate", ParseIntegerGate,
            [](const auto& asset, const auto& context) { return std::make_unique<IntegerGate>(asset, context); }));
    }

    GameplayAIObservationAsset GateAsset(const GameplayGOAPCompositionRegistry& registry)
    {
        return ParseGameplayAIBehaviorAsset(R"({"version":1,"id":"extension","model":"goap","definition":"unused",
            "observations":[{"type":"test_integer_gate","input":"coins","fact":"canAfford","minimum":2}],
            "capabilities":[]})", "extension.json", registry.AssetParsers()).observations.front();
    }

    class DependentIntegerWriter final : public IGameplayGOAPObservation
    {
    public:
        DependentIntegerWriter(AIWorldIntegerFactId output, AIWorldFactId input) : output_(output), input_(input) {}
        std::vector<AIWorldFactId> BooleanOutputs() const override { return {}; }
        std::vector<AIWorldIntegerFactId> IntegerOutputs() const override { return {output_}; }
        std::vector<AIWorldFactId> BooleanInputs() const override { return {input_}; }
        void Observe(const GameplayWorld&, EntityHandle, std::span<const GameplayWorldEvent>, AIAgentWorldState&) override {}
    private:
        AIWorldIntegerFactId output_;
        AIWorldFactId input_;
    };
}

TEST(GameplayGOAPDecisionTemplate, ComponentParserAndIntegerDependencyWorkInBothObservationPhases)
{
    Fixture fixture;
    RegisterGate(fixture.components);
    fixture.definition.facts.push_back({"canAfford", GameplayGOAPFactType::Boolean});
    // Deliberately put the consumer before the resource ledger.
    fixture.behavior.observations.insert(fixture.behavior.observations.begin(), GateAsset(fixture.components));
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
    const auto affordable = fixture.compiled.FindBooleanFact("canAfford").value();
    EXPECT_TRUE(inspection->GetObservedState().IsFactSet(affordable));
    decision->Update(fixture.ai, {fixture.world, {}}); // Synchronous purchase debits the ledger.
    EXPECT_TRUE(inspection->GetObservedState().IsFactSet(fixture.compiled.FindBooleanFact("hasAccessKey").value()));
    EXPECT_FALSE(inspection->GetObservedState().IsFactSet(affordable));
    decision->Cancel(fixture.ai);
}

TEST(GameplayGOAPDecisionTemplate, RejectsIntegerInputTypeErrorsAndMixedDependencyCycle)
{
    Fixture fixture;
    RegisterGate(fixture.components);
    fixture.definition.facts.push_back({"canAfford", GameplayGOAPFactType::Boolean});
    fixture.behavior.observations.push_back(GateAsset(fixture.components));
    auto& gate = std::any_cast<IntegerGateAsset&>(fixture.behavior.observations.back().parameters);
    gate.input = "missing";
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    gate.input = "hasAccessKey";
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    gate.input = "coins";
    fixture.behavior.observations.back().parameters = std::string{"wrong typed payload"};
    EXPECT_THROW(fixture.Create(), std::runtime_error);
    fixture.behavior.observations = {GateAsset(fixture.components)};
    fixture.definition.facts = {{"canAfford", GameplayGOAPFactType::Boolean}, {"coins", GameplayGOAPFactType::Integer}};
    fixture.definition.goals = {{"goal", 1, {{"canAfford", true}}}};
    fixture.definition.actions.clear();
    fixture.behavior.capabilities.clear();
    fixture.behavior.reactions.clear();
    ASSERT_TRUE(fixture.components.RegisterObservation<IntegerGateAsset>("test_integer_writer", ParseIntegerGate,
        [](const auto&, const GameplayGOAPCompositionContext& context)
        {
            return std::make_unique<DependentIntegerWriter>(context.IntegerFact("coins"), context.BooleanFact("canAfford"));
        }));
    fixture.behavior.observations.push_back({"test_integer_writer", IntegerGateAsset{}});
    EXPECT_THROW(fixture.Create(), std::runtime_error); // bool -> int -> bool cycle.
    EXPECT_FALSE(fixture.ai.HasActiveAction(fixture.agent));
}

TEST(GameplayGOAPDecisionTemplate, SharedTemplateKeepsWorldBindingsStateAndCostsPrivate)
{
    Fixture first;
    Fixture second;
    const auto prepared = std::make_shared<const GameplayGOAPDecisionTemplate>(first.behavior,
        first.bindings, first.definition, first.components, first.graph);
    const auto baseline = prepared->Compiled().definition.actions.front().baseCost;
    // The second world binds the same roles to different physical positions.
    for (auto& node : second.level.nodes)
    {
        node.transform.position = node.transform.position * 2.0f;
    }
    auto a = CreateGameplayGOAPDecisionFromTemplate(first.services, *prepared);
    auto b = CreateGameplayGOAPDecisionFromTemplate(second.services, *prepared);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    auto* ai = dynamic_cast<IGameplayGOAPInspection*>(a.get());
    auto* bi = dynamic_cast<IGameplayGOAPInspection*>(b.get());
    ASSERT_NE(ai, nullptr);
    ASSERT_NE(bi, nullptr);
    a->Update(first.ai, {first.world, {}});
    b->Update(second.ai, {second.world, {}});
    const auto firstPlan = ai->BuildDebugViewModel();
    const auto secondPlan = bi->BuildDebugViewModel();
    ASSERT_FALSE(firstPlan.selectedPlan.empty());
    ASSERT_FALSE(secondPlan.selectedPlan.empty());
    EXPECT_EQ(firstPlan.selectedPlan.front().contextId, secondPlan.selectedPlan.front().contextId);
    EXPECT_NEAR(secondPlan.selectedPlan.front().cost, firstPlan.selectedPlan.front().cost * 2, .001f);
    const std::array credit{GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
        first.agent, first.Role("coinA").entity}};
    a->Update(first.ai, {first.world, credit});
    EXPECT_EQ(ai->GetObservedState().GetIntegerFact(first.compiled.FindIntegerFact("coins").value()), 1);
    EXPECT_EQ(bi->GetObservedState().GetIntegerFact(second.compiled.FindIntegerFact("coins").value()), 0);
    EXPECT_EQ(prepared->Compiled().definition.actions.front().baseCost, baseline);
    // Destroy all source authoring/callback state; providers remain instance-owned.
    first.behavior = {};
    first.components = {};
    a->Cancel(first.ai);
    b->Cancel(second.ai);
}

TEST(GameplayGOAPDecisionTemplate, InstanceCostOverridesMatchCompilationAndDoNotMutateTemplate)
{
    Fixture fixture;
    const GameplayGOAPDecisionTemplate prepared{fixture.behavior, fixture.bindings,
        fixture.definition, fixture.components, fixture.graph};
    auto movement = fixture.Capability("move_to");
    const auto overrides = movement->CostOverrides();
    const auto patched = prepared.InstanceDefinition(overrides);
    const auto recompiled = CompileGameplayGOAPDefinition(fixture.definition, fixture.components.SemanticActions(), overrides);
    ASSERT_EQ(patched.actions.size(), recompiled.definition.actions.size());
    for (std::size_t index = 0; index < patched.actions.size(); ++index)
    {
        EXPECT_EQ(patched.actions[index].actionId, recompiled.definition.actions[index].actionId);
        EXPECT_EQ(patched.actions[index].contextId, recompiled.definition.actions[index].contextId);
        EXPECT_FLOAT_EQ(patched.actions[index].baseCost, recompiled.definition.actions[index].baseCost);
    }
    auto invalid = overrides;
    ASSERT_FALSE(invalid.empty());
    invalid.push_back(invalid.front());
    EXPECT_THROW(prepared.InstanceDefinition(invalid), std::runtime_error);
    invalid = overrides;
    invalid.front().cost = std::numeric_limits<float>::quiet_NaN();
    EXPECT_THROW(prepared.InstanceDefinition(invalid), std::runtime_error);
    invalid.front().cost = -1;
    EXPECT_THROW(prepared.InstanceDefinition(invalid), std::runtime_error);
    invalid.front().cost = 1;
    invalid.front().context = "missing";
    EXPECT_THROW(prepared.InstanceDefinition(invalid), std::runtime_error);
    EXPECT_EQ(prepared.InstanceDefinition({}).actions.front().baseCost, fixture.compiled.definition.actions.front().baseCost);
}
