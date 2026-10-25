"""Генерация reference-страниц атласа из проверенных данных (входы только читаются):

  docs/reference/source-references.md   реестр источников (резолвится tools/references.py; падение при ошибке)
  docs/data/source-references.json      машинная копия реестра со SHA-256 файлов (база для check_drift.py)
  docs/reference/class-index.md         индекс сущностей с фильтрами (домен, вид, текст)
  docs/reference/classes/<Name>.md      карточки: ручной разбор (tools/class_notes.py) + автоматическая часть
  docs/reference/diagrams.md            каталог схем (заголовки .mmd / gen_geometry.py, места использования)
"""
import html
import json
import re
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
DOCS = ATLAS / 'docs'
sys.path.insert(0, str(ATLAS / 'tools'))
import references  # noqa: E402
import class_notes  # noqa: E402

GITHUB = 'https://github.com/Fedorr47/CoreEngineModule/blob'
LANG = {'.cppm': 'cpp', '.cpp': 'cpp', '.h': 'cpp', '.inl': 'cpp', '.json': 'json'}
GROUPS = [
    ('Ядро GOAP и выбор цели', ('WS-', 'DC-', 'SO-', 'PL-', 'GS-')),
    ('Исполнение плана и decision runtime', ('PE-', 'BR-', 'DR-')),
    ('Action runtime и AISystem', ('AR-', 'AB-', 'AT-', 'AS-')),
    ('Решение, instance и отладка', ('GD-', 'GG-')),
    ('Авторинг', ('AU-',)),
    ('Композиция', ('CO-',)),
    ('Встроенные компоненты', ('CP-',)),
    ('Навигационные действия AI', ('NA-',)),
    ('Маршруты и steering', ('RT-', 'RS-', 'RF-', 'ST-', 'MU-')),
    ('Obstacle avoidance', ('OA-',)),
    ('Traversal и физика', ('TV-', 'PH-', 'CM-')),
    ('Ресурсы, события, брони', ('EV-', 'RV-', 'PK-')),
    ('Runtime-фасад и порядок кадра', ('GR-', 'AP-')),
    ('NavMesh', ('NV-',)),
    ('Development-сценарии', ('SC-',)),
    ('Ассеты', ('AS-CATALOG', 'AS-MARKER', 'AS-RECOVERY', 'AS-ACCESS', 'AS-SC')),
]


def group_of(identifier):
    best = None
    for title, prefixes in GROUPS:
        for prefix in prefixes:
            if identifier.startswith(prefix) and (best is None or len(prefix) > best[1]):
                best = (title, len(prefix))
    return best[0] if best else 'Прочее'


def page_text(lines):
    # Без хвостовых пробелов (пустые строки отступленных выдержек) и пустой строки в конце файла.
    text = '\n'.join(lines)
    return '\n'.join(line.rstrip() for line in text.split('\n')).rstrip('\n') + '\n'


def fence(language, start, lines):
    body = '\n'.join(line.replace('\t', '    ') for line in lines)
    marks = '````' if '```' in body else '```'
    return f'{marks}{language} linenums="{start}"\n{body}\n{marks}'


def source_references():
    resolved, failures = references.resolve()
    if failures:
        raise SystemExit('Unresolved references:\n' + '\n'.join(failures))
    snap = references.snapshot()
    head = snap['head']
    records = []
    for entry in resolved:
        start, end, lines = references.excerpt(entry['id'], 8)
        record = dict(entry)
        record['excerpt_sha256'] = __import__('hashlib').sha256('\n'.join(lines).encode('utf-8')).hexdigest()
        records.append(record)
    (DOCS / 'data' / 'source-references.json').write_text(json.dumps(dict(head=head, branch=snap['branch'],
        references=records), indent=1, ensure_ascii=False), encoding='utf-8')
    out = ['# Реестр источников', '',
           f'Каждая ссылка вида `[ID]` в атласе ведёт сюда. Строки **вычислены при сборке** инструментом '
           f'`tools/references.py`: для каждой записи найдено n-е вхождение ожидаемого текста в файле текущего '
           f'рабочего дерева. Отсутствующий текст ломает сборку, поэтому номера строк соответствуют снимку '
           f'`{snap["branch"]}` @ `{head[:12]}` на момент сборки. Колонка «состояние» показывает, был ли файл '
           f'изменён относительно HEAD (`working-tree`) — для таких файлов ссылка GitHub не является доказательством.',
           '', 'Ссылка «файл» открывает исходник из рабочего дерева рядом с атласом (работает из `file://`), '
           '«GitHub» — дополнительная навигация к тому же коммиту.', '']
    by_group = {}
    for record in records:
        by_group.setdefault(group_of(record['id']), []).append(record)
    order = [title for title, _ in GROUPS] + ['Прочее']
    for title in order:
        if title not in by_group:
            continue
        out += [f'## {title}', '']
        for record in sorted(by_group[title], key=lambda r: r['id']):
            start, end, lines = references.excerpt(record['id'], 8)
            local = '../../../../' + record['path']
            remote = f'{GITHUB}/{head}/{record["path"]}#L{record["line"]}'
            language = LANG.get(Path(record['path']).suffix, 'text')
            expected = record['expected'].replace('`', 'ˋ')
            out += [f'#### {record["id"]} {{#{record["id"].lower()}}}', '',
                    f'`{record["path"]}:{record["line"]}` · {record["description"]} · состояние: `{record["state"]}` · '
                    f'вхождение {record["nth"]} из {record["occurrences"]} · '
                    f'<a href="{local}">файл</a> · <a href="{remote}">GitHub</a>', '',
                    f'Якорь: `` {expected} ``', '',
                    '??? note "Фрагмент (8 строк от якоря)"', '',
                    '    ' + fence(language, start, lines).replace('\n', '\n    '), '']
    (DOCS / 'reference' / 'source-references.md').write_text(page_text(out), encoding='utf-8')
    return records


def class_pages():
    index = json.loads((DOCS / 'data' / 'class-index.json').read_text(encoding='utf-8'))
    notes = class_notes.NOTES
    head = references.snapshot()['head']
    types = index['types']
    names = {entry['name'] for entry in types}
    functions = [fn for fn in index['functions'] if fn['exported'] or fn['name'] in notes]
    unknown_notes = sorted(set(notes) - names - {fn['name'] for fn in functions})
    if unknown_notes:
        raise SystemExit('class_notes.py describes unknown entities: ' + ', '.join(unknown_notes))
    out_dir = DOCS / 'reference' / 'classes'
    out_dir.mkdir(parents=True, exist_ok=True)
    for stale in out_dir.glob('*.md'):
        stale.unlink()
    kind_ru = {'class': 'class', 'struct': 'struct', 'enum': 'enum', 'alias': 'alias', 'function': 'free function'}
    rows = []
    entities = [dict(entry, kind=entry['kind']) for entry in types]
    for fn in functions:
        if fn['name'] in names:
            continue
        entities.append(dict(name=fn['name'], kind='function', file=fn['file'], line=fn['line'], end_line=fn['line'],
                             domain=fn['domain'], exported=fn['exported'], internal=fn['internal'], members=[],
                             bases='', signature=fn['signature']))
    seen = set()
    for entity in sorted(entities, key=lambda e: (e['domain'], e['name'])):
        if entity['name'] in seen:
            continue
        seen.add(entity['name'])
        note = notes.get(entity['name'], {})
        purpose = note.get('purpose', '')
        manual = 'ручной' if note else 'авто'
        rows.append(f'| [{entity["name"]}](classes/{entity["name"]}.md) | {kind_ru[entity["kind"]]} | {entity["domain"]} | '
                    f'{"да" if entity["exported"] else "нет"} | `{entity["file"]}:{entity["line"]}` | {manual} | '
                    f'{purpose.replace("|", "/")} |')
        write_card(out_dir, entity, note, head)
    page = ['# Индекс сущностей', '',
            'Автоматический индекс собран `tools/build_class_index.py` эвристическим парсером из '
            f'{len(index["coverage"]["files"])} файлов AI, навигации, traversal и связанных систем '
            '(список — в конце страницы). Колонка «разбор» отделяет **ручной** архитектурный разбор '
            '(`tools/class_notes.py`: владелец, lifetime, вызовы, теория, тесты) от **автоматической** карточки '
            '(объявление, члены, строки). Тривиальные значения enum не получают отдельных карточек.', '',
            'Путь чтения: сущность → карточка (метод/контракт) → глава с теорией → [реестр источников]'
            '(source-references.md) → фрагмент кода.', '',
            '<div data-filter-table markdown>', '',
            '| Сущность | Вид | Домен | Экспорт | Объявление | Разбор | Назначение |',
            '|---|---|---|---|---|---|---|'] + rows + ['', '</div>', '',
            '## Покрытие автоматического индекса', '',
            f'Сущностей: {len(rows)} (из них с ручным разбором: {sum(1 for r in rows if "| ручной |" in r)}).', '',
            '??? note "Просканированные файлы"', ''] + [f'    - `{path}`' for path in index['coverage']['files']] + ['']
    (DOCS / 'reference' / 'class-index.md').write_text(page_text(page), encoding='utf-8')
    return len(rows)


def write_card(out_dir, entity, note, head):
    lines = [f'# {entity["name"]}', '']
    status = '<span class="ev ev-source">SOURCE_VERIFIED</span>'
    meta = (f'**{entity["kind"]}** · домен «{entity["domain"]}» · `{entity["file"]}:{entity["line"]}–{entity["end_line"]}` · '
            f'{"экспортируется модулем" if entity["exported"] else "внутренняя сущность модуля"} · {status} (снимок `{head[:7]}`)')
    lines += [meta, '']
    if entity.get('bases'):
        lines += [f'Реальные базы / определение: `{entity["bases"]}`', '']
    if note:
        lines += [f'> {note.get("purpose", "")}', '', '## Архитектурный разбор', '']
        for key, title in (('owner', 'Владелец и время жизни'), ('data', 'Основные данные: readers / writers'),
                           ('methods', 'Runtime-методы, входы и выходы'), ('callers', 'Кто вызывает и в какой фазе'),
                           ('calls', 'Что вызывает сама сущность'), ('errors', 'Ошибки, terminal states, отмена')):
            if note.get(key):
                lines += [f'**{title}.** {note[key]}', '']
        links = []
        for chapter in note.get('theory', []):
            links.append(f'[теория: {chapter}](../../chapters/{chapter}.md)')
        for diagram in note.get('diagrams', []):
            links.append(f'[схема {diagram}](../diagrams.md#{diagram})')
        for ref in note.get('refs', []):
            links.append(f'[{ref}](../source-references.md#{ref.lower()})')
        for test in note.get('tests', []):
            links.append(f'тест `{test}`')
        if links:
            lines += ['**Ссылки.** ' + ' · '.join(links), '']
    else:
        lines += ['Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. '
                  'Смысл и место в архитектуре см. в главах по домену «' + entity['domain'] + '».', '']
    members = entity.get('members', [])
    if members:
        lines += ['## Члены (автоматически)', '', '| Строка | Вид | Доступ | Сигнатура / поле |', '|---:|---|---|---|']
        kinds = {'ctor': 'конструктор', 'method': 'метод', 'field': 'поле'}
        for member in members:
            sig = member['signature'].replace('|', '\\|').replace('`', 'ˋ')
            lines.append(f'| {member["line"]} | {kinds[member["kind"]]} | {member["access"]} | `{sig}` |')
        lines.append('')
    source = (references.REPO / entity['file']).read_text(encoding='utf-8-sig', errors='replace').splitlines()
    start, end = entity['line'], min(entity['end_line'], entity['line'] + 60)
    language = LANG.get(Path(entity['file']).suffix, 'text')
    title = 'Объявление' if end == entity['end_line'] else 'Начало объявления (первые 61 строка)'
    lines += [f'## {title}', '', f'Включено из исходника при сборке; `{entity["file"]}:{start}–{end}`.', '',
              fence(language, start, source[start - 1:end]), '']
    (out_dir / f'{entity["name"]}.md').write_text(page_text(lines), encoding='utf-8')


def diagram_catalogue():
    meta = json.loads((DOCS / 'data' / 'diagram-meta.json').read_text(encoding='utf-8'))
    usage = {}
    for page in sorted(DOCS.rglob('*.md')):
        for ident in re.findall(r'@@diagram\s+([a-z0-9\-]+)', page.read_text(encoding='utf-8')):
            usage.setdefault(ident, []).append(page.relative_to(DOCS).as_posix())
    lines = ['# Каталог схем', '',
             'Все схемы хранятся как редактируемый текст (`diagrams/mermaid/*.mmd`) или как вычисляемые построения '
             '(`tools/gen_geometry.py`) и рендерятся локально в SVG (`tools/render_diagrams.py`). '
             'Семантика стрелок: вызов, данные, событие, владение или переход указаны в подписи каждой схемы.', '',
             '<div data-filter-table markdown>', '',
             '| Схема | Название | Вид | Уровень | Источник | Код | Используется |', '|---|---|---|---|---|---|---|']
    for ident in sorted(meta):
        info = meta[ident]
        refs = ', '.join(f'[{r}](source-references.md#{r.lower()})' for r in info.get('refs', []))
        used = ', '.join(f'[{p}](../{p})' for p in usage.get(ident, [])) or '—'
        lines.append(f'| <span id="{ident}"></span>[`{ident}`](../assets/diagrams/{ident}.svg) | '
                     f'{info.get("title", "")} | {info.get("kind", "")} | {info.get("level", "")} | '
                     f'`{info.get("source", "")}` | {refs} | {used} |')
    lines += ['', '</div>', '']
    (DOCS / 'reference' / 'diagrams.md').write_text(page_text(lines), encoding='utf-8')
    unused = sorted(set(meta) - set(usage))
    return len(meta), unused


if __name__ == '__main__':
    report = DOCS / 'reference' / 'validation-report.md'
    if not report.is_file():
        report.write_text('# Отчёт автоматической проверки\n\nОтчёт ещё не построен: запустите tools/build_site.py.\n',
                          encoding='utf-8')
    records = source_references()
    count = class_pages()
    diagrams, unused = diagram_catalogue()
    print(len(records), 'references;', count, 'index entries;', diagrams, 'diagrams; unused:', ', '.join(unused) or 'none')
