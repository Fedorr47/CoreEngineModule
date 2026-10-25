"""Автоматический индекс сущностей AI-подсистемы (эвристический построчный парсер C++ модулей).

Находит class/struct/enum/using-alias и свободные функции в выбранных файлах, их члены (методы, поля) и
диапазоны строк. Это АВТОМАТИЧЕСКОЕ покрытие: сигнатуры берутся из первой строки объявления, многострочные
параметры не разворачиваются. Ручной архитектурный разбор хранится отдельно (tools/class_notes.py).
Пишет docs/data/class-index.json.
"""
import json
import re
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
REPO = ATLAS.parents[1]
sys.path.insert(0, str(ATLAS / 'tools'))

SCAN = [
    ('src/Gameplay/AI', '**/*.cppm'),
    ('src/Gameplay/Navigation', '*.cppm'),
    ('src/Gameplay/Traversal', '*.cppm'),
    ('src/Navigation', 'Navigation.cppm'),
    ('src/Gameplay/Interaction', 'GameplayObjectReservationSystem.cppm'),
    ('src/Gameplay/Pickup', 'GameplayPickupSystem.cppm'),
    ('src/Gameplay/Events', 'GameplayWorldEvent.cppm'),
    ('src/App', 'GameplayPhysicsObstacleQuery.h'),
]
EXTRA_TYPES = {('src/Gameplay/Runtime/GameplayRuntime.cppm', 'GameplayRuntime')}

DOMAINS = [
    ('src/Gameplay/AI/GOAP/Authoring/', 'Авторинг'),
    ('src/Gameplay/AI/GOAP/Debug/', 'Диагностика'),
    ('src/Gameplay/AI/GOAP/AIPlanExecution', 'Исполнение плана'),
    ('src/Gameplay/AI/GOAP/AIDecisionRuntime', 'Исполнение плана'),
    ('src/Gameplay/AI/GOAP/GameplayGOAP', 'Решение (instance)'),
    ('src/Gameplay/AI/GOAP/', 'Ядро GOAP'),
    ('src/Gameplay/AI/Runtime/GameplayAIDecision', 'Решение (instance)'),
    ('src/Gameplay/AI/Runtime/', 'Action runtime'),
    ('src/Gameplay/AI/Composition/Components/', 'Компоненты'),
    ('src/Gameplay/AI/Composition/', 'Композиция'),
    ('src/Gameplay/AI/Navigation/', 'Навигационные действия'),
    ('src/Gameplay/Navigation/GameplayObstacleAvoidance', 'Obstacle avoidance'),
    ('src/Gameplay/Navigation/GameplaySteering.cppm', 'Steering'),
    ('src/Gameplay/Navigation/GameplayRoute', 'Маршруты'),
    ('src/Gameplay/Navigation/', 'Диагностика'),
    ('src/Gameplay/Traversal/', 'Traversal'),
    ('src/Navigation/', 'NavMesh'),
    ('src/Gameplay/Runtime/', 'Runtime-фасад'),
    ('src/App/', 'Интеграция App'),
    ('src/Gameplay/', 'Ресурсы и события'),
]

TYPE_RE = re.compile(r'^\s*(?:export\s+)?(class|struct)\s+([A-Za-z_]\w*)\s*(final)?\s*(?::\s*(.*?))?\s*(\{.*)?$')
ENUM_RE = re.compile(r'^\s*(?:export\s+)?enum\s+(?:class\s+)?([A-Za-z_]\w*)\b')
ALIAS_RE = re.compile(r'^\s*using\s+([A-Za-z_]\w*)\s*=\s*(.+?);')
NS_RE = re.compile(r'^\s*(export\s+)?namespace\s*([\w:]*)\s*(\{)?')
FUNC_RE = re.compile(r'^\s*(?:\[\[nodiscard\]\]\s*)?(?:(?:inline|constexpr|static|virtual|explicit|friend)\s+)*'
                     r'(?P<ret>[\w:<>,\*&\s]+?)\s+(?P<qual>[\w:]*::)?(?P<name>~?[A-Za-z_]\w*)\s*\(')
CTOR_RE = re.compile(r'^\s*(?:explicit\s+)?(?:constexpr\s+)?(?P<name>~?[A-Za-z_]\w*)\s*\(')
FIELD_RE = re.compile(r'^\s*(?:static\s+)?(?:constexpr\s+)?(?:inline\s+)?(?:const\s+)?(?P<type>[\w:<>,\*&\s]+?)\s+'
                      r'(?P<name>[A-Za-z_]\w*)\s*(?:\{[^;]*\}|=[^;]*)?;\s*(?://.*)?$')
KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'catch', 'sizeof', 'static_assert', 'decltype', 'assert',
            'case', 'else', 'throw', 'new', 'delete', 'operator'}


def strip_code(line, state):
    """Удаляет строки, символьные литералы и комментарии; state['block'] — внутри /* */."""
    out, i = [], 0
    while i < len(line):
        if state['block']:
            end = line.find('*/', i)
            if end < 0:
                return ''.join(out)
            state['block'] = False
            i = end + 2
            continue
        ch = line[i]
        if line.startswith('//', i):
            break
        if line.startswith('/*', i):
            state['block'] = True
            i += 2
            continue
        if ch in '"\'':
            quote, j = ch, i + 1
            while j < len(line) and line[j] != quote:
                j += 2 if line[j] == '\\' else 1
            out.append(quote + quote)
            i = j + 1
            continue
        out.append(ch)
        i += 1
    return ''.join(out)


def domain_of(path):
    for prefix, name in DOMAINS:
        if path.startswith(prefix):
            return name
    return 'Прочее'


def parse(path):
    lines = (REPO / path).read_text(encoding='utf-8-sig', errors='replace').splitlines()
    state = {'block': False}
    depth = 0
    stack = []          # (kind, name, depth_inside, extra)
    pending = None      # объявление, ожидающее '{'
    types, functions = [], []
    for number, raw in enumerate(lines, 1):
        code = strip_code(raw, state)
        stripped = code.strip()
        top = stack[-1] if stack else None
        in_namespace = top is None or top[0] == 'ns'
        in_type = top is not None and top[0] == 'type' and depth == top[2]
        if stripped and pending is None:
            if in_namespace:
                ns = NS_RE.match(code)
                t = TYPE_RE.match(code)
                e = ENUM_RE.match(code)
                a = ALIAS_RE.match(code)
                if ns and not t:
                    exported = bool(ns.group(1)) or (top is not None and top[3].get('exported'))
                    internal = ns.group(2) in ('', ) or ns.group(2).endswith('_detail') or ns.group(2) == 'details'
                    if ns.group(2) == '' and not ns.group(1):
                        internal = True
                    pending = ('ns', ns.group(2), {'exported': exported and not internal, 'internal': internal})
                elif t and not stripped.endswith(';'):
                    entry = dict(name=t.group(2), kind=t.group(1), file=path, line=number, bases=(t.group(4) or '').strip(' {'),
                                 exported=bool(top and top[3].get('exported')) or code.lstrip().startswith('export'),
                                 internal=bool(top and top[3].get('internal')), members=[])
                    types.append(entry)
                    pending = ('type', t.group(2), {'entry': entry, 'access': 'private' if t.group(1) == 'class' else 'public'})
                elif e and not stripped.endswith(';'):
                    entry = dict(name=e.group(1), kind='enum', file=path, line=number, bases='',
                                 exported=bool(top and top[3].get('exported')), internal=bool(top and top[3].get('internal')),
                                 members=[])
                    types.append(entry)
                    pending = ('enum', e.group(1), {'entry': entry})
                elif a:
                    types.append(dict(name=a.group(1), kind='alias', file=path, line=number, end_line=number,
                                      bases=a.group(2), exported=bool(top and top[3].get('exported')),
                                      internal=bool(top and top[3].get('internal')), members=[]))
                else:
                    f = FUNC_RE.match(code)
                    if f and f.group('name') not in KEYWORDS and f.group('ret').strip() not in KEYWORDS \
                            and not stripped.startswith(('return', 'using', 'typedef', '#')):
                        functions.append(dict(name=f.group('name'), file=path, line=number,
                                              signature=raw.strip().rstrip('{').strip(),
                                              qualified=bool(f.group('qual')),
                                              exported=bool(top and top[3].get('exported')),
                                              internal=bool(top and top[3].get('internal')),
                                              declaration=stripped.endswith(';')))
            elif in_type:
                info = top[3]
                entry = info['entry']
                if re.match(r'^(public|private|protected)\s*:', stripped):
                    info['access'] = stripped.split(':')[0]
                elif top[0] == 'type':
                    ctor = CTOR_RE.match(code)
                    f = FUNC_RE.match(code)
                    if ctor and ctor.group('name').lstrip('~') == entry['name']:
                        entry['members'].append(dict(kind='ctor', name=ctor.group('name'), line=number,
                                                     signature=raw.strip().rstrip('{').strip(), access=info['access']))
                    elif f and f.group('name') not in KEYWORDS and not stripped.startswith(('return', 'using', 'friend constexpr bool operator')):
                        entry['members'].append(dict(kind='method', name=f.group('name'), line=number,
                                                     signature=raw.strip().rstrip('{').strip(), access=info['access']))
                    else:
                        fld = FIELD_RE.match(code)
                        if fld and not stripped.startswith(('return', 'using', 'friend', 'class ', 'struct ')) \
                                and fld.group('type').strip() not in KEYWORDS:
                            entry['members'].append(dict(kind='field', name=fld.group('name'), line=number,
                                                         signature=raw.strip(), access=info['access']))
        # Учёт скобок и открытие отложенного блока
        for ch in code:
            if ch == '{':
                depth += 1
                if pending is not None:
                    kind, name, info = pending
                    stack.append((kind if kind != 'enum' else 'enum', name, depth, info))
                    pending = None
                else:
                    stack.append(('block', '', depth, {}))
            elif ch == '}':
                if stack and stack[-1][2] == depth:
                    kind, name, _, info = stack.pop()
                    if kind in ('type', 'enum'):
                        info['entry']['end_line'] = number
                depth -= 1
        if pending is not None and stripped.endswith(';') and '{' not in code:
            pending = None
    for entry in types:
        entry.setdefault('end_line', entry['line'])
        entry['domain'] = domain_of(path)
    return types, functions


def files():
    for root, pattern in SCAN:
        for path in sorted((REPO / root).glob(pattern)):
            yield path.relative_to(REPO).as_posix()


def main():
    types, functions = [], []
    for path in files():
        t, f = parse(path)
        types += [entry for entry in t if not entry['internal'] or entry['kind'] in ('class', 'struct')]
        for fn in f:
            fn['domain'] = domain_of(path)
        functions += f
    for path, name in EXTRA_TYPES:
        t, _ = parse(path)
        types += [entry for entry in t if entry['name'] == name]
    # Дубликаты имён (например, приватные Impl) — оставляем первое объявление, отмечаем повтор.
    seen, unique = {}, []
    for entry in types:
        if entry['name'] in seen:
            seen[entry['name']].setdefault('duplicates', []).append(f"{entry['file']}:{entry['line']}")
            continue
        seen[entry['name']] = entry
        unique.append(entry)
    # Свободные функции: только из экспортируемых/интерфейсных пространств, без out-of-line определений.
    free, fseen = [], set()
    for fn in functions:
        if fn['qualified'] or fn['name'] in seen:
            continue
        key = (fn['name'], fn['file'])
        if key in fseen:
            continue
        fseen.add(key)
        free.append(fn)
    data = dict(types=sorted(unique, key=lambda e: (e['domain'], e['name'])),
                functions=sorted(free, key=lambda e: (e['domain'], e['name'])),
                coverage=dict(files=sorted(set(files())), method='heuristic line parser (tools/build_class_index.py)'))
    (ATLAS / 'docs' / 'data').mkdir(parents=True, exist_ok=True)
    (ATLAS / 'docs' / 'data' / 'class-index.json').write_text(json.dumps(data, indent=1, ensure_ascii=False), encoding='utf-8')
    print(len(unique), 'types,', len(free), 'free functions from', len(data['coverage']['files']), 'files')


if __name__ == '__main__':
    main()
