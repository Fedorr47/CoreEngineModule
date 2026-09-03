#!/usr/bin/env python3
"""Check build/architecture boundaries and the checked-in asset reference graph."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / 'src'
ASSETS = ROOT / 'assets'


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def require(condition, message):
    if not condition:
        raise SystemExit(message)


modules = {}
for path in [*SRC.rglob('*.cppm'), *SRC.rglob('*.ixx')]:
    source = path.read_text(encoding='utf-8-sig')
    match = re.search(r'export module core:(\w+);', source)
    if match:
        name = match[1]
        require(name not in modules, f'Duplicate partition: {name}')
        modules[name] = path
for name, path in modules.items():
    for imported in re.findall(r'\bimport :(\w+);', path.read_text(encoding='utf-8-sig')):
        require(imported in modules, f'{name}: unresolved import {imported}')

cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
listed = set(re.findall(r'\b(Gameplay/AI/[\w/]+\.cppm)', cmake))
actual = {p.relative_to(SRC).as_posix() for p in SRC.glob('Gameplay/AI/**/*.cppm')}
require(listed == actual, f'AI CMake/source mismatch: {listed ^ actual}')

# These layers must not select scenarios or depend on concrete domain modules.
generic = [
    'src/Gameplay/AI/GOAP/AIDecisionContracts.cppm',
    'src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPCompositionRegistry.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm',
    'src/Gameplay/AI/Composition/GameplayGOAPBuiltinComponents.cppm',
]
for name in generic:
    source = (ROOT / name).read_text(encoding='utf-8-sig')
    require(not re.search(r'AccessKey|TargetRecovery|GOAP_Recovery|marker_visit|access_key|target_recovery', source),
            f'{name}: scenario leaked into generic composition')

# Component payload types belong to their modules, not to a closed generic variant.
for name in ('GameplayAIDecisionAsset.cppm', 'GameplayAIAssetParsing.cppm'):
    source = (SRC / 'Gameplay/AI/GOAP/Authoring' / name).read_text(encoding='utf-8-sig')
    require('std::variant' not in source, f'{name}: closed component payload variant')
    require(not re.search(r'"(?:move_to|purchase|resource_ledger|within_distance|hide_on_purchase)"', source),
            f'{name}: concrete component dispatch leaked into generic authoring')
composition = (SRC / 'Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm').read_text()
creation = composition.split('CreateGameplayGOAPDecisionFromTemplate(', 1)[1].split('// One-shot composition', 1)[0]
require(not re.search(r'(?:Load|Parse)GameplayAI|CompileGameplayGOAPDefinition', creation),
        'Per-agent creation repeats immutable asset preparation')

catalog = read_json(ASSETS / 'ai/decisions/catalog.json')
ids = {entry['id'] for entry in catalog['decisions']}
require(len(ids) == len(catalog['decisions']), 'Duplicate catalog decision id')
require({'target_recovery', 'marker_visit'} <= ids, 'Missing migrated/example decision')
for entry in catalog['decisions']:
    behavior = read_json(ASSETS / entry['behavior'])
    bindings = read_json(ASSETS / entry['bindings'])
    definition = read_json(ASSETS / behavior['definition'])
    facts = {fact['name'] for fact in definition['facts']}
    observed = []
    for observer in behavior['observations']:
        if 'fact' in observer:
            observed.append(observer['fact'])
        for group in ('pickups', 'receipts', 'locations'):
            observed.extend(item['fact'] for item in observer.get(group, []))
    require(set(observed) == facts and len(observed) == len(facts), f"{entry['id']}: observation coverage")
    roles = {role['role']: role['node'] for role in bindings['roles']}
    for observer in behavior['observations']:
        references = ([observer['target']] if 'target' in observer else [])
        for group in ('pickups', 'receipts', 'locations'):
            references += [item['target'] for item in observer.get(group, [])]
        require(all(role in roles for role in references), f"{entry['id']}: unbound observer role")
    contexts = {(action['action'], action['context']) for action in definition['actions']}
    configured = {(cap['type'], cap['context']) for cap in behavior['capabilities']}
    require(contexts == configured, f"{entry['id']}: action/context coverage")
    for capability in behavior['capabilities']:
        if capability['type'] == 'move_to':
            require(capability['source'] in roles and capability['target'] in roles, f"{entry['id']}: unbound capability role")
        else:
            receipts = [receipt for observer in behavior['observations'] for receipt in observer.get('receipts', [])]
            matches = [receipt for receipt in receipts if receipt['id'] == capability['receipt']]
            require(len(matches) == 1, f"{entry['id']}: unresolved receipt")
    receipts = [receipt for observer in behavior['observations'] for receipt in observer.get('receipts', [])]
    require(len({receipt['id'] for receipt in receipts}) == len(receipts), f"{entry['id']}: duplicate receipt id")
    for reaction in behavior.get('reactions', []):
        require(reaction['type'] == 'hide_on_purchase', f"{entry['id']}: unknown reaction type")
        require(reaction['target'] in roles, f"{entry['id']}: unbound reaction target")
        require(sum(receipt['id'] == reaction['receipt'] for receipt in receipts) == 1,
                f"{entry['id']}: unresolved reaction receipt")
    if behavior.get('routeGraph'):
        graph = read_json(ASSETS / behavior['routeGraph'])
        require(set(graph['nodes']) <= set(roles), f"{entry['id']}: unresolved graph role")
        require(len(set(graph['nodes'])) == len(graph['nodes']), f"{entry['id']}: duplicate graph node")
        traversals = {item['name'] for item in bindings.get('traversals', [])}
        for edge in graph['edges']:
            require(edge['from'] in graph['nodes'] and edge['to'] in graph['nodes'], f"{entry['id']}: unknown edge endpoint")
            require(not edge.get('traversal') or edge['traversal'] in traversals, f"{entry['id']}: unresolved traversal")
    # Verify that the demo level and its development script reference this config,
    # and that all role names resolve there. Runtime tests cover entity components.
    matched_levels = 0
    for path in (ASSETS / 'levels').glob('*.json'):
        level = read_json(path)
        scenario_path = level.get('developmentScenario')
        if not scenario_path:
            continue
        scenario = read_json(ASSETS / scenario_path)
        if not any(op.get('decision') == entry['id'] for op in scenario.get('start', [])):
            continue
        matched_levels += 1
        nodes = [node['name'] for node in level['nodes'] if node.get('alive', True)]
        for node in roles.values():
            require(nodes.count(node) == 1, f"{entry['id']}: level binding {node!r} does not resolve uniquely")
    require(matched_levels > 0, f"{entry['id']}: no development level exercises this config")

# A third scenario must require no identifier/selection branch in production C++.
for path in SRC.rglob('*'):
    if path.suffix in {'.cppm', '.cpp', '.h', '.inl', '.ixx'}:
        source = path.read_text(encoding='utf-8-sig')
        require(not re.search(r'marker_visit|GOAP_Marker_|access_key|AccessKey|GOAP_Coin_|GOAP_Jump_', source),
                f'Third scenario has a C++ dependency: {path}')

root = (SRC / 'Gameplay/AI/Composition/GameplayAIBuiltinDecisions.cppm').read_text()
require('.Register(' not in root, 'Composition root manually registers scenario configurations')

print(f'Passed module/build boundaries and {len(ids)} catalog asset graphs; third scenario is assets only.')
