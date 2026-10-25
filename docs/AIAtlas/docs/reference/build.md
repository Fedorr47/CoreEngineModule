# Сборка и обновление атласа

Полная инструкция — `docs/AIAtlas/README.md`. Кратко:

| Действие | Команда (из корня репозитория) |
|---|---|
| Открыть готовый атлас | `docs/AIAtlas/site/index.html` в браузере (без сервера и сети) |
| Полная сборка и проверки | `python -I -B docs/AIAtlas/tools/build_site.py` |
| Перерисовать все схемы | `python -I -B docs/AIAtlas/tools/build_site.py --force` |
| Браузерная проверка (file://, сеть заблокирована) | `node docs/AIAtlas/tools/qa_browser.js --widths 390,1280,1920,3440 --zoom 1,1.25,2` |
| Дрейф исходников с последней сборки | `python -I -B docs/AIAtlas/tools/check_drift.py` |
| Численные примеры | `python -I -B docs/AIAtlas/tools/verify_examples.py` |
| Только реестр | `python -I -B docs/AIAtlas/tools/references.py` |

Конвейер `build_site.py`: реестр источников → индекс сущностей → схемы (Mermaid + вычисленная геометрия) → данные
демо → reference-страницы → численные проверки → strict MkDocs → проверка сайта → повторная сборка с отчётом.

Toolchain изолирован в `docs/AIAtlas/.toolchain/` (в `.gitignore`): Python venv с `mkdocs-material 9.6.23`,
`mkdocs 1.6.1`, `pymdown-extensions 10.21.3`, `Pillow` (QA); Node-пакеты `@mermaid-js/mermaid-cli 11.17.0`,
`katex 0.16.47`, `iframe-worker 1.0.4` (копия shim лежит в `docs/assets/js/vendor/`); puppeteer использует системный
Chrome.
