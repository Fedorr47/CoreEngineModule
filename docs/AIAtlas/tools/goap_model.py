"""Учебная модель GOAP-поиска CoreEngine на реальных ассетах (только чтение assets/).

Повторяет контракт FindAIPlan (PL-FIND): uniform-cost search, порядок действий (actionId, contextId),
tie-break по порядку вставки, lazy deletion, отсечение known <= nextCost, проверка цели при извлечении.
Стоимости move_to с routeCost берутся из Дейкстры по авторскому графу (RS-FIND, CP-MOVE-GRAPH).
Отличия от runtime: float64 вместо float32, нет наблюдений, исполнения и мира.
"""
import heapq
import json
import math
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ASSETS = REPO / 'assets'
SEMANTIC_IDS = {'move_to': 2, 'purchase': 3}   # NA-MOVETO-ID, CP-PURCHASE-ID
OPS = {'>=': lambda a, b: a >= b, '<=': lambda a, b: a <= b, '==': lambda a, b: a == b,
       '!=': lambda a, b: a != b, '<': lambda a, b: a < b, '>': lambda a, b: a > b}


def load_json(relative):
    return json.loads((ASSETS / relative).read_text(encoding='utf-8-sig'))


def load_access_key():
    goap = load_json('ai/goap/access_key.goap.json')
    behavior = load_json('ai/behaviors/access_key.behavior.json')
    routes = load_json('ai/routes/access_key.routes.json')
    bindings = load_json('ai/bindings/access_key.bindings.json')
    level = load_json('levels/ai_goap_access_key_development.level.json')
    nodes = {node['name']: node['transform']['position'] for node in level['nodes']}
    roles = {role['role']: nodes[role['node']] for role in bindings['roles']}
    edges = {}
    for edge in routes['edges']:
        cost = edge.get('cost', math.dist(roles[edge['from']], roles[edge['to']]))
        edges.setdefault(edge['from'], []).append((edge['to'], cost))
    costs = {}
    for capability in behavior['capabilities']:
        if capability['type'] == 'move_to' and capability.get('routeCost'):
            costs[capability['context']] = dijkstra(edges, capability['source'], capability['target'])
    return dict(goap=goap, roles=roles, edges=edges, route_costs=costs, level_nodes=nodes)


def dijkstra(edges, start, goal):
    best, queue, order = {start: 0.0}, [(0.0, 0, start)], 1
    while queue:
        cost, _, node = heapq.heappop(queue)
        if cost != best.get(node):
            continue
        if node == goal:
            return cost
        for target, edge_cost in edges.get(node, []):
            candidate = cost + edge_cost
            if candidate < best.get(target, math.inf):
                best[target] = candidate
                heapq.heappush(queue, (candidate, order, target))
                order += 1
    return None


def compile_actions(goap, route_costs=None):
    route_costs = route_costs or {}
    actions = []
    for context_id, action in enumerate(goap['actions']):
        actions.append(dict(action, contextId=context_id, actionId=SEMANTIC_IDS[action['action']],
                            cost=route_costs.get(action['context'], action['cost'])))
    actions.sort(key=lambda a: (a['actionId'], a['contextId']))
    return actions


def initial_state(goap, **values):
    state = {fact['name']: (False if fact['type'] == 'bool' else 0) for fact in goap['facts']}
    state.update(values)
    return state


def failed_conditions(state, action):
    failed = [f'{c["fact"]} == {str(c["value"]).lower()} (факт: {str(state[c["fact"]]).lower()})'
              for c in action.get('preconditions', []) if state[c['fact']] != c['value']]
    failed += [f'{c["fact"]} {c["op"]} {c["value"]} (факт: {state[c["fact"]]})'
               for c in action.get('numericPreconditions', []) if not OPS[c['op']](state[c['fact']], c['value'])]
    return failed


def apply(state, action):
    result = dict(state)
    for effect in action.get('effects', []):
        result[effect['fact']] = effect['value']
    for effect in action.get('numericEffects', []):
        if effect['op'] == 'set':
            result[effect['fact']] = effect['value']
        else:
            value = result[effect['fact']] + effect['value']
            if not -2 ** 31 <= value < 2 ** 31:
                return None
            result[effect['fact']] = value
    return result


def plan(goap, actions, start, trace=None):
    """Возвращает (cost, [contexts], expanded, distinct_states). trace(list) получает события поиска."""
    facts = [fact['name'] for fact in goap['facts']]
    key = lambda s: tuple(s[f] for f in facts)  # noqa: E731
    goal = goap['goals'][0]
    satisfied = lambda s: all(s[c['fact']] == c['value'] for c in goal['facts'])  # noqa: E731
    if satisfied(start):
        return 0.0, [], 0, 1
    best = {key(start): 0.0}
    sequence = 0
    queue = [(0.0, sequence, start, [])]
    expanded = 0
    while queue:
        cost, seq, state, steps = heapq.heappop(queue)
        if best.get(key(state)) != cost:
            if trace is not None:
                trace.append(dict(type='stale', seq=seq, cost=cost, steps=steps))
            continue
        expanded += 1
        if trace is not None:
            trace.append(dict(type='pop', seq=seq, cost=cost, steps=steps, state=state))
        if satisfied(state):
            if trace is not None:
                trace.append(dict(type='goal', seq=seq, cost=cost, steps=steps))
            return cost, steps, expanded, len(best)
        for action in actions:
            failed = failed_conditions(state, action)
            if failed:
                if trace is not None:
                    trace.append(dict(type='reject', context=action['context'], reason=failed))
                continue
            nxt = apply(state, action)
            if nxt is None:
                if trace is not None:
                    trace.append(dict(type='reject', context=action['context'], reason=['переполнение int32']))
                continue
            next_cost = cost + action['cost']
            known = best.get(key(nxt))
            if known is not None and known <= next_cost:
                if trace is not None:
                    trace.append(dict(type='dominated', context=action['context'], cost=next_cost, known=known))
                continue
            sequence += 1
            best[key(nxt)] = next_cost
            heapq.heappush(queue, (next_cost, sequence, nxt, steps + [action['context']]))
            if trace is not None:
                trace.append(dict(type='push', seq=sequence, context=action['context'], cost=next_cost))
    return None


if __name__ == '__main__':
    data = load_access_key()
    actions = compile_actions(data['goap'], data['route_costs'])
    start = initial_state(data['goap'], atStart=True, coinAAvailable=True, coinBAvailable=True, coinCAvailable=True)
    for context, cost in data['route_costs'].items():
        print(f'{context:28s} {cost:.4f}')
    print(plan(data['goap'], actions, start))
