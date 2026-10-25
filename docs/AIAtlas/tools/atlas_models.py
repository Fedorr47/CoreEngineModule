"""Учебные Python-модели формул CoreEngine для схем, демо и численных проверок.

Каждая функция повторяет описанный в атласе контракт и ссылается на реализацию (ID реестра). Это НЕ запуск движка:
float64 вместо float32, нет физики и кадрового цикла. Расхождения с runtime описаны в главах.
Координаты: правосторонняя система, +Y вверх; steering работает в плоскости XZ (вектор (x, z)).
"""
import math

LENGTH_EPS = 1e-6       # MU-EPS: kLengthEpsilon
LENGTH_EPS_SQ = LENGTH_EPS * LENGTH_EPS
MOVE_EPS = 1e-4         # kMoveEpsilon
ARRIVAL_TOL = 0.001     # kArrivalTolerance / kAcceptanceTolerance


def _norm(x, z):
    length = math.hypot(x, z)
    return (x / length, z / length) if length > LENGTH_EPS else (0.0, 0.0)


def seek(p, t, acceptance=0.25):
    """ST-SEEK: (направление, magnitude, arrived, d) в XZ."""
    dx, dz = t[0] - p[0], t[1] - p[1]
    d = math.hypot(dx, dz)
    r = max(acceptance, 0.0)
    if d <= r + ARRIVAL_TOL or d <= LENGTH_EPS:
        return (0.0, 0.0), 0.0, True, d
    return (dx / d, dz / d), 1.0, False, d


def flee(p, threat):
    """ST-FLEE: направление от угрозы, magnitude 1; совпадение позиций → нулевая команда."""
    dx, dz = p[0] - threat[0], p[1] - threat[1]
    d = math.hypot(dx, dz)
    if d <= LENGTH_EPS:
        return (0.0, 0.0), 0.0
    return (dx / d, dz / d), 1.0


def arrival_magnitude(d, r=0.25, big_r=1.0, m_min=0.1):
    """ST-ARRIVAL: magnitude как функция горизонтальной дистанции d."""
    r = max(r, 0.0)
    big_r = max(big_r, r)
    if d <= r + ARRIVAL_TOL or d <= LENGTH_EPS:
        return 0.0
    interval = big_r - r
    if interval <= LENGTH_EPS or d >= big_r:
        return 1.0
    linear = min(max((d - r) / interval, 0.0), 1.0)
    return max(linear, min(max(m_min, 0.0), 1.0))


def lookahead_target(p, start, corner, nxt, look=0.75):
    """RF-LOOKAHEAD: виртуальная цель q или None."""
    if look <= 0:
        return None
    inc = (corner[0] - start[0], corner[1] - start[1])
    out = (nxt[0] - corner[0], nxt[1] - corner[1])
    li, lo = math.hypot(*inc), math.hypot(*out)
    if li <= LENGTH_EPS or lo <= LENGTH_EPS:
        return None
    d = math.hypot(corner[0] - p[0], corner[1] - p[1])
    if d >= look:
        return None
    along = min(look - d, lo)
    return (corner[0] + out[0] * along / lo, corner[1] + out[1] * along / lo)


def passed_corner(p, start, corner, nxt, look=0.75):
    """RF-PASSED: d ≤ L и dot(p − c, n̂) ≥ 0, n̂ = norm(û_in + û_out) (или û_in при вырождении)."""
    inc = (corner[0] - start[0], corner[1] - start[1])
    out = (nxt[0] - corner[0], nxt[1] - corner[1])
    li, lo = math.hypot(*inc), math.hypot(*out)
    if li <= LENGTH_EPS or lo <= LENGTH_EPS:
        return False, None
    ui, uo = (inc[0] / li, inc[1] / li), (out[0] / lo, out[1] / lo)
    n = (ui[0] + uo[0], ui[1] + uo[1])
    nl2 = n[0] ** 2 + n[1] ** 2
    n = ui if nl2 <= LENGTH_EPS_SQ else (n[0] / math.sqrt(nl2), n[1] / math.sqrt(nl2))
    fc = (p[0] - corner[0], p[1] - corner[1])
    inside = math.hypot(*fc) <= look
    return inside and (fc[0] * n[0] + fc[1] * n[1]) >= 0.0, n


def flee_step(is_fleeing, d, trigger=2.0, safe=3.0):
    """NA-FLEE-HYST: новое состояние бегства по XZ-дистанции."""
    trigger = max(trigger, 0.0)
    safe = max(safe, trigger)
    return d < safe if is_fleeing else d <= trigger


def forward_probe_distance(speed, base=3.0, horizon=0.75):
    """OA-SANITIZE: L_f = max(L0, v·τ)."""
    return max(base, max(speed, 0.0) * max(horizon, 0.0))


def feelers(forward, angle_deg=30.0):
    """OA-FEELERS: (left, right) поворотом forward=(x,z) на ±θ по формулам CoreEngine."""
    c, s = math.cos(math.radians(angle_deg)), math.sin(math.radians(angle_deg))
    fx, fz = forward
    left = (fx * c + fz * s, fz * c - fx * s)
    right = (fx * c - fz * s, fz * c + fx * s)
    return left, right


def escape_candidate(feeler, forward, side, hit_normal, w_feeler=0.6, w_tangent=0.4, w_normal=0.3):
    """OA-ESCAPE: norm(0.6·feeler + 0.4·tangent_side + 0.3·n_planar)."""
    n = _norm(*hit_normal)
    if n == (0.0, 0.0):
        return feeler
    right_axis = (-forward[1], forward[0])
    tangent = (-n[1], n[0])
    if tangent[0] * right_axis[0] + tangent[1] * right_axis[1] < 0:
        tangent = (-tangent[0], -tangent[1])
    if side == 'left':
        tangent = (-tangent[0], -tangent[1])
    e = (feeler[0] * w_feeler + tangent[0] * w_tangent + n[0] * w_normal,
         feeler[1] * w_feeler + tangent[1] * w_tangent + n[1] * w_normal)
    out = _norm(*e)
    return feeler if out == (0.0, 0.0) else out


def choose_side(left_clear, right_clear, left_ok, right_ok, committed, advantage=0.15):
    """OA-SIDE: выбор стороны; возвращает (side, committed_after). side=None → нулевая команда."""
    if not left_ok and not right_ok:
        return None, None
    preferred = 'left' if left_clear >= right_clear else 'right'
    if left_ok != right_ok:
        chosen = 'left' if left_ok else 'right'
    elif committed == 'left':
        chosen = 'right' if right_clear > left_clear + advantage else 'left'
    elif committed == 'right':
        chosen = 'left' if left_clear > right_clear + advantage else 'right'
    else:
        chosen = preferred
    return chosen, chosen


def walkable(normal_y_unit, max_slope_deg):
    """OA-SUPPORT: n̂.y ≥ cos θ_max − ε²."""
    return normal_y_unit >= math.cos(math.radians(max_slope_deg)) - LENGTH_EPS_SQ
