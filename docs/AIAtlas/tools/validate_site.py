"""Проверка исходников и собранного AI-атласа. Пишет docs/reference/validation-report.md и docs/data/validation.json.
Код выхода 1 при любом провале.

Проверки:
  1. все записи реестра источников резолвятся; все [[ID]] / @@code ID в Markdown существуют
  2. каждая .mmd отрендерена в непустой SVG; каждая @@diagram ссылается на существующую схему
  3. C++-идентификаторы в подписях схем существуют в индексе сущностей (кроме схем со статусом PROPOSED)
  4. структура тематических глав: обязательные разделы, теория, схема, выдержка кода, ссылки на реестр
  5. PROPOSED-схемы встраиваются только в главу 20
  6. собранный сайт: страницы nav существуют, внутренние ссылки/якоря/ресурсы резолвятся,
     офлайн-индекс поиска присутствует, нет внешних script/link/img (CDN), наши JS не используют fetch()
Не проверяет: корректность смысла стрелок (это ручной обзор), поведение движка, визуальное качество.
"""
import html.parser
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ATLAS = Path(__file__).resolve().parents[1]
DOCS = ATLAS / 'docs'
SITE = ATLAS / 'site'
sys.path.insert(0, str(ATLAS / 'tools'))
import references  # noqa: E402

THEMATIC = re.compile(r'^(0[1-9]|1\d|20)-')
REQUIRED_SECTIONS = [
    ('задача', r'^##\s+.*(Задача|интуиция)'),
    ('теория', r'^##\s+.*(Теори|выкладк|Вывод)'),
    ('реализация', r'^##\s+.*(CoreEngine|Реализац)'),
    ('пример', r'^##\s+.*(пример|Пример)'),
    ('границы', r'^##\s+.*(Границ|ошибк|Ошибк)'),
]
SYMBOL = re.compile(r'\b([A-Z][A-Za-z0-9]*[a-z][A-Za-z0-9]*[A-Z]\w*)(?:::(~?\w+))?')


class Links(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.hrefs, self.ids, self.external = [], set(), []

    def handle_starttag(self, tag, attrs):
        attributes = dict(attrs)
        if 'id' in attributes:
            self.ids.add(attributes['id'])
        if tag == 'a' and 'name' in attributes:
            self.ids.add(attributes['name'])
        for key in ('href', 'src'):
            value = attributes.get(key)
            if not value:
                continue
            if tag in ('script', 'link', 'img', 'iframe') and re.match(r'^(https?:)?//', value):
                self.external.append(f'<{tag} {key}={value}>')
            if tag in ('a', 'link', 'script', 'img'):
                self.hrefs.append(value)


def words(text):
    return len(re.findall(r'[A-Za-zА-Яа-яЁё0-9_]+', text))


def chapter_checks(page, text, failures):
    result = dict(page=page.name)
    for name, pattern in REQUIRED_SECTIONS:
        result[name] = bool(re.search(pattern, text, re.M))
        if not result[name]:
            failures.append(f'{page.name}: нет обязательного раздела «{name}»')
    theory = re.search(r'^##\s+[^\n]*(?:Теори|выкладк|Вывод)[^\n]*\n(.*?)(?=^##\s|\Z)', text, re.M | re.S)
    result['theory_words'] = words(theory.group(1)) if theory else 0
    result['words'] = words(text)
    result['diagrams'] = len(re.findall(r'^@@diagram\s', text, re.M))
    result['code'] = len(re.findall(r'^@@code\s', text, re.M))
    result['citations'] = len(re.findall(r'\[\[[A-Z][A-Z0-9\-]+\]\]', text))
    result['formulas'] = text.count('$$') // 2 + len(re.findall(r'(?<!\$)\$(?!\$)[^$\n]+\$', text))
    result['examples'] = bool(re.search(r'(?i)пример', text))
    if result['theory_words'] < 250:
        failures.append(f'{page.name}: теория слишком короткая ({result["theory_words"]} слов < 250)')
    for key, minimum in (('diagrams', 1), ('code', 1), ('citations', 3)):
        if result[key] < minimum:
            failures.append(f'{page.name}: {key} = {result[key]} < {minimum}')
    return result


def main():
    final = '--final' in sys.argv
    results, failures, notes = {}, [], []
    resolved, unresolved = references.resolve()
    results['references'] = dict(resolved=len(resolved), unresolved=unresolved)
    failures += [f'reference: {item}' for item in unresolved]
    ids = {entry['id'] for entry in resolved}

    index = json.loads((DOCS / 'data' / 'class-index.json').read_text(encoding='utf-8'))
    type_members = {t['name']: {m['name'] for m in t['members']} for t in index['types']}
    known = set(type_members) | {f['name'] for f in index['functions']}
    known |= {'GameplayWorld', 'GameplayRuntimeMode', 'LevelAsset', 'LevelInstance', 'GameplayUpdateContext',
              'EntityHandle', 'JoltPhysicsWorld', 'GameplayCharacterCommandComponent', 'GameplayCharacterMotorComponent',
              'GameplayCharacterMovementStateComponent', 'GameplayInteractionPointComponent', 'GameplayPickupComponent',
              'GameplayTransformComponent', 'GameplayDoorComponent', 'GameplayActionComponent',
              'GameplayCharacterPhysicalSettingsComponent', 'AppState', 'ScenarioContext', 'DevelopmentScenarioRunner',
              'AppDevelopmentScenarioRuntime', 'LevelPhysicsRuntime', 'RecastNavigation', 'NavMesh'}

    for entry in index['types']:
        if entry['kind'] == 'enum':
            body = (references.REPO / entry['file']).read_text(encoding='utf-8-sig').splitlines()[entry['line']:entry['end_line']]
            known |= set(re.findall(r'[A-Z][A-Za-z0-9_]*', ' '.join(body)))
    # Любой идентификатор или имя файла, реально встречающиеся в src/: схема не может ссылаться на несуществующее.
    for source in (references.REPO / 'src').rglob('*'):
        if source.suffix in ('.cppm', '.cpp', '.h', '.inl'):
            known.add(source.stem)
            known |= set(re.findall(r'[A-Za-z_][A-Za-z0-9_]*', source.read_text(encoding='utf-8-sig', errors='replace')))
    # Понятийные имена действий/поведений, не являющиеся C++-символами
    known |= {'CoreEngine','MoveTo', 'FollowRoute', 'FollowTarget', 'FleeTarget', 'NavMesh', 'StaticWorld', 'DevelopmentScenario',
              'SteeringPlayground', 'MarkerVisit', 'TargetRecovery', 'AccessKey', 'InteractionPoint', 'CharacterCommand',
              'BeginFrame', 'PrePhysicsUpdate', 'PostPhysicsUpdate', 'PostAnimationUpdate', 'UpdateSkinned'}
    meta = json.loads((DOCS / 'data' / 'diagram-meta.json').read_text(encoding='utf-8'))
    sources = sorted((ATLAS / 'diagrams' / 'mermaid').glob('*.mmd'))
    missing = [p.stem for p in sources if not (DOCS / 'assets' / 'diagrams' / f'{p.stem}.svg').is_file()
               or (DOCS / 'assets' / 'diagrams' / f'{p.stem}.svg').stat().st_size < 500]
    failures += [f'diagram not rendered: {name}' for name in missing]
    unknown_symbols = []
    for path in sources:
        text = path.read_text(encoding='utf-8')
        if 'PROPOSED' in meta.get(path.stem, {}).get('status', ''):
            continue
        body = '\n'.join(line for line in text.splitlines() if not line.startswith('%%'))
        for name, member in set(SYMBOL.findall(body)):
            if name not in known:
                unknown_symbols.append(f'{path.name}: {name}')
            elif member and name in type_members and member not in type_members[name] \
                    and member.lstrip('~') != name:
                unknown_symbols.append(f'{path.name}: {name}::{member}')
    failures += [f'diagram symbol not in source index: {item}' for item in sorted(set(unknown_symbols))]
    results['diagrams'] = dict(mermaid=len(sources), total=len(meta), missing=missing,
                               unknown_symbols=sorted(set(unknown_symbols)))

    chapters, used_diagrams, used_refs = [], set(), set()
    for page in sorted(DOCS.rglob('*.md')):
        text = page.read_text(encoding='utf-8')
        for ident in re.findall(r'^@@diagram\s+([a-z0-9\-]+)', text, re.M):
            used_diagrams.add(ident)
            if ident not in meta:
                failures.append(f'{page.relative_to(DOCS)}: unknown diagram {ident}')
            elif 'PROPOSED' in meta[ident].get('status', '') and not page.name.startswith('20-') \
                    and page.name != 'diagrams.md':
                failures.append(f'{page.name}: PROPOSED diagram {ident} outside chapter 20')
        for ident in re.findall(r'\[\[([A-Z][A-Z0-9]*(?:-[A-Z0-9]+)*)\]\]', text) + \
                re.findall(r'^@@code\s+([A-Z][A-Z0-9\-]*)', text, re.M):
            if ident in ('SV', 'EV', 'INF', 'HIST', 'PROP', 'NV'):
                continue
            used_refs.add(ident)
            if ident not in ids:
                failures.append(f'{page.relative_to(DOCS)}: unknown reference {ident}')
        if page.parent.name == 'chapters' and THEMATIC.match(page.name):
            chapters.append(chapter_checks(page, text, failures))
    results['chapters'] = chapters
    results['unused_references'] = sorted(ids - used_refs)
    results['unused_diagrams'] = sorted(set(meta) - used_diagrams)
    if results['unused_diagrams']:
        notes.append('схемы без встраивания: ' + ', '.join(results['unused_diagrams']))

    site_problems, external = [], []
    if not SITE.is_dir():
        site_problems.append('site/ missing (run mkdocs build first)')
    else:
        pages = sorted(page for page in SITE.rglob('*.html') if page.name != '404.html')
        parsed = {}
        for page in pages:
            parser = Links()
            parser.feed(page.read_text(encoding='utf-8', errors='replace'))
            parsed[page.resolve()] = parser
            external += [f'{page.relative_to(SITE)}: {item}' for item in parser.external]
        checked = 0
        for page, parser in parsed.items():
            for href in parser.hrefs:
                split = urlsplit(href)
                if split.scheme or href.startswith(('mailto:', 'javascript:', 'data:')):
                    continue
                target = (page.parent / unquote(split.path)).resolve() if split.path else page
                checked += 1
                if not target.exists():
                    site_problems.append(f'{page.relative_to(SITE.resolve())}: missing {href}')
                    continue
                if split.fragment and target.suffix == '.html':
                    target_parser = parsed.get(target)
                    if target_parser and unquote(split.fragment) not in target_parser.ids:
                        site_problems.append(f'{page.relative_to(SITE.resolve())}: missing anchor {href}')
        results['site'] = dict(html_pages=len(pages), links_checked=checked,
                               offline_search_index=(SITE / 'search' / 'search_index.js').is_file())
        if not results['site']['offline_search_index']:
            site_problems.append('offline search index (search/search_index.js) missing')
    for script in (DOCS / 'assets' / 'js').glob('*.js'):
        if re.search(r'\bfetch\s*\(|XMLHttpRequest|import\s*\(', script.read_text(encoding='utf-8')):
            site_problems.append(f'{script.name}: runtime fetch/import breaks file://')
    failures += site_problems[:300] + [f'external resource: {item}' for item in external]
    results['site_problems'] = site_problems[:300]
    results['external'] = external
    results['failures'] = failures
    results['notes'] = notes
    results['status'] = 'PASS' if not failures else 'FAIL'
    (DOCS / 'data' / 'validation.json').write_text(json.dumps(results, indent=1, ensure_ascii=False), encoding='utf-8')
    write_report(results, final)
    print(results['status'], len(failures), 'failures')
    for failure in failures[:60]:
        print('  ', failure)
    return 0 if not failures else 1


def write_report(results, final):
    site = results.get('site', {})
    head = references.snapshot()['head']
    lines = ['# Отчёт автоматической проверки', '',
             f'**Статус автоматических проверок: {results["status"]}** — `tools/validate_site.py`, снимок `{head[:12]}`. '
             'Это проверка структуры, ссылок и источников документации; она **не** доказывает поведение движка и '
             'не заменяет визуальную/браузерную проверку (см. `audit/validation.md`).', '',
             '| Проверка | Результат |', '|---|---|',
             f'| Записи реестра источников | {results["references"]["resolved"]} резолвлено, '
             f'{len(results["references"]["unresolved"])} ошибок |',
             f'| Неиспользуемые записи реестра | {len(results["unused_references"])} |',
             f'| Схемы | Mermaid: {results["diagrams"]["mermaid"]}, всего: {results["diagrams"]["total"]}; '
             f'не отрендерено: {len(results["diagrams"]["missing"])}; без встраивания: {len(results["unused_diagrams"])} |',
             f'| C++-символы в схемах вне индекса | {len(results["diagrams"]["unknown_symbols"])} |',
             f'| HTML-страниц | {site.get("html_pages", 0)} |',
             f'| Внутренних ссылок/ресурсов проверено | {site.get("links_checked", 0)}; проблем: {len(results["site_problems"])} |',
             f'| Внешние ресурсы (CDN) | {len(results["external"])} |',
             f'| Офлайн-индекс поиска | {"есть" if site.get("offline_search_index") else "нет"} |', '',
             '## Тематические главы', '',
             '| Глава | Слов | Слов теории | Схем | Выдержек | Ссылок на реестр | Формул | Обязательные разделы |',
             '|---|---:|---:|---:|---:|---:|---:|---|']
    for chapter in results['chapters']:
        sections = 'все' if all(chapter[n] for n, _ in REQUIRED_SECTIONS) else \
            'нет: ' + ', '.join(n for n, _ in REQUIRED_SECTIONS if not chapter[n])
        lines.append(f'| {chapter["page"]} | {chapter["words"]} | {chapter["theory_words"]} | {chapter["diagrams"]} | '
                     f'{chapter["code"]} | {chapter["citations"]} | {chapter["formulas"]} | {sections} |')
    if results['notes']:
        lines += ['', '## Замечания', ''] + [f'- {note}' for note in results['notes']]
    if results['failures']:
        lines += ['', '## Провалы', ''] + [f'- {failure}' for failure in results['failures'][:200]]
    if not final:
        lines += ['', '_Предварительный отчёт: будет перезаписан после повторной сборки._']
    (DOCS / 'reference' / 'validation-report.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


if __name__ == '__main__':
    sys.exit(main())
