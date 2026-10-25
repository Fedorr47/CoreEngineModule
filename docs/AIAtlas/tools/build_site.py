"""Одна команда сборки AI-атласа CoreEngine.

  python -I -B tools/build_site.py              данные → схемы → reference → strict build → проверка → повторная сборка
  python -I -B tools/build_site.py --force      перерисовать все схемы
  python -I -B tools/build_site.py --serve      после сборки — dev-сервер http://127.0.0.1:8000 (не обязателен)

Toolchain (изолирован, в .gitignore): docs/AIAtlas/.toolchain/venv (MkDocs Material),
docs/AIAtlas/.toolchain/node (Mermaid CLI, KaTeX), puppeteer использует системный Chrome.
Исходники движка только читаются. Результат: docs/AIAtlas/site/index.html (открывается из file://).
"""
import subprocess
import sys
from pathlib import Path

ATLAS = Path(__file__).resolve().parents[1]
TOOLS = ATLAS / 'tools'
VENV = ATLAS / '.toolchain' / 'venv' / 'Scripts'
MKDOCS = [str(VENV / 'python.exe'), '-m', 'mkdocs']


def step(name, command):
    print(f'== {name}', flush=True)
    result = subprocess.run(command, cwd=ATLAS)
    if result.returncode:
        raise SystemExit(f'FAILED: {name} (exit {result.returncode})')


def main():
    python = [sys.executable, '-I', '-B', '-X', 'utf8']
    step('Реестр источников (резолв якорей)', python + [str(TOOLS / 'references.py')])
    step('Индекс сущностей', python + [str(TOOLS / 'build_class_index.py')])
    render = python + [str(TOOLS / 'render_diagrams.py')] + (['--force'] if '--force' in sys.argv else [])
    step('Схемы (Mermaid + вычисленная геометрия)', render)
    step('Данные учебных демонстраций', python + [str(TOOLS / 'gen_demo_data.py')])
    step('Reference-страницы', python + [str(TOOLS / 'gen_reference_pages.py')])
    step('Численные контрольные примеры', python + [str(TOOLS / 'verify_examples.py')])
    step('Strict MkDocs build', MKDOCS + ['build', '--strict', '--clean'])
    step('Проверка сайта', python + [str(TOOLS / 'validate_site.py')])
    step('Повторная сборка с отчётом проверки', MKDOCS + ['build', '--strict', '--clean'])
    step('Проверка пересобранного сайта', python + [str(TOOLS / 'validate_site.py'), '--final'])
    print(f'Сайт: {ATLAS / "site" / "index.html"}')
    if '--serve' in sys.argv:
        subprocess.run(MKDOCS + ['serve', '--dev-addr', '127.0.0.1:8000'], cwd=ATLAS)


if __name__ == '__main__':
    main()
