#!/usr/bin/env python3
"""Run real asset parsers/compiler on GCC without the engine's MSVC module build.

This is a focused authoring smoke check, not a replacement for core_tests. It
removes module declarations in a temporary translation unit and renames one
private helper to avoid an anonymous-namespace collision from amalgamation.
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
    'src/Gameplay/AI/GOAP/Authoring/GameplayAIDecisionAsset.cppm',
]

HARNESS = r'''
using namespace rendern;
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
int main() {
    const std::vector semantics{GameplayGOAPSemanticAction{"move_to", AIActionId{2u}}, GameplayGOAPSemanticAction{"purchase", AIActionId{3u}}};
    auto catalog = LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json");
    Check(catalog.decisions.size() == 5);
    for (const auto& reference : catalog.decisions) {
        auto behavior = LoadGameplayAIBehaviorAsset(reference.behavior);
        auto bindings = LoadGameplayAILevelBindingsAsset(reference.bindings);
        auto definition = LoadGameplayGOAPDefinitionAsset(behavior.definition);
        auto compiled = CompileGameplayGOAPDefinition(definition, semantics);
        Check(behavior.model == "goap");
        if (!behavior.routeGraph.empty()) {
            const auto graph = LoadGameplayAIRouteGraphAsset(behavior.routeGraph);
            Check(graph.nodes.size() == bindings.roles.size());
            Check(graph.edges.size() >= 13);
            Check(behavior.inspectPath);
            Check(compiled.definition.actions.size() == 14);
            const auto& ledger = std::get<GameplayAIResourceLedgerAsset>(behavior.observations.front().parameters);
            Check(ledger.pickups.size() == 3);
            Check(ledger.receipts.front().price == 2);
            Check(compiled.FindIntegerFact(ledger.fact).has_value());
            const auto& purchase = std::get<GameplayAIPurchaseAsset>(behavior.capabilities.back().parameters);
            Check(purchase.receipt == ledger.receipts.front().id);
            Check(compiled.definition.actions.back().actionId == AIActionId{3u});
            Check(behavior.reactions.size() == 1);
            const auto& hide = std::get<GameplayAIHideOnPurchaseAsset>(behavior.reactions.front().parameters);
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
              compiled.FindBooleanFact(std::get<GameplayAISpatialObservationAsset>(behavior.observations.front().parameters).fact).value());
        Check(action.contextId == compiled.FindActionContext(behavior.capabilities.front().context).value());
        Check(std::get<GameplayAIMoveToAsset>(behavior.capabilities.front().parameters).acceptanceRadius == std::get<GameplayAISpatialObservationAsset>(behavior.observations.back().parameters).radius);
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
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"typo":true})",
        "behavior.json"); }, "unknown field 'typo'");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[{"type":"within_distance","target":"g","fact":"at","radius":"far"}],"capabilities":[]})",
        "behavior.json"); }, "radius");
    Reject([] { (void)ParseGameplayAIBehaviorAsset("{", "broken.json"); }, "broken.json");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[{"type":"resource_ledger","fact":"money","pickups":[],"receipts":[{"id":"x","target":"shop","fact":"unlocked","price":-1}]}],"capabilities":[]})",
        "ledger.json"); }, "positive integer");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[{"type":"purchase","context":"x","receipt":"unlock","source":"wrong"}]})",
        "purchase.json"); }, "unknown field");
    Reject([] { (void)ParseGameplayAILevelBindingsAsset(
        R"({"version":1,"roles":[],"traversals":[{"name":"gap","target":"land","type":"jump","handle":1.5}]})",
        "bindings.json"); }, "positive integer");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"wrong","receipt":"x","target":"shop"}]})",
        "reactions.json"); }, "unknown reaction type");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"hide_on_purchase","receipt":"x"}]})",
        "reactions.json"); }, "target");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a","observations":[],"capabilities":[],"reactions":[{"type":"hide_on_purchase","receipt":"","target":"shop"}]})",
        "reactions.json"); }, "non-empty string");
    Reject([] { (void)ParseGameplayAIBehaviorAsset(
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
    std::cout << "Passed " << checked << " authoring checks (module declarations removed).\n";
}
'''

parts = ['#include <bits/stdc++.h>\n']
for name in SOURCES:
    source = (ROOT / name).read_text(encoding='utf-8-sig')
    source = re.sub(r'^(?:export )?(?:module|import)\b[^;]*;', '', source, flags=re.M)
    source = re.sub(r'\bexport\s+(?=namespace)', '', source)
    if name.endswith('GameplayGOAPDefinitionCompiler.cppm'):
        source = re.sub(r'\bInvalid\b', 'CompilerInvalid', source)
    parts.append(f'\n#line 1 "{name}"\n' + source)
parts.append('\n#line 1 "portable_harness.cpp"\n' + HARNESS)
with tempfile.TemporaryDirectory(prefix='goap-authoring-') as work:
    cpp = Path(work) / 'check.cpp'
    cpp.write_text('\n'.join(parts))
    binary = Path(work) / 'check'
    subprocess.run(['g++', '-std=c++23', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
