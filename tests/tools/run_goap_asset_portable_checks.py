#!/usr/bin/env python3
"""Run authoring, templates and dependency ordering on GCC without the MSVC build.

This is not a replacement for runtime GoogleTests. It removes module declarations,
retains only the actual entity aliases from EnTTHelpers (no ECS implementation),
and renames a private helper to avoid a collision caused by amalgamation.
Component parser functions and preparation/ordering logic are production code;
component runtime factory callbacks are inert, and are never invoked here.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    'src/Core/Math/MathUtils.cppm',
    'src/Core/String/StringUtils.cppm',
    'src/Core/Json/JsonUtils.cppm',
    'src/Core/IO/FileUtils.cppm',
    'src/Render/FileSystem.cppm',
    'src/Gameplay/AI/Runtime/AIActionContracts.cppm',
    'src/Gameplay/AI/GOAP/AIAgentWorldState.cppm',
    'src/Gameplay/AI/GOAP/AIDecisionContracts.cppm',
    'src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm',
    'src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm',
    'src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm',
    'src/Gameplay/AI/GOAP/Authoring/GameplayGOAPDefinitionCompiler.cppm',
    'src/Gameplay/AI/GOAP/Authoring/GameplayAIAssetParsing.cppm',
    'src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm',
    'src/Gameplay/AI/Composition/Components/GameplayGOAPSpatialComponentAssets.cppm',
    'src/Gameplay/AI/Composition/Components/GameplayGOAPResourceComponentAssets.cppm',
    'src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponentAssets.cppm',
    'src/ECS/EnTTHelpers.cppm',
    'src/Gameplay/AI/Runtime/AIActionRuntime.cppm',
    'src/Gameplay/AI/Runtime/AIActionBinding.cppm',
    'src/Gameplay/Events/GameplayWorldEvent.cppm',
    'src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm',
    'src/Gameplay/AI/GOAP/GameplayGOAPDecisionSetup.cppm',
    'src/Gameplay/AI/Runtime/GameplayAIDecisionCreationContext.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPObservationOrder.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPDecisionTemplate.cppm',
]

def parser_registrations():
    result = []
    registrations = []
    for name in ('SpatialComponents', 'ResourceComponents', 'MoveToComponent'):
        source = (ROOT / f'src/Gameplay/AI/Composition/Components/GameplayGOAP{name}.cppm').read_text()
        pattern = r'Register(Observation|Capability|Reaction)<(\w+)>\("([^"]+)",\s*(?:(\w+),\s*)?(Parse\w+),'
        for kind, payload, key, action_id, parse in re.findall(pattern, source):
            result.append(f'    if (!parsers.Register<{payload}>(GameplayAIComponentKind::{kind}, "{key}", {parse})) {{ throw std::runtime_error("Duplicate parser"); }}')
            registration = f'components.Register{kind}<{payload}>("{key}", '
            if kind == 'Capability':
                # Read the semantic ID from its implementation instead of duplicating values.
                candidates = [source, (ROOT / 'src/Gameplay/AI/Navigation/AIMoveToAction.cppm').read_text(encoding='utf-8-sig')]
                value = next(re.search(r'AIActionId\s+' + action_id + r'\s*\{([^}]+)\}', text)[1]
                             for text in candidates if re.search(r'AIActionId\s+' + action_id + r'\s*\{([^}]+)\}', text))
                registration += 'AIActionId{' + value + '}, '
            interface = {'Observation': 'IGameplayGOAPObservation', 'Capability': 'IGameplayGOAPCapability',
                         'Reaction': 'IGameplayGOAPEventReaction'}[kind]
            registration += parse + f', [](const auto&, const auto&) -> std::unique_ptr<{interface}> {{ return nullptr; }})'
            registrations.append('    if (!' + registration + ') { throw std::runtime_error("Duplicate component"); }')
    assert len(result) >= 8
    return 'GameplayAIComponentParsers MakeParsers() {\n    GameplayAIComponentParsers parsers;\n' + '\n'.join(result) + '\n    return parsers;\n}\n' + 'GameplayGOAPCompositionRegistry MakeComponents() {\n    GameplayGOAPCompositionRegistry components;\n' + '\n'.join(registrations) + '\n    return components;\n}\n'

HARNESS = r'''
using namespace rendern;
GameplayAIBehaviorAsset ParseBehavior(std::string_view json, std::string_view source)
{
    return ParseGameplayAIBehaviorAsset(json, source, MakeParsers());
}
int checked = 0;
void Check(bool ok) {
    ++checked;
    if (!ok) { throw std::runtime_error("Check failed: " + std::to_string(checked)); }
}
template<class F> void Reject(F&& operation, std::string_view message) {
    try { operation(); }
    catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()).find(message) != std::string_view::npos);
        return;
    }
    throw std::runtime_error("Expected rejection: " + std::string(message));
}
struct ExtensionPayload { std::string value; };
struct MetadataObserver final : IGameplayGOAPObservation
{
    std::vector<AIWorldFactId> boolOut, boolIn;
    std::vector<AIWorldIntegerFactId> intOut, intIn;
    std::vector<AIWorldFactId> BooleanOutputs() const override { return boolOut; }
    std::vector<AIWorldFactId> BooleanInputs() const override { return boolIn; }
    std::vector<AIWorldIntegerFactId> IntegerOutputs() const override { return intOut; }
    std::vector<AIWorldIntegerFactId> IntegerInputs() const override { return intIn; }
    void Observe(const GameplayWorld&, EntityHandle, std::span<const GameplayWorldEvent>, AIAgentWorldState&) override {}
};
void CheckExtensionsAndOrdering()
{
    auto components = MakeComponents();
    int parses = 0;
    const auto parser = [&](const GameplayAIComponentParseContext& input)
    {
        ++parses;
        ai_asset_detail::Fields(input.object, {"type", "value"}, input.source, input.location);
        return ExtensionPayload{ai_asset_detail::String(input.object, "value", input.source, input.location)};
    };
    const auto compiler = [](const auto&, const auto&) -> std::unique_ptr<IGameplayGOAPObservation> { return nullptr; };
    Check(components.RegisterObservation<ExtensionPayload>("external_component", parser, compiler));
    Check(!components.RegisterObservation<ExtensionPayload>("external_component", parser, compiler));
    auto behavior = ParseGameplayAIBehaviorAsset(R"({"version":1,"id":"x","model":"goap","definition":"x",
        "observations":[{"type":"external_component","value":"owned"}],"capabilities":[]})", "external.json", components.AssetParsers());
    Check(parses == 1 && std::any_cast<ExtensionPayload&>(behavior.observations.front().parameters).value == "owned");
    auto copy = behavior;
    std::any_cast<ExtensionPayload&>(copy.observations.front().parameters).value = "changed";
    Check(std::any_cast<ExtensionPayload&>(behavior.observations.front().parameters).value == "owned");
    Reject([&] { (void)ParseGameplayAIBehaviorAsset(R"({"version":1,"id":"x","model":"goap","definition":"x",
        "observations":[{"type":"external_component","value":"x","typo":true}],"capabilities":[]})", "external.json", components.AssetParsers()); }, "typo");

    GameplayGOAPCompiledDefinition compiled;
    compiled.definition.metadata.booleanFacts = {{AIWorldFactId{0}, "bool"}};
    compiled.definition.metadata.integerFacts = {{AIWorldIntegerFactId{0}, "int"}};
    const auto nodes = [](bool cycle, bool unknown, bool duplicate)
    {
        std::vector<std::unique_ptr<IGameplayGOAPObservation>> result;
        auto consumer = std::make_unique<MetadataObserver>();
        consumer->boolOut = {AIWorldFactId{0}};
        consumer->intIn = {AIWorldIntegerFactId{static_cast<std::uint8_t>(unknown ? 7 : 0)}};
        auto producer = std::make_unique<MetadataObserver>();
        producer->intOut = {AIWorldIntegerFactId{0}};
        if (cycle) { producer->boolIn = {AIWorldFactId{0}}; }
        if (duplicate) { consumer->intOut = {AIWorldIntegerFactId{0}}; }
        result.push_back(std::move(consumer));
        result.push_back(std::move(producer));
        return result;
    };
    auto ordered = OrderGameplayGOAPObservations(nodes(false, false, false), compiled, "order");
    Check(ordered.size() == 2 && !ordered.front()->IntegerOutputs().empty());
    Reject([&] { (void)OrderGameplayGOAPObservations(nodes(true, false, false), compiled, "cycle"); }, "cyclic");
    Reject([&] { (void)OrderGameplayGOAPObservations(nodes(false, true, false), compiled, "unknown"); }, "unresolved");
    Reject([&] { (void)OrderGameplayGOAPObservations(nodes(false, false, true), compiled, "duplicate"); }, "multiple observation writers");
    auto missing = nodes(false, false, false);
    missing.pop_back();
    Reject([&] { (void)OrderGameplayGOAPObservations(std::move(missing), compiled, "missing"); }, "no observation writer");
}
int main() {
    CheckExtensionsAndOrdering();
    const std::vector semantics{GameplayGOAPSemanticAction{"move_to", AIActionId{2u}}, GameplayGOAPSemanticAction{"purchase", AIActionId{3u}}};
    auto catalog = LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json");
    Check(catalog.decisions.size() == 5);
    for (const auto& reference : catalog.decisions) {
        auto behavior = LoadGameplayAIBehaviorAsset(reference.behavior, MakeParsers());
        auto bindings = LoadGameplayAILevelBindingsAsset(reference.bindings);
        auto definition = LoadGameplayGOAPDefinitionAsset(behavior.definition);
        auto compiled = CompileGameplayGOAPDefinition(definition, semantics);
        Check(behavior.model == "goap");
        std::optional<GameplayAIRouteGraphAsset> graphInput;
        if (!behavior.routeGraph.empty()) { graphInput = LoadGameplayAIRouteGraphAsset(behavior.routeGraph); }
        const GameplayGOAPDecisionTemplate prepared{behavior, bindings, definition, MakeComponents(), graphInput};
        Check(prepared.Compiled().definition.actions.size() == compiled.definition.actions.size());
        const auto& actionName = definition.actions.front();
        std::vector<GameplayGOAPActionCostOverride> costs{{actionName.action, actionName.context, 9.25f}};
        for (int index = 0; index < 100; ++index)
        {
            const auto instance = prepared.InstanceDefinition(costs);
            Check(instance.actions.front().baseCost == 9.25f);
        }
        const auto recompiled = CompileGameplayGOAPDefinition(definition, semantics, costs);
        Check(recompiled.definition.actions.front().baseCost == prepared.InstanceDefinition(costs).actions.front().baseCost);
        Check(prepared.Compiled().definition.actions.front().baseCost == compiled.definition.actions.front().baseCost);
        costs.push_back(costs.front());
        Reject([&] { (void)prepared.InstanceDefinition(costs); }, "duplicate");
        costs.pop_back();
        costs.front().cost = -1;
        Reject([&] { (void)prepared.InstanceDefinition(costs); }, "finite and non-negative");
        costs.front().cost = std::numeric_limits<float>::quiet_NaN();
        Reject([&] { (void)prepared.InstanceDefinition(costs); }, "finite and non-negative");
        costs.front().cost = 1;
        costs.front().context = "absent";
        Reject([&] { (void)prepared.InstanceDefinition(costs); }, "does not match");
        if (!behavior.routeGraph.empty()) {
            const auto graph = LoadGameplayAIRouteGraphAsset(behavior.routeGraph);
            Check(graph.nodes.size() == bindings.roles.size());
            Check(graph.edges.size() >= 13);
            Check(behavior.inspectPath);
            Check(compiled.definition.actions.size() == 14);
            const auto& ledger = std::any_cast<GameplayAIResourceLedgerAsset&>(behavior.observations.front().parameters);
            Check(ledger.pickups.size() == 3);
            Check(ledger.receipts.front().price == 2);
            Check(compiled.FindIntegerFact(ledger.fact).has_value());
            const auto& purchase = std::any_cast<GameplayAIPurchaseAsset&>(behavior.capabilities.back().parameters);
            Check(purchase.receipt == ledger.receipts.front().id);
            Check(compiled.definition.actions.back().actionId == AIActionId{3u});
            Check(behavior.reactions.size() == 1);
            const auto& hide = std::any_cast<GameplayAIHideOnPurchaseAsset&>(behavior.reactions.front().parameters);
            Check(hide.receipt == ledger.receipts.front().id && hide.target == "shop");
            continue;
        }
        Check(behavior.reactions.empty());
        Check(bindings.roles.size() == 2);
        Check(behavior.observations.size() == 2);
        Check(behavior.capabilities.size() == 1);
        Check(compiled.definition.actions.size() == 1);
        const auto& action = compiled.definition.actions.front();
        Check(action.continuationConditions.size() == 1);
        Check(action.continuationConditions.front().bExpectedValue);
        Check(action.continuationConditions.front().factId ==
              compiled.FindBooleanFact(std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.front().parameters).fact).value());
        Check(action.contextId == compiled.FindActionContext(behavior.capabilities.front().context).value());
        Check(std::any_cast<GameplayAIMoveToAsset&>(behavior.capabilities.front().parameters).acceptanceRadius == std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.back().parameters).radius);
        definition.actions.front().continuationConditions.front().fact = "missing_fact";
        Reject([&] { (void)CompileGameplayGOAPDefinition(definition, semantics); }, "missing_fact");
    }
    Reject([] { (void)ParseGameplayAIDecisionCatalogAsset(
        R"({"version":2,"decisions":[]})", "catalog.json"); }, "catalog.json");
    Reject([] { (void)ParseGameplayAIDecisionCatalogAsset(
        R"({"version":1,"decisions":[{"id":"a","behavior":"b","bindings":"c"},{"id":"a","behavior":"d","bindings":"e"}]})",
        "catalog.json"); }, "duplicate decision id");
    Reject([] { (void)ParseGameplayAILevelBindingsAsset(
        R"({"version":1,"roles":[{"role":"goal","node":"a"},{"role":"goal","node":"b"}]})",
        "bindings.json"); }, "duplicate role");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"typo":true})",
        "behavior.json"); }, "unknown field 'typo'");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[{"type":"within_distance","target":"g","fact":"at","radius":"far"}],"capabilities":[]})",
        "behavior.json"); }, "radius");
    Reject([] { (void)ParseBehavior("{", "broken.json"); }, "broken.json");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[{"type":"resource_ledger","fact":"money","pickups":[],"receipts":[{"id":"x","target":"shop","fact":"unlocked","price":-1}]}],"capabilities":[]})",
        "ledger.json"); }, "positive integer");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[{"type":"purchase","context":"x","receipt":"unlock","source":"wrong"}]})",
        "purchase.json"); }, "unknown field");
    Reject([] { (void)ParseGameplayAILevelBindingsAsset(
        R"({"version":1,"roles":[],"traversals":[{"name":"gap","target":"land","type":"jump","handle":1.5}]})",
        "bindings.json"); }, "positive integer");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"wrong","receipt":"x","target":"shop"}]})",
        "reactions.json"); }, "unknown reaction type");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"hide_on_purchase","receipt":"x"}]})",
        "reactions.json"); }, "target");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"hide_on_purchase","receipt":"","target":"shop"}]})",
        "reactions.json"); }, "non-empty string");
    Reject([] { (void)ParseBehavior(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"hide_on_purchase","receipt":"x","target":"shop","typo":true}]})",
        "reactions.json"); }, "unknown field");
    auto numeric = ParseGameplayGOAPDefinitionAsset(R"({"id":"numeric","facts":[
        {"name":"ok","type":"bool"},{"name":"resource","type":"int"}],
        "goals":[{"name":"done","score":1,"facts":[{"fact":"ok","value":true}]}],
        "actions":[{"action":"move_to","context":"g","cost":1,"effects":[{"fact":"ok","value":true}],
        "numericContinuationConditions":[{"fact":"resource","op":">=","value":1}]}]})", "numeric.json");
    auto compiled = CompileGameplayGOAPDefinition(numeric, semantics);
    Check(compiled.definition.actions.front().numericContinuationConditions.front().comparison ==
        AINumericConditionOperator::GreaterOrEqual);
    Check(compiled.definition.actions.front().numericContinuationConditions.front().value == 1);
    numeric.actions.front().numericContinuationConditions.front().operation = "add";
    Reject([&] { (void)CompileGameplayGOAPDefinition(numeric, semantics); }, "unknown numeric condition operator");
    numeric.actions.front().numericContinuationConditions.front().operation = ">=";
    numeric.actions.front().numericContinuationConditions.front().fact = "ok";
    Reject([&] { (void)CompileGameplayGOAPDefinition(numeric, semantics); }, "wrong type");
    std::cout << "Passed " << checked << " authoring/template/dependency checks (module declarations removed).\n";
}
'''

parts = ['#include <bits/stdc++.h>\n']
for name in SOURCES:
    source = (ROOT / name).read_text(encoding='utf-8-sig')
    if name.endswith('EnTTHelpers.cppm'):
        # Only actual entity aliases are needed by these pure contracts. No ECS implementation is exercised.
        source = source[:source.index('\t[[nodiscard]]')] + '}\n'
        source = source.replace('#include <entt/entt.hpp>', '')
    source = re.sub(r'^(?:export )?(?:module|import)\b[^;]*;', '', source, flags=re.M)
    source = re.sub(r'\bexport\s+(?=namespace)', '', source)
    if name.endswith('GameplayGOAPDefinitionCompiler.cppm'):
        source = re.sub(r'\bInvalid\b', 'CompilerInvalid', source)
    parts.append(f'\n#line 1 "{name}"\n' + source)
parts.append('\nusing namespace rendern;\n' + parser_registrations())
parts.append('\n#line 1 "portable_harness.cpp"\n' + HARNESS)
with tempfile.TemporaryDirectory(prefix='goap-authoring-') as work:
    cpp = Path(work) / 'check.cpp'
    cpp.write_text('\n'.join(parts))
    binary = Path(work) / 'check'
    subprocess.run(['g++', '-std=c++23', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
