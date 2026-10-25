# Прогресс AI-атласа CoreEngine

Файл позволяет продолжить работу без повторного аудита. Обновляется после каждого этапа.

## Snapshot

| Поле | Значение |
|---|---|
| Workspace | `D:\C++projects\CoreEngineModule` (`git rev-parse --show-toplevel` подтверждён) |
| Ветка | `Deferred` (не переключалась) |
| HEAD при старте сессии | `8b7390095dc10f0c5e6698dbbf9a8ae1779bef11` (30.08.2026) — отставал от `origin/Deferred` на 48 коммитов |
| HEAD после fast-forward пользователем | `8c6c0dc37ab369e201d1b3252448f96d1e81cd37` (= `origin/Deferred`, 10.09.2026) |
| Снимок старого PDF | `55de4ab55c48af98c022534e2bc8013f11f7bd70` (03.09.2026) — предок текущего HEAD |
| Дата начала | 2026-10-13 |
| Исходный `git status --short` | `?? ArchitectureAtlas(2).rar`, `?? CoreEngine_AI_Atlas_ClaudeCode_Prompt_RU.md`, `?? dlls/DebugUtils-d.dll`, `?? dlls/imguid.lib`, `?? docs/_reference/`, `?? docs/architecture/gameplay-component-ownership.md`, `?? patch.patch` — всё пользовательское, не трогать |

Решение пользователя (2026-10-13): источник истины — рабочее дерево после его собственного `git pull` до `origin/Deferred`.

## Входные материалы

| Материал | Статус |
|---|---|
| `docs/_reference/ai-atlas/ArchitectureAtlas(2).rar` | есть; распакован только во временный scratchpad сессии (вне репозитория) |
| `docs/_reference/ai-atlas/CoreEngine_AI_Documentation_RU.pdf` | есть; 45 стр., текст извлечён `pdftotext -layout` |
| `COREENGINE_DOCUMENTATION_RULES (1).md` | **отсутствует** в workspace (проверены `docs/_reference/ai-atlas/`, корень, `docs/`) |
| `CoreEngine_Development_Test_Scenario_Rules.txt` | **отсутствует** |
| `goap_development_roadmap.md` | **отсутствует** |
| Правила проекта | `CLAUDE.md`, `AGENTS.md` (добавлен в `bf03b19`), `tests/README.md`, `docs/architecture/*`, `docs/refactor/*` |
| Существующие skills | `.claude/` отсутствует — аналогов нет |

## Инструменты окружения

- Python 3.12.10 (`python`), pypdf 6.19, `pdftotext` (mingw64), Node 24.19 / npm 11.17, bsdtar 3.8.8.
- Java отсутствует → PlantUML не используется; диаграммы — Mermaid CLI (локально) + SVG, вычисляемые Python.
- Браузеры: Chrome, Edge (x86 path). MkDocs в системе не установлен → изолированный venv в `docs/AIAtlas/.toolchain/` (gitignored).

## Этапы

- [x] A0. Проверка workspace/ветки/HEAD/статуса, расхождение HEAD vs PDF решено пользователем
- [x] A1. Изучен образец (mkdocs.yml, tools/*, viewer, css, validate)
- [x] A2. Извлечён текст PDF
- [x] A3. Аудит исходников → `source-audit.md` (AI-код HEAD == снимок PDF)
- [x] A4. `reference-delta.md` (81 утверждение)
- [x] A5. План (ниже)
- [x] B. Skills + toolchain + главы-образцы (03 модель состояния, 05 планировщик, 13 steering)
- [x] C. Остальные главы (01, 02, 04, 06–12, 14–20)
- [x] D. Reference, runtime-справочник, демо, глоссарий
- [x] E. Проверки, README, coverage, validation, отчёт

## Принятые решения

- Atlas root: `docs/AIAtlas/` (в репозитории принят нижний регистр `docs/`).
- Toolchain: MkDocs + Material, изолированный venv `docs/AIAtlas/.toolchain/venv` (gitignored);
  Mermaid CLI через локальный `docs/AIAtlas/.toolchain/node` (puppeteer на системном Chrome, без скачивания Chromium);
  PlantUML не используется (нет Java); геометрические схемы — SVG, вычисляемые Python (`tools/gen_geometry.py`).
- Формулы: KaTeX рендерится **при сборке** (hook + node), CSS и шрифты KaTeX копируются локально → работает без JS.
- Source registry: `tools/references.py` (id, путь, ожидаемый текст, n-е вхождение) → номера строк вычисляются при
  сборке, нерезолвленный якорь ломает сборку; snippet-выдержки проверяются маркером `<!-- expect: ... -->`.
- Viewer/CSS адаптированы из образца (переведены подписи, тёмная тема), без UE-контента.
- Поиск: встроенный search Material + `lang: [ru, en]`; доп. JS раскрывает `<details>` с найденным/целевым фрагментом.
- Учебные демо: (1) пошаговый GOAP-поиск на реальном Access Key (с route-costs) и мини-домене; (2) профиль Arrival
  + гистерезис Flee. Встраиваются как inline JS без fetch.

## План глав (файлы `docs/chapters/`)

00 how-to-read · 01 architecture · 02 goap-strips · 03 state-model · 04 goals-actions · 05 planner ·
06 assets-composition · 07 observations-events · 08 decision-runtime · 09 execution-lifetime · 10 frame-order ·
11 spatial-routes · 12 route-follower · 13 steering · 14 temporal-behaviors · 15 obstacle-avoidance ·
16 traversal-physics · 17 resources-multiagent · 18 authoring-scenarios · 19 diagnostics · 20 limits-future.
Reference: class-index (+карточки), source-references, runtime-quick-reference, diagrams, glossary, demos,
external-sources, build, validation-report.

## Выполнено на этапе B

- Toolchain: venv (mkdocs 1.6.1, mkdocs-material 9.6.23, pymdown-extensions 10.21.3, Pillow для QA),
  node (`@mermaid-js/mermaid-cli` 11.17.0, `katex` 0.16.47, `iframe-worker` 1.0.4), puppeteer → системный Chrome.
- Инструменты: `tools/references.py` (реестр), `build_class_index.py`, `render_diagrams.py`, `gen_geometry.py`,
  `atlas_models.py`, `goap_model.py`, `gen_demo_data.py`, `gen_reference_pages.py`, `verify_examples.py`,
  `validate_site.py`, `check_drift.py`, `build_site.py`, `qa_browser.js`, `katex_render.js`, hook `hooks/atlas_hooks.py`.
- Пилотные главы 00, 03, 05, 13 собраны и проверены в браузере (file://, сеть заблокирована): поиск ru/C++,
  viewer, тема, overflow — OK.
- Найден и устранён CDN-дефект offline-плагина Material (iframe-worker с unpkg) — локальная копия.
- Skills: `.claude/skills/coreengine-{source-audit,theory-to-code,interactive-atlas,atlas-qa}/SKILL.md` + references.

## Выполнено на этапах C–E

- Главы 01–20 по шаблону (6 разделов), 42 Mermaid + 9 вычисленных SVG, 2 демо, справочник, карточки сущностей,
  реестр источников, словарь, внешние источники, страница сборки.
- Проверки: strict build, `validate_site` 0 ошибок, `verify_examples` 77/77, браузерная QA без сетевых запросов и
  JS-ошибок (детали — `validation.md`, покрытие — `coverage.md`).
- Тесты движка как доказательства: `CoreEngineModuleTests` не собирается на HEAD (ошибки в тестах),
  `CoreEngineAIPhysicsSmokeTests` 1/3 PASS. Тесты и исходники не изменялись; пересобраны только объекты в `out/build`.

## Активная задача

Нет — атлас завершён. Коммит не выполнялся (ждёт отдельного запроса пользователя).
При изменении кода: `check_drift.py` → правка глав/реестра → `build_site.py` → `qa_browser.js`.
