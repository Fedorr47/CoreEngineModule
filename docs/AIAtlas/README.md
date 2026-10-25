# CoreEngine AI Atlas

Русскоязычный интерактивный атлас AI-подсистемы CoreEngine: GOAP, решения и исполнение, наблюдения и события,
маршруты, RouteFollower, steering, FollowTarget/FleeTarget, obstacle avoidance, traversal, NavMesh и связи с
Gameplay/Physics. Описан снимок `Deferred` @ `8c6c0dc` (10.09.2026).

## Открыть

`docs/AIAtlas/site/index.html` — открывается из файловой системы (`file://`) без сервера и сети: поиск, схемы,
формулы (KaTeX отрисован при сборке), viewer и демо работают офлайн.

## Структура

```text
docs/AIAtlas/
  mkdocs.yml                 конфигурация MkDocs Material (strict, offline, search ru+en)
  hooks/atlas_hooks.py       директивы [[REF]], @@code, @@diagram, [[cls:]], значки, KaTeX при сборке
  docs/
    index.md                 главная и маршруты чтения
    chapters/NN-*.md         21 глава (00–20)
    demos/                   учебные модели (GOAP-поиск, Arrival/Flee)
    reference/               runtime-справочник, индекс и карточки сущностей, реестр источников, схемы, словарь
    assets/                  CSS, JS (viewer, reveal/фильтры, демо), KaTeX, SVG-схемы
    data/                    машинные данные (индекс, реестр, метаданные схем, отчёт проверки)
  diagrams/mermaid/*.mmd     редактируемые исходники схем (заголовки %% title/kind/level/refs/status)
  tools/                     сборка, генераторы, проверки, модели (см. ниже)
  audit/                     аудит, сверка с PDF, покрытие, прогресс, результаты проверок, скриншоты
  site/                      собранный сайт
  .toolchain/                изолированный toolchain (в .gitignore)
```

## Toolchain (один раз)

Из `docs/AIAtlas/`:

```text
python -m venv .toolchain/venv
.toolchain/venv/Scripts/python.exe -m pip install "mkdocs-material==9.6.*" pillow
mkdir .toolchain/node && cd .toolchain/node && npm init -y
PUPPETEER_SKIP_DOWNLOAD=1 npm install @mermaid-js/mermaid-cli@11 katex@0.16 iframe-worker@1
```

`.toolchain/puppeteer-config.json`: `{"executablePath": "<путь к chrome.exe>", "args": ["--no-sandbox"], "headless": "new"}`.
Проверенные версии: mkdocs 1.6.1, mkdocs-material 9.6.23, pymdown-extensions 10.21.3, mermaid-cli 11.17.0,
katex 0.16.47, iframe-worker 1.0.4, Node 24.19, Python 3.12. Java/PlantUML не требуются.

## Сборка и проверки (из корня репозитория)

| Действие | Команда |
|---|---|
| полная сборка + проверки | `python -I -B docs/AIAtlas/tools/build_site.py` (`--force` — перерисовать схемы, `--serve` — dev-сервер) |
| браузерная проверка | `node docs/AIAtlas/tools/qa_browser.js --widths 390,1280,1920,3440 --zoom 1,1.25,2 --out audit/screenshots` |
| дрейф исходников | `python -I -B docs/AIAtlas/tools/check_drift.py` |
| численные примеры | `python -I -B docs/AIAtlas/tools/verify_examples.py` |

## Обновить source references

1. `check_drift.py` — список изменившихся якорей и файлов.
2. Перечитать связанные главы (поиск `[[ID]]`), поправить текст.
3. В `tools/references.py` обновить ожидаемый текст или n-е вхождение; новые утверждения — новые записи
   `(ID, путь, уникальный текст строки, nth, описание)`. Номера строк руками не писать.
4. `build_site.py`.

## Добавить главу или схему

- Глава: шаблон skill `coreengine-theory-to-code` (разделы: Задача и интуиция → Теоретическая выкладка →
  Визуальное объяснение → Как это реализовано в CoreEngine → Разобранный пример → Ошибки/границы), строка в `nav`
  и в таблице `docs/index.md`. Директивы: `[[ID]]`, `@@code ID N`, `@@diagram id | подпись`, `[[cls:Name]]`,
  `[[SV]] [[EV]] [[INF]] [[HIST]] [[PROP]] [[NV]]`.
- Схема Mermaid: `diagrams/mermaid/<id>.mmd` с заголовком `%% title/kind/level/refs` (`%% status: PROPOSED` — только
  глава 20). Вычисленная геометрия: функция с `@figure` в `tools/gen_geometry.py`.
- Карточка сущности: ручной разбор — `tools/class_notes.py`; автоматическая часть строится из исходников.

## Инструменты

`references.py` (реестр), `build_class_index.py`, `class_notes.py`, `gen_reference_pages.py`, `render_diagrams.py`,
`gen_geometry.py`, `atlas_models.py` и `goap_model.py` (учебные модели), `gen_demo_data.py`, `verify_examples.py`,
`validate_site.py`, `check_drift.py`, `qa_browser.js`, `katex_render.js`, `build_site.py`.

Связанные skills: `.claude/skills/coreengine-{source-audit,theory-to-code,interactive-atlas,atlas-qa}`.
