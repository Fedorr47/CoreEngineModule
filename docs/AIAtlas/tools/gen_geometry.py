"""Вычисленные геометрические построения и графики атласа (SVG без внешних зависимостей).

Все точки, векторы и кривые вычисляются моделями tools/atlas_models.py по тем же формулам, что и runtime.
Вид сверху: X вправо, Z ВНИЗ (камера над сценой смотрит вдоль −Y в правосторонней системе) — так сохраняется
реальная ориентация «влево/вправо» агента. Пишет docs/assets/diagrams/g*.svg и docs/data/geometry-meta.json.
"""
import json
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import atlas_models as m  # noqa: E402

ATLAS = Path(__file__).resolve().parents[1]
OUT = ATLAS / 'docs' / 'assets' / 'diagrams'
FONT = "Segoe UI, Noto Sans, Arial, sans-serif"
COL = dict(ink='#14212e', grid='#e3e8ee', axis='#8796a5', route='#3f6ea3', agent='#c2410c', target='#15803d',
           virtual='#7c3aed', plane='#b45309', wall='#475569', probe='#0ea5e9', hit='#dc2626', ok='#15803d',
           bad='#b91c1c', soft='#94a3b8')


class Svg:
    def __init__(self, ident, width, height, title):
        self.ident, self.w, self.h, self.items = ident, width, height, []
        self.items.append(f'<title>{title}</title>')
        self.items.append(f'<defs>{self._markers()}</defs>')
        self.items.append(f'<rect x="0" y="0" width="{width}" height="{height}" fill="#ffffff"/>')

    def _markers(self):
        out = []
        for name, color in COL.items():
            out.append(f'<marker id="{self.ident}-arrow-{name}" viewBox="0 0 10 10" refX="9" refY="5" '
                       f'markerUnits="userSpaceOnUse" markerWidth="12" markerHeight="12" orient="auto-start-reverse">'
                       f'<path d="M0,0 L10,5 L0,10 z" fill="{color}"/></marker>')
        return ''.join(out)

    def line(self, a, b, color='ink', width=2, dash=None, arrow=False, pad=0.0):
        if pad:
            dx, dy = b[0] - a[0], b[1] - a[1]
            length = math.hypot(dx, dy) or 1.0
            a = (a[0] + dx / length * pad, a[1] + dy / length * pad)
            b = (b[0] - dx / length * pad, b[1] - dy / length * pad)
        d = f' stroke-dasharray="{dash}"' if dash else ''
        mk = f' marker-end="url(#{self.ident}-arrow-{color})"' if arrow else ''
        self.items.append(f'<line x1="{a[0]:.1f}" y1="{a[1]:.1f}" x2="{b[0]:.1f}" y2="{b[1]:.1f}" '
                          f'stroke="{COL.get(color, color)}" stroke-width="{width}"{d}{mk}/>')

    def poly(self, points, color='ink', width=2, dash=None, fill='none'):
        pts = ' '.join(f'{x:.1f},{y:.1f}' for x, y in points)
        d = f' stroke-dasharray="{dash}"' if dash else ''
        self.items.append(f'<polyline points="{pts}" fill="{fill}" stroke="{COL.get(color, color)}" '
                          f'stroke-width="{width}"{d}/>')

    def circle(self, c, r, color='ink', fill='none', width=1.5, dash=None):
        d = f' stroke-dasharray="{dash}"' if dash else ''
        self.items.append(f'<circle cx="{c[0]:.1f}" cy="{c[1]:.1f}" r="{r:.1f}" fill="{COL.get(fill, fill)}" '
                          f'stroke="{COL.get(color, color)}" stroke-width="{width}"{d}/>')

    def rect(self, x, y, w, h, color='wall', fill='#e2e8f0', width=1.5):
        self.items.append(f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" fill="{fill}" '
                          f'stroke="{COL.get(color, color)}" stroke-width="{width}"/>')

    def text(self, p, s, size=14, color='ink', anchor='start', weight='normal', italic=False):
        style = ' font-style="italic"' if italic else ''
        s = s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
        self.items.append(f'<text x="{p[0]:.1f}" y="{p[1]:.1f}" font-family="{FONT}" font-size="{size}" '
                          f'fill="{COL.get(color, color)}" text-anchor="{anchor}" font-weight="{weight}"{style}>{s}</text>')

    def render(self):
        return (f'<svg xmlns="http://www.w3.org/2000/svg" id="svg-{self.ident}" viewBox="0 0 {self.w} {self.h}" '
                f'width="{self.w}" height="{self.h}">' + ''.join(self.items) + '</svg>\n')


class View:
    """Мир XZ → экран: X вправо, Z вниз."""
    def __init__(self, x0, z0, scale, ox, oy):
        self.x0, self.z0, self.s, self.ox, self.oy = x0, z0, scale, ox, oy

    def __call__(self, x, z):
        return (self.ox + (x - self.x0) * self.s, self.oy + (z - self.z0) * self.s)


FIGURES = {}


def figure(ident, title, kind, level, refs):
    def wrap(fn):
        FIGURES[ident] = (fn, dict(title=title, kind=kind, level=level, refs=refs,
                                   source='tools/gen_geometry.py', renderer='python'))
        return fn
    return wrap


def grid(svg, view, xs, zs):
    for x in xs:
        svg.line(view(x, zs[0]), view(x, zs[-1]), 'grid', 1)
    for z in zs:
        svg.line(view(xs[0], z), view(xs[-1], z), 'grid', 1)


def axes_note(svg, p):
    svg.text(p, 'вид сверху: X →, Z ↓ (+Y на зрителя)', 12, 'axis')


@figure('g13-seek-flee', 'Seek и Flee для агента в (0,0,0) и точки (3,0,4)', 'геометрия (вычислено)', 'детали',
        ['ST-SEEK', 'ST-FLEE'])
def seek_flee():
    svg = Svg('g13-seek-flee', 720, 470, 'Seek и Flee')
    v = View(-2, -2, 62, 70, 40)
    grid(svg, v, range(-2, 6), range(-2, 6))
    p, t = (0.0, 0.0), (3.0, 4.0)
    direction, mag, _, d = m.seek(p, t)
    fdir, _ = m.flee(p, t)
    svg.line(v(*p), v(*t), 'soft', 2, '6 5')
    svg.text(v(1.7, 1.75), f'd = |Δ| = {d:.1f}', 14, 'soft')
    svg.circle(v(*t), 0.25 * v.s, 'target', dash='4 3')
    svg.circle(v(*t), 5, 'target', 'target')
    svg.text((v(*t)[0] + 12, v(*t)[1] + 5), 'цель (3, 4); r = 0.25', 14, 'target')
    svg.circle(v(*p), 6, 'agent', 'agent')
    svg.text((v(*p)[0] - 10, v(*p)[1] - 12), 'агент (0, 0)', 14, 'agent', 'end')
    svg.line(v(*p), v(direction[0] * 1.6, direction[1] * 1.6), 'route', 3, arrow=True)
    svg.text(v(direction[0] * 1.6 + 0.15, direction[1] * 1.6 - 0.1), f'Seek: ({direction[0]:.1f}, {direction[1]:.1f}), m = {mag:.0f}', 14, 'route')
    svg.line(v(*p), v(fdir[0] * 1.6, fdir[1] * 1.6), 'hit', 3, arrow=True)
    svg.text(v(fdir[0] * 1.6 - 0.1, fdir[1] * 1.6 - 0.25), f'Flee: ({fdir[0]:.1f}, {fdir[1]:.1f})', 14, 'hit', 'end')
    axes_note(svg, (430, 455))
    return svg.render()


@figure('g13-arrival-profile', 'Профиль Arrival: magnitude от горизонтальной дистанции', 'график (вычислено)', 'детали',
        ['ST-ARRIVAL', 'ST-ARRIVAL-SETTINGS'])
def arrival_profile():
    svg = Svg('g13-arrival-profile', 780, 430, 'Профиль Arrival')
    x0, y0, w, h, dmax = 80, 360, 640, 300, 3.0
    X = lambda d: x0 + d / dmax * w  # noqa: E731
    Y = lambda mval: y0 - mval * h  # noqa: E731
    for i in range(0, 7):
        d = i * 0.5
        svg.line((X(d), y0), (X(d), y0 - h), 'grid', 1)
        svg.text((X(d), y0 + 20), f'{d:.1f}', 12, 'axis', 'middle')
    for i in range(0, 6):
        mval = i * 0.2
        svg.line((x0, Y(mval)), (x0 + w, Y(mval)), 'grid', 1)
        svg.text((x0 - 8, Y(mval) + 4), f'{mval:.1f}', 12, 'axis', 'end')
    svg.line((x0, y0), (x0 + w, y0), 'axis', 1.5)
    svg.line((x0, y0), (x0, y0 - h), 'axis', 1.5)
    svg.text((x0 + w, y0 + 40), 'горизонтальная дистанция d, м', 13, 'ink', 'end')
    svg.text((x0 - 50, y0 - h - 14), 'magnitude m', 13, 'ink')

    def curve(r, big_r, m_min, color, dash=None):
        pts, prev = [], None
        steps = 1200
        for i in range(steps + 1):
            d = dmax * i / steps
            val = m.arrival_magnitude(d, r, big_r, m_min)
            if prev is not None and abs(val - prev) > 0.05:
                svg.poly(pts, color, 2.5, dash)
                pts = []
            pts.append((X(d), Y(val)))
            prev = val
        svg.poly(pts, color, 2.5, dash)

    curve(0.5, 2.5, 0.1, 'route')
    curve(0.25, 1.0, 0.1, 'virtual', '7 5')
    for d in (1.5, 0.6):
        val = m.arrival_magnitude(d, 0.5, 2.5, 0.1)
        svg.circle((X(d), Y(val)), 5, 'agent', 'agent')
        dx, dy = (10, -10) if d > 1 else (14, 26)
        svg.text((X(d) + dx, Y(val) + dy), f'd = {d}: m = {val:.2f}', 13, 'agent')
    svg.line((X(0.5), y0), (X(0.5), y0 - h), 'plane', 1.5, '3 3')
    svg.line((X(2.5), y0), (X(2.5), y0 - h), 'plane', 1.5, '3 3')
    svg.text((X(0.5) + 4, y0 - h + 14), 'r', 13, 'plane')
    svg.text((X(2.5) + 4, y0 - h + 14), 'R', 13, 'plane')
    lx, ly = X(1.75), Y(0.3)
    svg.line((lx, ly), (lx + 30, ly), 'route', 2.5)
    svg.text((lx + 38, ly + 5), 'учебный: r = 0.5, R = 2.5, m_min = 0.1', 13)
    svg.line((lx, ly + 24), (lx + 30, ly + 24), 'virtual', 2.5, '7 5')
    svg.text((lx + 38, ly + 29), 'по умолчанию: r = 0.25, R = 1.0, m_min = 0.1', 13)
    svg.text((x0, y0 + 40), 'при d = r + 0.001 скачок 0 → m_min', 12, 'plane')
    return svg.render()


@figure('g12-lookahead', 'Look-ahead: виртуальная цель на исходящем сегменте (L = 0.75)', 'геометрия (вычислено)',
        'детали', ['RF-LOOKAHEAD'])
def lookahead():
    svg = Svg('g12-lookahead', 720, 470, 'Look-ahead')
    v = View(-3.2, -1.2, 120, 40, 40)
    grid(svg, v, [x * 0.5 for x in range(-6, 3)], [z * 0.5 for z in range(-2, 7)])
    start, corner, nxt = (-3.0, 0.0), (0.0, 0.0), (0.0, 3.0)
    p = (-0.25, 0.0)
    q = m.lookahead_target(p, start, corner, nxt, 0.75)
    svg.poly([v(*start), v(*corner), v(*nxt)], 'route', 4)
    svg.text((v(*start)[0], v(*start)[1] - 12), 'points[i]', 13, 'route')
    svg.text((v(*corner)[0] + 10, v(*corner)[1] - 10), 'угол c = points[i+1]', 13, 'route')
    svg.text((v(*nxt)[0] + 10, v(*nxt)[1]), 'points[i+2]', 13, 'route')
    svg.circle(v(*corner), 0.75 * v.s, 'virtual', dash='6 4')
    svg.text(v(-0.95, -0.85), 'окрестность L = 0.75', 13, 'virtual')
    svg.circle(v(*p), 6, 'agent', 'agent')
    svg.text((v(*p)[0] - 6, v(*p)[1] + 24), 'p: d = 0.25', 13, 'agent', 'end')
    svg.circle(v(*q), 6, 'virtual', 'virtual')
    svg.text((v(*q)[0] + 10, v(*q)[1] + 5), f'q = c + û_out·(L − d) = ({q[0]:.2f}, {q[1]:.2f})', 13, 'virtual')
    svg.line(v(*p), v(*q), 'agent', 2.5, arrow=True)
    svg.text(v(-2.9, 2.6), 'Seek к q (acceptance 0) вместо Arrival к углу:', 13)
    svg.text(v(-2.9, 2.85), 'агент «срезает» угол, не останавливаясь в c', 13)
    axes_note(svg, (430, 455))
    return svg.render()


@figure('g12-corner-plane', 'Плоскость перехода угла +X → +Z и точка (−0.2, 0.3)', 'геометрия (вычислено)',
        'детали', ['RF-PASSED'])
def corner_plane():
    svg = Svg('g12-corner-plane', 720, 500, 'Плоскость перехода')
    v = View(-1.6, -1.2, 150, 40, 40)
    grid(svg, v, [x * 0.5 for x in range(-3, 4)], [z * 0.5 for z in range(-2, 5)])
    start, corner, nxt = (-3.0, 0.0), (0.0, 0.0), (0.0, 3.0)
    p = (-0.2, 0.3)
    crossed, n = m.passed_corner(p, start, corner, nxt, 0.75)
    svg.poly([v(-1.6, 0), v(*corner), v(0, 1.6)], 'route', 4)
    svg.line(v(-0.85, 0.85), v(0.85, -0.85), 'plane', 2.5)
    svg.text(v(0.32, -0.6), 'плоскость перехода: x + z = 0', 13, 'plane')
    svg.line(v(0, -1.1), v(0, 1.6), 'soft', 1.5, '5 4')
    svg.text(v(0.05, -0.95), 'старая плоскость x = 0', 12, 'soft')
    svg.line(v(*corner), v(n[0] * 0.6, n[1] * 0.6), 'plane', 3, arrow=True)
    svg.text(v(n[0] * 0.6 + 0.12, n[1] * 0.6 + 0.05), f'n̂ = ({n[0]:.3f}, {n[1]:.3f})', 13, 'plane')
    svg.circle(v(*corner), 0.75 * v.s, 'virtual', dash='6 4')
    svg.circle(v(*p), 6, 'agent', 'agent')
    svg.line(v(*p), v(-0.9, 1.22), 'agent', 1, '2 3')
    svg.text(v(-1.55, 1.3), 'p = (−0.2, 0.3)', 13, 'agent')
    svg.text(v(-1.55, 1.45), f'dot(p − c, n̂) = {(p[0]*n[0]+p[1]*n[1]):.4f} ≥ 0', 13, 'agent')
    svg.text(v(-1.55, 1.6), '→ угол пройден' if crossed else '→ не пройден', 13, 'agent')
    svg.text(v(-1.55, -0.1), 'входящий +X', 13, 'route')
    svg.text(v(0.08, 1.5), 'исходящий +Z', 13, 'route')
    axes_note(svg, (430, 468))
    return svg.render()


@figure('g14-flee-hysteresis', 'Гистерезис FleeTarget: trigger 5, safe 7 (Steering Playground)', 'график (вычислено)',
        'детали', ['NA-FLEE-HYST', 'AS-SC-PLAYGROUND'])
def flee_hysteresis():
    svg = Svg('g14-flee-hysteresis', 760, 360, 'Гистерезис Flee')
    x0, w, dmax = 70, 640, 10.0
    X = lambda d: x0 + d / dmax * w  # noqa: E731
    y_on, y_off = 110, 250
    for i in range(0, 11):
        svg.line((X(i), 70), (X(i), 290), 'grid', 1)
        svg.text((X(i), 312), str(i), 12, 'axis', 'middle')
    svg.text((x0 + w, 338), 'горизонтальная дистанция до угрозы d, м', 13, 'ink', 'end')
    svg.text((x0 - 10, y_on + 5), 'бежит', 13, 'hit', 'end')
    svg.text((x0 - 10, y_off + 5), 'стоит', 13, 'route', 'end')
    svg.poly([(X(0), y_on), (X(7), y_on)], 'hit', 4)
    svg.poly([(X(5), y_off), (X(10), y_off)], 'route', 4)
    svg.line((X(7), y_on), (X(7), y_off - 6), 'hit', 2, '5 4', arrow=True)
    svg.line((X(5), y_off), (X(5), y_on + 6), 'route', 2, '5 4', arrow=True)
    svg.text((X(7) + 6, (y_on + y_off) / 2), 'd ≥ safe = 7 → стоп', 13, 'hit')
    svg.text((X(5) - 6, (y_on + y_off) / 2), 'd ≤ trigger = 5 → бег', 13, 'route', 'end')
    svg.rect(X(5), 80, X(7) - X(5), 200, 'plane', 'none', 1)
    svg.text(((X(5) + X(7)) / 2, 74), 'зона памяти: решает прошлое состояние', 12, 'plane', 'middle')
    for d, state in ((6.0, False), (6.0, True)):
        y = y_on if m.flee_step(state, d, 5, 7) else y_off
        svg.circle((X(d), y), 6, 'agent', 'agent')
    svg.text((X(6) + 9, y_on - 10), 'd=6, был бег → бег', 12, 'agent')
    svg.text((X(6) + 9, y_off + 22), 'd=6, стоял → стоит', 12, 'agent')
    return svg.render()


@figure('g15-feelers', 'Три feelers и escape-кандидаты перед наклонной стеной (учебные числа)',
        'геометрия (вычислено)', 'детали', ['OA-FEELERS', 'OA-ESCAPE', 'OA-SANITIZE'])
def feelers():
    svg = Svg('g15-feelers', 820, 600, 'Feelers и escape')
    v = View(-1.0, -2.4, 125, 40, 30)
    grid(svg, v, [x * 0.5 for x in range(-2, 10)], [z * 0.5 for z in range(-4, 6)])
    forward = (1.0, 0.0)
    left, right = m.feelers(forward, 30)
    n = m._norm(-1.0, -0.35)  # нормаль стены: к агенту и немного к −Z (вверх на экране)
    le = m.escape_candidate(left, forward, 'left', n)
    re = m.escape_candidate(right, forward, 'right', n)
    radius = 0.35 + 0.02
    wall_point = (2.2, 0.0)
    c = n[0] * wall_point[0] + n[1] * wall_point[1]
    centre_x = (radius + c) / n[0]          # центр сферы на луче +X, касающейся стены
    contact = (centre_x - n[0] * radius, -n[1] * radius)
    t = (-n[1], n[0])
    a, b = (2.2 + t[0] * 2.2, t[1] * 2.2), (2.2 - t[0] * 1.8, -t[1] * 1.8)
    svg.line(v(*a), v(*b), 'wall', 7)
    svg.text((v(*a)[0] + 10, v(*a)[1] + 4), 'стена (StaticWorld)', 13, 'wall')
    origin = (0.0, 0.0)
    svg.circle(v(*origin), radius * v.s, 'agent', dash='4 3')
    svg.circle(v(*origin), 6, 'agent', 'agent')
    svg.text((v(*origin)[0] - 50, v(*origin)[1] - 52), 'агент: r + margin = 0.37', 12, 'agent')
    svg.circle(v(centre_x, 0), radius * v.s, 'hit', dash='3 3', width=1)
    svg.line(v(*origin), v(3.0, 0.0), 'probe', 2, '6 4', arrow=True)
    svg.text(v(2.95, 0.25), 'forward, L_f = 3', 12, 'probe', 'end')
    svg.line(v(*origin), v(left[0] * 2, left[1] * 2), 'probe', 2, '6 4', arrow=True)
    svg.text(v(left[0] * 2 + 0.08, left[1] * 2 - 0.05), 'left: 30°, 2 м', 12, 'probe')
    svg.line(v(*origin), v(right[0] * 2, right[1] * 2), 'probe', 2, '6 4', arrow=True)
    svg.text(v(right[0] * 2 - 0.1, right[1] * 2 + 0.25), 'right: 30°, 2 м', 12, 'probe', 'end')
    svg.circle(v(*contact), 5, 'hit', 'hit')
    svg.line(v(*contact), v(contact[0] + n[0] * 0.75, contact[1] + n[1] * 0.75), 'hit', 2.5, arrow=True)
    svg.text(v(contact[0] + n[0] * 0.75 - 0.05, contact[1] + n[1] * 0.75 - 0.12),
             f'n = ({n[0]:.2f}, {n[1]:.2f})', 12, 'hit', 'end')
    svg.text(v(centre_x + 0.05, 0.62), f'контакт сферы при s = {centre_x:.2f}', 12, 'hit')
    svg.line(v(*origin), v(le[0] * 1.3, le[1] * 1.3), 'ok', 3.5, arrow=True)
    svg.text(v(le[0] * 1.3 + 0.1, le[1] * 1.3), f'escape left ({le[0]:.2f}, {le[1]:.2f})', 13, 'ok')
    svg.line(v(*origin), v(re[0] * 1.3, re[1] * 1.3), 'virtual', 3.5, arrow=True)
    svg.text(v(re[0] * 1.3 + 0.1, re[1] * 1.3 + 0.05), f'escape right ({re[0]:.2f}, {re[1]:.2f})', 13, 'virtual')
    svg.text((40, 585), 'forward = +X; left = (cos θ, −sin θ): к −Z, вверх на экране. Вид сверху: X →, Z ↓', 12, 'axis')
    return svg.render()


@figure('g15-support', 'Support probe: вынос 0.75, подъём 0.25, допустимое падение 0.35 (вид сбоку)',
        'геометрия (вычислено)', 'детали', ['OA-SUPPORT', 'OA-SETTINGS'])
def support():
    svg = Svg('g15-support', 780, 400, 'Support probe')
    s, ox, oy = 150, 120, 270
    P = lambda x, y: (ox + x * s, oy - y * s)  # noqa: E731  (вид сбоку: горизонталь вдоль escape, +Y вверх)
    for case, shift, ground_y, ok in (('опора есть', 0.0, 0.0, True), ('обрыв', 2.6, -0.8, False)):
        base = (shift, 0.0)
        svg.rect(P(shift - 0.6, 0)[0], P(0, 0)[1], s * (0.95 if not ok else 1.9), 18, 'wall', '#cbd5e1')
        if not ok:
            svg.rect(P(shift + 0.35, 0)[0], P(0, ground_y)[1], s * 1.0, 18, 'wall', '#cbd5e1')
        svg.circle(P(*base), 6, 'agent', 'agent')
        svg.line(P(*base), P(shift, 0.9), 'agent', 2, '4 3')
        svg.text(P(shift - 0.05, 0.95), 'основание b', 12, 'agent', 'middle')
        origin = (shift + 0.75, 0.25)
        svg.line(P(*base), P(origin[0], 0.0), 'soft', 1.5, '4 3')
        svg.line(P(origin[0], 0.0), P(*origin), 'soft', 1.5, '4 3')
        svg.circle(P(*origin), 5, 'probe', 'probe')
        end = (origin[0], origin[1] - 0.60)
        svg.line(P(*origin), P(*end), 'ok' if ok else 'bad', 3, arrow=True)
        svg.text(P(origin[0] + 0.06, 0.12), 'луч вниз 0.60', 12, 'ok' if ok else 'bad')
        if not ok:
            svg.text(P(shift + 1.45, -0.72), 'земля ниже на 0.8 > 0.35', 12, 'bad')
        svg.text(P(shift + 0.3, -0.62), f'{case}: {"кандидат допустим" if ok else "нет попадания → кандидат отвергнут"}',
                 13, 'ok' if ok else 'bad', 'middle')
    svg.text((40, 30), 'origin = b + d̂_escape·0.75 + (0, 0.25, 0); длина = 0.25 + 0.35 = 0.60', 13)
    svg.text((40, 52), 'нормаль попадания допустима при n̂·ŷ ≥ cos(θ_max) (для 45°: ≈ 0.7071)', 13)
    return svg.render()


@figure('g05-access-key-map', 'Access Key: авторский граф маршрутов на позициях уровня и оптимальный план (стоимость 42.09)',
        'геометрия (вычислено из ассетов)', 'обзор', ['AS-ACCESS-ROUTES', 'CP-MOVE-GRAPH', 'RS-FIND', 'PL-FIND'])
def access_key_map():
    import goap_model as g
    data = g.load_access_key()
    roles = {name: (pos[0], pos[2]) for name, pos in data['roles'].items()}
    actions = g.compile_actions(data['goap'], data['route_costs'])
    start = g.initial_state(data['goap'], atStart=True, coinAAvailable=True, coinBAvailable=True, coinCAvailable=True)
    cost, steps, _, _ = g.plan(data['goap'], actions, start)
    contexts = {c['context']: (c['source'], c['target']) for c in
                g.load_json('ai/behaviors/access_key.behavior.json')['capabilities'] if c['type'] == 'move_to'}
    svg = Svg('g05-access-key-map', 760, 840, 'Access Key map')
    v = View(-8.5, -8.5, 36, 50, 40)
    grid(svg, v, range(-8, 9, 2), range(-8, 13, 2))
    drawn = set()
    for source, targets in data['edges'].items():
        for target, edge_cost in targets:
            pair = tuple(sorted((source, target)))
            both = any(t == source for t, _ in data['edges'].get(target, []))
            if pair in drawn:
                continue
            drawn.add(pair)
            a, b = v(*roles[source]), v(*roles[target])
            svg.line(a, b, 'soft', 1.5, '5 4', arrow=not both, pad=11)
            mid = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
            svg.text((mid[0] + 4, mid[1] - 4), f'{edge_cost:.2f}{" ⇄" if both else ""}', 11, 'axis')
    total = 0.0
    for index, step in enumerate(steps, 1):
        if step not in contexts:
            continue
        source, target = contexts[step]
        a, b = v(*roles[source]), v(*roles[target])
        svg.line(a, b, 'agent', 4, arrow=True, pad=11)
        total += data['route_costs'][step]
        mid = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
        svg.circle((mid[0] - 14, mid[1] + 12), 10, 'agent', '#ffffff')
        svg.text((mid[0] - 14, mid[1] + 16), str(index if index < 4 else index), 12, 'agent', 'middle', 'bold')
    labels = {'start': 'start (0, 0)', 'coinA': 'coinA (−6, −3)', 'coinB': 'coinB (6, 1)', 'coinC': 'coinC (−5, 6)',
              'shop': 'shop (0, −7)', 'goal': 'goal (0, 10)'}
    for name, pos in roles.items():
        p = v(*pos)
        svg.circle(p, 8, 'route', '#ffffff', 2.5)
        svg.text((p[0] + 12, p[1] - 10), labels[name], 13, 'route', weight='bold')
    svg.text((50, 778), f'план: {" → ".join(steps)}', 12, 'agent')
    svg.text((50, 798), f'стоимость = 7.82 + 9.06 + 7.21 + 1 (buy_key) + 17.00 = {cost:.2f}', 12, 'agent')
    svg.text((50, 820), 'стоимость ребра = 3D-длина между позициями ролей (cost в .routes.json не задан); вид сверху: X →, Z ↓', 11, 'axis')
    return svg.render()


@figure('g12-playground-turn', 'Steering Playground: острый поворот Corner → Goal, look-ahead и плоскость перехода (реальные узлы, увеличено у угла)',
        'геометрия (вычислено из уровня)', 'детали', ['RF-LOOKAHEAD', 'RF-PASSED', 'AS-SC-PLAYGROUND'])
def playground_turn():
    s, c, g = (-8.0, 6.0), (8.0, 6.0), (4.0, 8.0)
    p = (7.6, 6.1)
    q = m.lookahead_target(p, s, c, g)
    _, n = m.passed_corner(p, s, c, g)
    out = m._norm(g[0] - c[0], g[1] - c[1])
    svg = Svg('g12-playground-turn', 760, 560, 'Playground turn')
    v = View(6.3, 4.95, 260, 30, 40)
    grid(svg, v, [x * 0.25 for x in range(26, 38)], [z * 0.25 for z in range(20, 28)])
    svg.poly([v(6.3, 6.0), v(*c)], 'route', 4)
    tail = (c[0] + out[0] * 1.55, c[1] + out[1] * 1.55)
    svg.poly([v(*c), v(*tail)], 'route', 4)
    svg.text(v(6.35, 5.92), '← от Route_Start (−8, 6)', 13, 'route')
    svg.text((v(*tail)[0] + 8, v(*tail)[1] + 22), '↙ к Goal (4, 8)', 13, 'route')
    svg.text((v(*c)[0] + 10, v(*c)[1] + 22), 'Corner (8, 6)', 13, 'route', weight='bold')
    svg.circle(v(*c), 0.75 * v.s, 'virtual', dash='6 4')
    svg.text(v(8.15, 5.2), 'окрестность L = 0.75', 12, 'virtual')
    t = (-n[1], n[0])
    svg.line(v(c[0] + t[0] * 0.95, c[1] + t[1] * 0.95), v(c[0] - t[0] * 0.95, c[1] - t[1] * 0.95), 'plane', 2.5)
    svg.line(v(*c), v(c[0] + n[0] * 0.55, c[1] + n[1] * 0.55), 'plane', 3, arrow=True)
    svg.text(v(c[0] + n[0] * 0.55 + 0.04, c[1] + n[1] * 0.55 + 0.02), f'n̂ = ({n[0]:.3f}, {n[1]:.3f})', 13, 'plane')
    svg.text(v(8.95, 5.62), 'плоскость перехода', 12, 'plane', 'end')
    svg.circle(v(*p), 6, 'agent', 'agent')
    svg.text(v(7.05, 6.42), 'p = (7.6, 6.1): d = 0.412, dot = 0.0053 ≥ 0', 13, 'agent')
    svg.line(v(7.42, 6.36), v(*p), 'agent', 1, '2 3')
    svg.circle(v(*q), 6, 'virtual', 'virtual')
    svg.line(v(*p), v(*q), 'agent', 2, arrow=True, pad=6)
    svg.text(v(7.05, 6.62), f'q = c + û_out·(0.75 − 0.412) = ({q[0]:.3f}, {q[1]:.3f})', 13, 'virtual')
    svg.line(v(7.9, 6.56), v(*q), 'virtual', 1, '2 3')
    p2 = (7.5, 5.9)
    svg.circle(v(*p2), 5, 'soft', 'soft')
    svg.text((v(*p2)[0] - 10, v(*p2)[1] - 10), 'p′ = (7.5, 5.9): до плоскости', 12, 'soft', 'end')
    svg.text((30, 545), 'вид сверху: X →, Z ↓; L = 0.75; acceptance 0.2, slowing 2.0 (сценарий startMoveTo)', 12, 'axis')
    return svg.render()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    meta = {}
    for ident, (fn, info) in FIGURES.items():
        (OUT / f'{ident}.svg').write_text(fn(), encoding='utf-8')
        meta[ident] = info
    (ATLAS / 'docs' / 'data' / 'geometry-meta.json').write_text(json.dumps(meta, indent=1, ensure_ascii=False),
                                                                encoding='utf-8')
    print(len(meta), 'geometry figures')


if __name__ == '__main__':
    main()
