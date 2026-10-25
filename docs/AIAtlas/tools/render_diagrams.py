"""Локальный рендер схем атласа в SVG (без облачных сервисов).

  diagrams/mermaid/<id>.mmd  → docs/assets/diagrams/<id>.svg  (Mermaid CLI из .toolchain/node, системный Chrome)
  tools/gen_geometry.py      → docs/assets/diagrams/g*.svg    (вычисленные геометрические построения и графики)

Заголовок .mmd — строки-комментарии «%% ключ: значение» (title, kind, level, refs, status). Они попадают в
docs/data/diagram-meta.json, из которого hook строит подпись, а генератор — каталог схем.
По умолчанию перерисовываются только изменившиеся источники; --force — всё. Падает при ошибке рендера.
"""
import json
import subprocess
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
TOOLCHAIN = ATLAS / '.toolchain'
OUT = ATLAS / 'docs' / 'assets' / 'diagrams'
META = ATLAS / 'docs' / 'data' / 'diagram-meta.json'
MMDC = TOOLCHAIN / 'node' / 'node_modules' / '@mermaid-js' / 'mermaid-cli' / 'src' / 'cli.js'
CONFIG = ATLAS / 'diagrams' / 'mermaid-config.json'
PUPPETEER = TOOLCHAIN / 'puppeteer-config.json'


def header(path):
    meta = {}
    for line in path.read_text(encoding='utf-8').splitlines():
        if not line.startswith('%%'):
            if line.strip():
                break
            continue
        body = line[2:].strip()
        if ':' in body:
            key, value = body.split(':', 1)
            meta[key.strip()] = value.strip()
    if 'refs' in meta:
        meta['refs'] = [ref.strip() for ref in meta['refs'].split(',') if ref.strip()]
    return meta


def render_mermaid(force):
    report, failures = {}, []
    sources = sorted((ATLAS / 'diagrams' / 'mermaid').glob('*.mmd'))
    newest_config = CONFIG.stat().st_mtime
    for path in sources:
        meta = header(path)
        for key in ('title', 'kind', 'level'):
            if key not in meta:
                failures.append(f'{path.name}: header lacks "%% {key}:"')
        svg = OUT / f'{path.stem}.svg'
        meta.update(source=path.relative_to(ATLAS).as_posix(), renderer='mermaid')
        report[path.stem] = meta
        if not force and svg.is_file() and svg.stat().st_mtime > max(path.stat().st_mtime, newest_config):
            continue
        svg.unlink(missing_ok=True)
        command = ['node', str(MMDC), '-p', str(PUPPETEER), '-c', str(CONFIG), '-i', str(path), '-o', str(svg),
                   '-b', 'white', '--svgId', f'svg-{path.stem}', '-q']
        process = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace')
        if process.returncode or not svg.is_file() or svg.stat().st_size < 500:
            errors = [line for line in process.stderr.splitlines() if 'Error' in line or 'Expecting' in line]
            failures.append(f'{path.name}: ' + (' | '.join(errors[:3]) or process.stderr.strip()[-600:])[:900])
        else:
            print('rendered', path.name, flush=True)
    return report, failures


def main():
    force = '--force' in sys.argv
    OUT.mkdir(parents=True, exist_ok=True)
    report, failures = render_mermaid(force)
    geometry = subprocess.run([sys.executable, '-I', '-B', str(ATLAS / 'tools' / 'gen_geometry.py')],
                              capture_output=True, text=True, encoding='utf-8')
    if geometry.returncode:
        failures.append('gen_geometry.py: ' + geometry.stderr[-1500:])
    else:
        report.update(json.loads((ATLAS / 'docs' / 'data' / 'geometry-meta.json').read_text(encoding='utf-8')))
    META.write_text(json.dumps(report, indent=1, ensure_ascii=False), encoding='utf-8')
    print(len(report), 'diagrams;', len(failures), 'failures')
    for failure in failures:
        print('FAIL', failure)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
