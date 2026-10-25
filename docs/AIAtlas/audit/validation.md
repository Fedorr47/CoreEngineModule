# Результаты проверок

Снимок: `Deferred` @ `8c6c0dc37ab369e201d1b3252448f96d1e81cd37`, 13.10.2026. Статусы: PASS / FAIL / NOT_RUN.
Машинные результаты — `audit/generated/`, скриншоты — `audit/screenshots/`.

## Атлас

| Проверка | Команда / артефакт | Результат |
|---|---|---|
| Strict-сборка MkDocs (битые ссылки, nav, якоря) | `build_site.py` → `mkdocs build --strict` | PASS |
| Резолв реестра источников (~260 якорей, неоднозначность, SHA-256) | `build_site.py` / `check_drift.py` | PASS, дрейфа нет |
| Структура глав (6 разделов, объём теории, схема, пример, `[[ID]]`, `[[cls:]]`, символы ⊂ `src/`) | `validate_site.py` | PASS, 0 ошибок |
| Численные примеры (план Access Key 42.0854 / 22 раскрытия, реплан 40.0852, Arrival, гистерезис Flee, геометрия) | `verify_examples.py` → `numeric-checks.json` | PASS, 77/77 |
| Офлайн `file://`, сеть заблокирована: внешние запросы | `qa_browser.js` → `browser-qa-main.json` | PASS, 0 запросов |
| JS-ошибки и ошибки консоли | то же | PASS, 0 |
| Поиск ru и C++ (`планировщик`, `гистерезис`, `FindAIPlan`, `Decision`, `physicallyBlocked`) + раскрытие `<details>` | то же | PASS |
| Viewer: zoom/pan/fit/reset/expand, Esc, возврат фокуса (7 страниц) | то же | PASS |
| Светлая/тёмная тема | то же | PASS |
| Фильтры индекса сущностей (256 → 14) | то же | PASS |
| Демо GOAP-поиска (42.0854, 22 раскрытия) и Arrival/Flee (0.50) | то же | PASS |
| Ширины 390/1280/1920/3440, 12 страниц: переполнение, перекрытия | `browser-qa-main.json` | PASS |
| Все 21 глава на 390 px | `browser-qa-all-chapters-390.json` | PASS |
| Zoom 1.25 и 2 (4 страницы) | `browser-qa-zoom.json` | PASS |
| `git diff --check` (новые файлы атласа и skills) | см. итоговый отчёт | PASS |

## Движок (только как источник доказательств; тесты не изменялись)

Окружение: MSVC 14.44.35207 (VS2022, `-vcvars_ver=14.44`), CMake 4.4.2, Ninja, `out/build/x64-Debug-DX12`.

| Проверка | Результат | Детали |
|---|---|---|
| Сборка `CoreEngineModuleTests` | FAIL | ошибки компиляции в тестах на HEAD: `TestRenderSceneExtractor.cpp` (нет `Scene::AddMaterial`), `TestGameplayGOAPResourceComponents.cpp:264` (нет `Vec3::operator+=`) — `engine-tests/build-CoreEngineModuleTests-errors.txt` |
| `CoreEngineAIPhysicsSmokeTests` | FAIL (1 PASS / 2 FAIL) | PASS `FreeMovementUsesPhysicsCharacter`; FAIL `WallBlockProducesPhysicalBlockedFeedback`, `WallSlidePreservesTangentialMovement` — причина не исследовалась (NV); `engine-tests/gtest-ai-physics-smoke*.{json,txt}` |
| `tests/tools/check_goap_asset_composition.py` | PASS | |
| `run_goap_asset_portable_checks.py` | NOT_RUN | нет GCC |
| Ручные сценарии `app` (DevelopmentScenario) | NOT_RUN | |

## Skills

| Skill | Проверка | Результат |
|---|---|---|
| `coreengine-source-audit`, `coreengine-theory-to-code`, `coreengine-interactive-atlas`, `coreengine-atlas-qa` | структура (frontmatter, триггеры, ссылки на references), применены в этой сессии к главам 01–20 | PASS (структурная/ручная) |
| Pressure-сценарии на изолированном агенте (с skill / без skill) | — | NOT_RUN |
