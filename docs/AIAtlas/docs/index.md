# CoreEngine AI Atlas

**GOAP, Steering и связанные контракты исполнения** — русскоязычный учебник и справочник по AI-подсистеме
CoreEngine. Каждая тематическая глава начинается с теории (интуиция, формулы или модель данных), затем
возвращается к реальным классам, методам, данным и местам вызова, разбирает пример и перечисляет границы.

!!! info "Снимок и доверие"
    Описан код ветки `Deferred` @ `8c6c0dc` (10.09.2026). Каждое утверждение о реализации ссылается на
    [реестр источников](reference/source-references.md), где номер строки вычислен при сборке из рабочего дерева.
    Значки: <span class="ev ev-source">SOURCE_VERIFIED</span> прочитано в коде ·
    <span class="ev ev-evidence">EVIDENCE_VERIFIED</span> подтверждено запуском ·
    <span class="ev ev-inferred">INFERRED</span> вывод с указанным основанием ·
    <span class="ev ev-hist">HISTORICAL</span> прежний снимок ·
    <span class="ev ev-proposed">PROPOSED</span> предложение ·
    <span class="ev ev-notcov">NOT_VERIFIED</span> не проверено. Подробно — [глава 00](chapters/00-how-to-read.md).

## Маршруты чтения

| Цель | Маршрут |
|---|---|
| Понять систему с нуля | 00 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 → 11 → 12 → 13 → 14 → 15 → 16 → 17 → 18 → 19 → 20 |
| Написать новое поведение из ассетов | 01 → 03 → 04 → 06 → 07 → [18](chapters/18-authoring-scenarios.md) → 19 |
| Разобраться, почему агент не идёт | [19](chapters/19-diagnostics.md) → 10 → 12 → 13 → 15 → 16 |
| Понять движение и steering | 11 → 12 → [13](chapters/13-steering.md) → 14 → 15 → 16 |
| Быстро найти контракт | [Runtime: быстрый справочник](reference/runtime-quick-reference.md) → [индекс сущностей](reference/class-index.md) |

## Главы

| № | Глава | О чём |
|---|---|---|
| 00 | [Как читать атлас и чему доверять](chapters/00-how-to-read.md) | scope, ревизия, статусы, источники |
| 01 | [Полная архитектура AI](chapters/01-architecture.md) | decision → execution → route → steering → physics → observation |
| 02 | [GOAP и STRIPS](chapters/02-goap-strips.md) | символическое планирование, прогноз против мира |
| 03 | [Модель состояния](chapters/03-state-model.md) | bool/int факты, идентичность, хеш |
| 04 | [Цели и контракты действий](chapters/04-goals-actions.md) | выбор цели, preconditions, continuation, стоимость |
| 05 | [Планировщик](chapters/05-planner.md) | uniform-cost search, дубликаты, оптимальность, пример Access Key |
| 06 | [Ассеты и композиция](chapters/06-assets-composition.md) | catalog, behavior, bindings, template, instance |
| 07 | [Наблюдения и события](chapters/07-observations-events.md) | writers, порядок зависимостей, подтверждение событий |
| 08 | [Decision runtime](chapters/08-decision-runtime.md) | monitored loop, replanning, Failed против Cancel |
| 09 | [Исполнение и lifetime](chapters/09-execution-lifetime.md) | bridge, binding, AISystem, владение |
| 10 | [Порядок кадра](chapters/10-frame-order.md) | BeginFrame, решения, ticks, физика, задержки |
| 11 | [Пространственные маршруты](chapters/11-spatial-routes.md) | авторский граф, Дейкстра, NavMesh, route cost |
| 12 | [RouteFollower](chapters/12-route-follower.md) | сегменты, look-ahead, плоскость перехода, прибытие |
| 13 | [Steering](chapters/13-steering.md) | Seek, Flee, Arrival, команда |
| 14 | [FollowTarget и FleeTarget](chapters/14-temporal-behaviors.md) | выборка, переиздание команды, гистерезис |
| 15 | [Obstacle avoidance](chapters/15-obstacle-avoidance.md) | feelers, escape, опора, память стороны |
| 16 | [Traversal и физика](chapters/16-traversal-physics.md) | передача управления, Jump, Door, physicallyBlocked |
| 17 | [Ресурсы и несколько агентов](chapters/17-resources-multiagent.md) | ledger, покупка, брони, конкуренция |
| 18 | [Авторинг и сценарии](chapters/18-authoring-scenarios.md) | Marker Visit, Target Recovery, Maze, Playground |
| 19 | [Диагностика и проверка](chapters/19-diagnostics.md) | debug views, тесты, матрица симптомов |
| 20 | [Ограничения и развитие](chapters/20-limits-future.md) | проверенные границы; предложения отдельно |

## Справочник и инструменты

- [Runtime: быстрый справочник](reference/runtime-quick-reference.md) — типы, методы, триггеры, схемы исполнения.
- [Индекс сущностей](reference/class-index.md) с фильтрами по домену и виду; карточки классов и функций.
- [Реестр источников](reference/source-references.md) · [каталог схем](reference/diagrams.md) ·
  [словарь](reference/glossary.md) · [внешние источники](reference/external-sources.md).
- Учебные модели: [пошаговый GOAP-поиск](demos/goap-search.md) · [Arrival и гистерезис Flee](demos/arrival-flee.md).
- [Сборка и обновление атласа](reference/build.md) · [отчёт автоматических проверок](reference/validation-report.md).

Поиск (клавиша `/` или значок лупы) работает офлайн по русскому тексту и C++-идентификаторам; переход из поиска
раскрывает свёрнутые блоки с найденным словом. Схемы масштабируются кнопками над ними, `Ctrl` + колесо,
клавишами `+`/`−`/`F`/`R`, раскрываются в слой поверх страницы (`Esc` — закрыть).
