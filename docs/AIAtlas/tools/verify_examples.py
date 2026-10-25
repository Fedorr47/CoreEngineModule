"""Численные контрольные примеры атласа.

Ожидаемые значения выведены вручную (см. главы 05, 12–15) и сравниваются с учебными моделями
tools/atlas_models.py и tools/goap_model.py. Это проверка согласованности текста, схем и моделей с формулами
CoreEngine — не запуск движка. Пишет audit/generated/numeric-checks.json; код выхода 1 при расхождении.
"""
import json
import math
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ATLAS / 'tools'))
import atlas_models as m  # noqa: E402
import goap_model as g  # noqa: E402

CHECKS = []


def check(name, actual, expected, tol=1e-4):
    if isinstance(expected, (tuple, list)) and expected and isinstance(expected[0], float):
        ok = actual is not None and all(abs(a - e) <= tol for a, e in zip(actual, expected))
    elif isinstance(expected, float):
        ok = actual is not None and abs(actual - expected) <= tol
    else:
        ok = actual == expected
    CHECKS.append(dict(name=name, actual=repr(actual), expected=repr(expected), ok=ok))


def steering():
    direction, mag, arrived, d = m.seek((0, 0), (3, 4))
    check('Seek (0,0)→(3,4): направление', direction, (0.6, 0.8))
    check('Seek (0,0)→(3,4): magnitude', mag, 1.0)
    check('Flee от (3,4): направление', m.flee((0, 0), (3, 4))[0], (-0.6, -0.8))
    check('Seek: d = 0.2505 при r = 0.25 → прибыл (допуск 0.001)', m.seek((0, 0), (0.2505, 0))[2], True)
    check('Seek: d = 0.2520 при r = 0.25 → движется', m.seek((0, 0), (0.252, 0))[2], False)
    check('Flee: совпадение позиций → нулевая magnitude', m.flee((1, 1), (1, 1))[1], 0.0)
    check('Arrival r=0.5 R=2.5: d=1.5 → 0.5', m.arrival_magnitude(1.5, 0.5, 2.5, 0.1), 0.5)
    check('Arrival r=0.5 R=2.5: d=0.6 → m_min 0.1', m.arrival_magnitude(0.6, 0.5, 2.5, 0.1), 0.1)
    check('Arrival: d=0.5005 (внутри r+0.001) → 0', m.arrival_magnitude(0.5005, 0.5, 2.5, 0.1), 0.0)
    check('Arrival: d=0.5011 → m_min', m.arrival_magnitude(0.5011, 0.5, 2.5, 0.1), 0.1)
    check('Arrival: d=R → 1', m.arrival_magnitude(2.5, 0.5, 2.5, 0.1), 1.0)
    check('Arrival: R = r → 1 вне acceptance', m.arrival_magnitude(1.0, 0.5, 0.5, 0.1), 1.0)
    check('Arrival defaults: d=0.6 → (0.6−0.25)/0.75', m.arrival_magnitude(0.6, 0.25, 1.0, 0.1), 0.35 / 0.75)
    check('Arrival: R < r приводится к R = r', m.arrival_magnitude(0.9, 0.5, 0.2, 0.1), 1.0)


def follower():
    start, corner, nxt = (-3.0, 0.0), (0.0, 0.0), (0.0, 3.0)
    check('Look-ahead L=0.75, d=0.25 → q=(0, 0.5)', m.lookahead_target((-0.25, 0.0), start, corner, nxt), (0.0, 0.5))
    check('Look-ahead: d = L → нет цели (строгое <)', m.lookahead_target((-0.75, 0.0), start, corner, nxt), None)
    check('Look-ahead: L = 0 → отключён', m.lookahead_target((-0.25, 0.0), start, corner, nxt, 0.0), None)
    check('Look-ahead: короткий исходящий 0.3 → q = конец сегмента',
          m.lookahead_target((-0.1, 0.0), start, corner, (0.0, 0.3)), (0.0, 0.3))
    crossed, n = m.passed_corner((-0.2, 0.3), start, corner, nxt)
    check('Плоскость перехода: n̂ = (0.7071, 0.7071)', n, (math.sqrt(0.5), math.sqrt(0.5)))
    check('Точка (−0.2, 0.3) пересекла плоскость', crossed, True)
    check('Точка (−0.3, 0.2) не пересекла плоскость', m.passed_corner((-0.3, 0.2), start, corner, nxt)[0], False)
    reversal = m.passed_corner((-0.1, 0.2), (-3.0, 0.0), (0.0, 0.0), (-3.0, 0.0))
    check('Разворот 180°: нормаль = û_in = (1, 0)', reversal[1], (1.0, 0.0))
    check('Разворот 180°: (−0.1, 0.2) не пройден', reversal[0], False)
    s, c, g = (-8.0, 6.0), (8.0, 6.0), (4.0, 8.0)   # Steering Playground: Route_Start → Corner → Goal (XZ)
    check('Playground: q при p = (7.6, 6.1)', m.lookahead_target((7.6, 6.1), s, c, g), (7.69796, 6.15102), 1e-4)
    passed, normal = m.passed_corner((7.6, 6.1), s, c, g)
    check('Playground: нормаль плоскости перехода', normal, (0.22975, 0.97325), 1e-4)
    check('Playground: p = (7.6, 6.1) прошёл угол', passed, True)
    check('Playground: p = (7.5, 5.9) не прошёл угол', m.passed_corner((7.5, 5.9), s, c, g)[0], False)


def temporal():
    check('Flee 5/7: стоял, d=6 → стоит', m.flee_step(False, 6, 5, 7), False)
    check('Flee 5/7: бежал, d=6 → бежит', m.flee_step(True, 6, 5, 7), True)
    check('Flee: d = trigger → старт (≤)', m.flee_step(False, 5, 5, 7), True)
    check('Flee: d = safe → стоп (<)', m.flee_step(True, 7, 5, 7), False)
    check('Flee: safe < trigger приводится к trigger', m.flee_step(True, 2.5, 3, 2), True)
    # Интервал выборки 0.05 с и остаток fmod: 0.016+0.016+0.016+0.016 = 0.064 → обновление, остаток 0.014
    elapsed = 0.0
    refreshes = 0
    for _ in range(4):
        elapsed += 0.016
        if elapsed >= 0.05:
            refreshes += 1
            elapsed = math.fmod(elapsed, 0.05)
    check('Sampling: 4 тика по 16 мс → 1 пересчёт', refreshes, 1)
    check('Sampling: остаток после fmod', elapsed, 0.014, 1e-9)


def avoidance():
    check('Forward probe: v=2 → 3', m.forward_probe_distance(2.0), 3.0)
    check('Forward probe: v=6 → 4.5', m.forward_probe_distance(6.0), 4.5)
    check('Forward probe: v=4 → 3 (=4·0.75)', m.forward_probe_distance(4.0), 3.0)
    left, right = m.feelers((1.0, 0.0), 30)
    check('Feeler left для forward +X', left, (math.cos(math.radians(30)), -0.5))
    check('Feeler right для forward +X', right, (math.cos(math.radians(30)), 0.5))
    n = m._norm(-1.0, -0.35)
    check('Escape left (учебная стена)', m.escape_candidate(left, (1.0, 0.0), 'left', n), (0.42877, -0.90343), 2e-4)
    check('Escape right (учебная стена)', m.escape_candidate(right, (1.0, 0.0), 'right', n), (0.17749, 0.98413), 2e-4)
    check('Сторона: committed left, 1.00 / 1.10 → left', m.choose_side(1.0, 1.1, True, True, 'left')[0], 'left')
    check('Сторона: committed left, 1.00 / 1.20 → right', m.choose_side(1.0, 1.2, True, True, 'left')[0], 'right')
    check('Сторона: committed left, 1.00 / 1.15 → left (строгое >)', m.choose_side(1.0, 1.15, True, True, 'left')[0], 'left')
    check('Сторона: равенство без памяти → left', m.choose_side(1.0, 1.0, True, True, None)[0], 'left')
    check('Сторона: опора только справа → right', m.choose_side(2.0, 0.5, False, True, 'left')[0], 'right')
    check('Сторона: нет опоры → нулевая команда', m.choose_side(2.0, 2.0, False, False, 'left')[0], None)
    check('Длина support-луча 0.25 + 0.35', 0.25 + 0.35, 0.6)
    # Steering Playground: агент идёт по +X вдоль z = 6, грань препятствия с нормалью (−1, 0)
    pl, pr = m.feelers((1.0, 0.0), 30)
    check('Playground: escape left при n = (−1, 0)', m.escape_candidate(pl, (1.0, 0.0), 'left', (-1.0, 0.0)), (0.29935, -0.95415), 2e-4)
    check('Playground: escape right при n = (−1, 0)', m.escape_candidate(pr, (1.0, 0.0), 'right', (-1.0, 0.0)), (0.29935, 0.95415), 2e-4)
    check('Playground: первый контакт сферы r=0.37 с гранью x=−0.6 при L_f=3 → центр x ≥ −3.97', -0.6 - 0.37 - 3.0, -3.97)
    check('Walkable: n.y = cos 45° → допустимо', m.walkable(math.cos(math.radians(45)), 45), True)
    check('Walkable: n.y = 0.7071 (< cos 45°) → недопустимо', m.walkable(0.7071, 45), False)


def jump():
    g, vy, air = 9.81, 5.5, 2.5          # Jolt по умолчанию (INF), link.verticalSpeed, airDeceleration
    t = 2 * vy / g
    check('Прыжок: время полёта 2·5.5/9.81', t, 1.12130, 1e-4)
    check('Прыжок: высота апекса 5.5²/(2·9.81)', vy * vy / (2 * g), 1.54179, 1e-4)
    run = 4.5 * t - 0.5 * air * t * t if t <= 4.5 / air else 4.5 ** 2 / (2 * air)
    check('Прыжок с бегом: дальность ≈ 3.47 м', run, 3.4744, 1e-3)
    walk = 2.0 * t - 0.5 * air * t * t if t <= 2.0 / air else 2.0 ** 2 / (2 * air)
    check('Прыжок шагом: скорость гаснет за 0.8 с, дальность 0.8 м', walk, 0.8, 1e-6)


def contracts():
    # Согласованность arrival (XZ, r + 0.001) и within_distance (3D, R) для ассетов с целью на 0.2 выше старта
    for name, r, big_r in (('Marker Visit', 0.8, 0.8), ('Target Recovery', 0.4, 0.4)):
        worst = math.hypot(r + 0.001, 0.2)
        check(f'{name}: 3D-дистанция на краю acceptance при Δy = 0.2 > R', worst > big_r, True)
        check(f'{name}: горизонталь, нужная для подтверждения', math.sqrt(max(big_r ** 2 - 0.04, 0.0)),
              {'Marker Visit': 0.774597, 'Target Recovery': 0.346410}[name], 1e-5)


def goap():
    data = g.load_access_key()
    goap_def = data['goap']
    actions = g.compile_actions(goap_def, data['route_costs'])
    expected_costs = {'start_to_coin_a': 6.7158, 'start_to_coin_b': 6.0912, 'start_to_coin_c': 7.8168,
                      'coin_c_to_coin_a': 9.0554, 'coin_a_to_access_key_shop': 7.2111,
                      'coin_b_to_access_key_shop': 10.0, 'access_key_shop_to_goal': 17.0021}
    for context, cost in expected_costs.items():
        check(f'Route cost {context}', data['route_costs'][context], cost, 1e-4)
    start = g.initial_state(goap_def, atStart=True, coinAAvailable=True, coinBAvailable=True, coinCAvailable=True)
    result = g.plan(goap_def, actions, start)
    check('Access Key: план', result[1], ['start_to_coin_c', 'coin_c_to_coin_a', 'coin_a_to_access_key_shop',
                                          'buy_key', 'access_key_shop_to_goal'])
    check('Access Key: стоимость', result[0], 42.0854, 1e-3)
    check('Access Key: раскрыто состояний', result[2], 22)
    replan = g.initial_state(goap_def, atCoinC=True, coinCCollected=True, coins=1, coinBAvailable=True)
    result = g.plan(goap_def, actions, replan)
    check('Перепланирование (у C, A исчезла): план', result[1],
          ['coin_c_to_coin_b', 'coin_b_to_access_key_shop', 'buy_key', 'access_key_shop_to_goal'])
    check('Перепланирование: стоимость', result[0], 40.0852, 1e-3)
    stuck = g.initial_state(goap_def, atCoinC=True, coinCCollected=True, coins=1)
    check('Нет доступных монет → плана нет', g.plan(goap_def, actions, stuck), None)
    recovery = g.load_json('ai/goap/target_recovery.goap.json')
    rec_actions = g.compile_actions(recovery)
    check('Target Recovery: цель недоступна → плана нет',
          g.plan(recovery, rec_actions, g.initial_state(recovery, goalAvailable=False)), None)
    check('Target Recovery: цель доступна → [destination]',
          g.plan(recovery, rec_actions, g.initial_state(recovery, goalAvailable=True))[1], ['destination'])
    done = g.initial_state(recovery, atDestination=True)
    check('Цель уже выполнена → пустой план', g.plan(recovery, rec_actions, done)[1], [])


def main():
    steering()
    follower()
    temporal()
    avoidance()
    contracts()
    jump()
    goap()
    out = ATLAS / 'audit' / 'generated' / 'numeric-checks.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(CHECKS, indent=1, ensure_ascii=False), encoding='utf-8')
    failed = [c for c in CHECKS if not c['ok']]
    print(len(CHECKS) - len(failed), 'passed,', len(failed), 'failed')
    for c in failed:
        print('FAIL', c['name'], 'actual', c['actual'], 'expected', c['expected'])
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
