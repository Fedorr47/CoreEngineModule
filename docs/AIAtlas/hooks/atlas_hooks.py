"""MkDocs hooks AI-атласа CoreEngine.

Markdown-директивы (раскрываются до конвертации, ошибки ломают strict-сборку):
  [[REF-ID]]              ссылка на запись реестра источников (tools/references.py)
  [[SV]] [[EV]] [[INF]] [[HIST]] [[PROP]] [[NV]]   значки статуса доказательности
  [[cls:Name]]            ссылка на карточку сущности reference/classes/Name.md
  @@code REF-ID N         выдержка N строк, начиная со строки якоря (номер строки вычисляется при сборке)
HTML-директивы (после конвертации):
  @@diagram ID | Подпись      (отдельный абзац) → <figure> со встроенным SVG docs/assets/diagrams/ID.svg
Формулы: pymdownx.arithmatex (generic) → KaTeX при сборке (tools/katex_render.js), без JS в браузере.
"""
import html
import json
import re
import subprocess
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ATLAS / 'tools'))
import references  # noqa: E402

NODE_KATEX = ATLAS / 'tools' / 'katex_render.js'
DIAGRAM_SOURCES = {'mmd': ATLAS / 'diagrams' / 'mermaid', 'py': ATLAS / 'tools'}

BADGES = {
    'SV': ('ev-source', 'SOURCE_VERIFIED', 'Поведение прочитано в исходниках текущего снимка'),
    'EV': ('ev-evidence', 'EVIDENCE_VERIFIED', 'Есть проверяемый результат теста/запуска с датой и ревизией'),
    'INF': ('ev-inferred', 'INFERRED', 'Вывод из нескольких подтверждённых контрактов; основание указано рядом'),
    'HIST': ('ev-hist', 'HISTORICAL', 'Относится к прежнему снимку или старой документации'),
    'PROP': ('ev-proposed', 'PROPOSED', 'Предложение, не текущая реализация'),
    'NV': ('ev-notcov', 'NOT_VERIFIED', 'Данных недостаточно или проверка не выполнялась'),
}
STATE = {}

CITE = re.compile(r'\[\[([A-Z][A-Z0-9]*(?:-[A-Z0-9]+)*)\]\]')
CLS = re.compile(r'\[\[cls:([A-Za-z_][\w]*)\]\]')
CODE = re.compile(r'^@@code\s+([A-Z][A-Z0-9\-]*)(?:\s+(\d+))?\s*(?:@@)?\s*$', re.M)
DIAGRAM = re.compile(r'<p>@@diagram\s+([a-z0-9][a-z0-9\-]*)\s*\|\s*(.*?)(?:@@)?</p>', re.S)
MATH_INLINE = re.compile(r'<span class="arithmatex">\\\((.*?)\\\)</span>', re.S)
MATH_BLOCK = re.compile(r'<div class="arithmatex">\\\[(.*?)\\\]</div>', re.S)
VIEWBOX = re.compile(r'viewBox="\s*([-\d.eE]+)[\s,]+([-\d.eE]+)[\s,]+([\d.eE]+)[\s,]+([\d.eE]+)\s*"')
TABLE_CODE = re.compile(r'(<td[^>]*>(?:(?!</td>).)*?<code>)([^<]{18,})(</code>)', re.S)


def _break_points(text):
    """Точки переноса для длинных C++-идентификаторов в таблицах: перед заглавной после строчной, после ::, _ и /."""
    text = re.sub(r'(?<=[a-z0-9])(?=[A-Z])', '<wbr>', text)
    return re.sub(r'(::|_|/|\.)(?=\w)', r'\1<wbr>', text)


LANG = {'.cppm': 'cpp', '.cpp': 'cpp', '.h': 'cpp', '.inl': 'cpp', '.json': 'json', '.py': 'python'}


def _rel(page):
    return '../' * page.file.src_uri.count('/')


def on_config(config):
    resolved, failures = references.resolve()
    if failures:
        raise SystemExit('Unresolved source references:\n' + '\n'.join(failures))
    STATE['refs'] = {entry['id']: entry for entry in resolved}
    STATE['head'] = references.snapshot()['head']
    # Карточки генерирует tools/gen_reference_pages.py (типы и функции); ссылка допустима только на существующую.
    STATE['classes'] = {path.stem for path in (ATLAS / 'docs' / 'reference' / 'classes').glob('*.md')}
    meta_path = ATLAS / 'docs' / 'data' / 'diagram-meta.json'
    STATE['diagrams'] = json.loads(meta_path.read_text(encoding='utf-8')) if meta_path.is_file() else {}
    STATE['used_refs'] = set()
    return config


def on_page_markdown(markdown, page, config, files):
    rel = _rel(page)
    registry_page = f'{rel}reference/source-references.md'

    def code(match):
        identifier, count = match.group(1), int(match.group(2) or 12)
        if identifier not in STATE['refs']:
            raise SystemExit(f'{page.file.src_uri}: unknown code reference {identifier}')
        STATE['used_refs'].add(identifier)
        entry = STATE['refs'][identifier]
        start, end, lines = references.excerpt(identifier, count)
        language = LANG.get(Path(entry['path']).suffix, 'text')
        body = '\n'.join(line.replace('\t', '    ') for line in lines)
        fence = '````' if '```' in body else '```'
        return (f'{fence}{language} title="{entry["path"]}:{start}–{end}" linenums="{start}"\n{body}\n{fence}\n\n'
                f'Источник: [{identifier}]({registry_page}#{identifier.lower()}) · '
                f'`{entry["path"]}:{start}` · снимок `{STATE["head"][:7]}`\n{{: .src-ref }}\n')

    markdown = CODE.sub(code, markdown)

    def cite(match):
        identifier = match.group(1)
        if identifier in BADGES:
            css, label, title = BADGES[identifier]
            return f'<span class="ev {css}" title="{title}">{label}</span>'
        if identifier not in STATE['refs']:
            raise SystemExit(f'{page.file.src_uri}: unknown source reference [[{identifier}]]')
        STATE['used_refs'].add(identifier)
        return f'[{identifier}]({registry_page}#{identifier.lower()}){{: .cite }}'

    markdown = CITE.sub(cite, markdown)

    def cls(match):
        name = match.group(1)
        if STATE['classes'] and name not in STATE['classes']:
            raise SystemExit(f'{page.file.src_uri}: unknown class card [[cls:{name}]]')
        return f'[`{name}`]({rel}reference/classes/{name}.md)'

    return CLS.sub(cls, markdown)


def _inline_svg(identifier):
    path = ATLAS / 'docs' / 'assets' / 'diagrams' / f'{identifier}.svg'
    if not path.is_file():
        raise SystemExit(f'diagram {identifier}: missing {path}')
    svg = path.read_text(encoding='utf-8')
    svg = re.sub(r'<\?xml[^>]*\?>', '', svg)
    svg = re.sub(r'<!DOCTYPE[^>]*>', '', svg).strip()
    head_end = svg.find('>')
    head, rest = svg[:head_end + 1], svg[head_end + 1:]
    match = VIEWBOX.search(head)
    if not match:
        raise SystemExit(f'diagram {identifier}: SVG without viewBox')
    width, height = float(match.group(3)), float(match.group(4))
    head = re.sub(r'\s(width|height)="[^"]*"', '', head)
    head = re.sub(r'\sstyle="[^"]*"', '', head)
    head = head.replace('<svg', f'<svg width="{width:.0f}" height="{height:.0f}" '
                        f'style="--diagram-width:{width:.2f}" focusable="false"', 1)
    return (head + rest).replace('\n', ' '), width, height


def _render_math(items):
    if not items:
        return []
    process = subprocess.run(['node', str(NODE_KATEX)], input=json.dumps(items), capture_output=True,
                             text=True, encoding='utf-8', cwd=ATLAS)
    if process.returncode:
        raise SystemExit('KaTeX failed: ' + process.stderr[-2000:])
    return json.loads(process.stdout)


def on_page_content(content, page, config, files):
    rel = _rel(page).replace('.md', '.html')

    def diagram(match):
        identifier, caption = match.group(1), match.group(2).strip()
        svg, width, height = _inline_svg(identifier)
        meta = STATE['diagrams'].get(identifier, {})
        refs = ', '.join(f'<a class="cite" href="{rel}reference/source-references.html#{ref.lower()}">{ref}</a>'
                         for ref in meta.get('refs', []))
        source = meta.get('source', f'docs/assets/diagrams/{identifier}.svg')
        level = meta.get('level', '')
        kind = meta.get('kind', '')
        label = html.escape(re.sub('<[^>]+>', '', caption))
        details = ' · '.join(part for part in [
            f'вид: {html.escape(kind)}' if kind else '',
            f'уровень: {html.escape(level)}' if level else '',
            f'источник: <code>{html.escape(source)}</code>',
            f'код: {refs}' if refs else '',
            f'<a href="{rel}assets/diagrams/{identifier}.svg">SVG</a>',
            f'<a href="{rel}reference/diagrams.html#{identifier}">каталог</a>'] if part)
        return (f'<figure class="diagram" id="fig-{identifier}">'
                f'<div class="diagram-viewport" role="img" aria-label="{label}">{svg}</div>'
                f'<figcaption><span class="fig-title">{caption}</span>'
                f'<span class="fig-meta">{details}</span></figcaption></figure>')

    content = DIAGRAM.sub(diagram, content)
    content = TABLE_CODE.sub(lambda match: match.group(1) + _break_points(match.group(2)) + match.group(3), content)
    if '@@diagram' in content:
        raise SystemExit(f'{page.file.src_uri}: malformed @@diagram directive (must be its own paragraph)')

    items = []
    for match in MATH_BLOCK.finditer(content):
        items.append({'tex': html.unescape(match.group(1)), 'display': True})
    for match in MATH_INLINE.finditer(content):
        items.append({'tex': html.unescape(match.group(1)), 'display': False})
    rendered = iter(_render_math(items))
    content = MATH_BLOCK.sub(lambda _: f'<div class="math-display">{next(rendered)}</div>', content)
    content = MATH_INLINE.sub(lambda _: f'<span class="math-inline">{next(rendered)}</span>', content)
    return content


def on_post_build(config):
    used = ATLAS / 'audit' / 'generated' / 'used-references.json'
    used.parent.mkdir(parents=True, exist_ok=True)
    used.write_text(json.dumps(sorted(STATE.get('used_refs', set())), indent=1), encoding='utf-8')
