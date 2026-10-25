# Сверка старого PDF с текущим кодом

Исторический документ: `docs/_reference/ai-atlas/CoreEngine_AI_Documentation_RU.pdf` (редакция 08.09.2026, снимок
`55de4ab`). Текущий снимок: `8c6c0dc` (10.09.2026). AI-исходники и ассеты между снимками не менялись
(`git diff 55de4ab HEAD` по `src/Gameplay`, `assets/ai`, `assets/development`, `src/Navigation`,
`src/App/Development`, `src/App/LevelNavigation.cppm`, `src/App/GameplayPhysics*` пуст); выделен
`src/App/AppNavigationCoordinator.cpp`.

Статусы: `CONFIRMED` — подтверждено кодом; `CHANGED` — утверждение неточно или требует уточнения (в новом атласе
исправлено); `REMOVED` — механизм отсутствует; `NOT_VERIFIED` — проверить невозможно без запуска/данных.
Разделы атласа — номера глав (`chapters/NN-*.md`).

| # | Утверждение PDF (раздел) | Результат | Текущий источник (символ/якорь) | Глава |
|---|---|---|---|---|
| 1 | Классы `GameplayAIAccessKeyDecision`, `GameplayAIAccessKeyObservation`, модуль `GameplayAIDecisionComposition` удалены (§1) | CONFIRMED | grep по `src/`, `tests/` — совпадений нет | 06 |
| 2 | `AIAgentWorldState`: до 128 bool и 128 int32, независимые пространства ID, bitset + массив (§2) | CONFIRMED | `FactCapacity = 128u`, `IntegerFactCapacity = 128u` | 03 |
| 3 | Нет значения «неизвестно» (§2) | CONFIRMED | `AIAgentWorldState` хранит только значения | 03 |
| 4 | ID фактов/целей/контекстов назначаются по порядку объявления (§2) | CONFIRMED | `CompileGameplayGOAPDefinition` (`booleanIndex++`, `AIGoalId{index}`, `nextContext`) | 03, 06 |
| 5 | Равенство и хеширование по полному состоянию, FNV-1a (§2) | CONFIRMED | `AIAgentWorldStateHash` | 03, 05 |
| 6 | Цель — требуемые булевы значения; выбор цели «использует базовую оценку и правила, зависящие от булевых фактов» (§3) | CHANGED | контракт `AIGoalScoreRule` существует, но ассетный компилятор создаёт только `baseScore`; правила недоступны из JSON; при равных score побеждает первая цель (`score > selectedGoal.score`) | 04 |
| 7 | Нет автоматического перебора менее приоритетных целей при отсутствии плана (§3) | CONFIRMED | `AIDecisionRuntime::Update` | 04, 08 |
| 8 | Для покупки семантическое имя `purchase`, контекст `buy_key` (§3) | CONFIRMED | `kAIPurchaseActionId{3u}`, `access_key.goap.json` | 17 |
| 9 | Каждое continuation condition обязано присутствовать среди preconditions (§3) | CONFIRMED (уточнение) | проверяется в `IsInputValid` планировщика, а не при компиляции: нарушение → любой поиск `nullopt`, решение Failed, регистрация проходит | 04, 05 |
| 10 | В поиске continuation не образует отдельный фильтр (§3) | CONFIRMED | `FindAIPlan` использует только preconditions | 04 |
| 11 | Target Recovery: `goalAvailable` в preconditions и continuation (§3) | CONFIRMED | `target_recovery.goap.json` | 08, 18 |
| 12 | `FindAIPlan` — uniform-cost search, без эвристики A* (§4) | CONFIRMED | `FindAIPlan`, `SearchNodeGreater` | 05 |
| 13 | Очередь по стоимости, затем порядок вставки; действия по `(actionId, contextId)`; lazy-deletion дубликатов (§4) | CONFIRMED | `orderedActions` sort, `bestCosts`, `current.cost != currentBest->second` | 05 |
| 14 | Пример стоимостей 13 против 18 (§4) | CONFIRMED как учебный | условные числа PDF; в атласе добавлен реальный пример Access Key (42.0854) | 05 |
| 15 | `routeCost: true` заменяет стоимость контекста ценой пути в копии определения (§4) | CONFIRMED | `MoveToCapability` → `costs_`, `GameplayGOAPDecisionTemplate::InstanceDefinition` | 06, 11 |
| 16 | Пустой план при уже выполненной цели; отсутствие плана ≠ Cancel (§4) | CONFIRMED | `FindAIPlan`, `AIDecisionRuntime` | 05, 08 |
| 17 | Узел хранит копию состояния и вектора шагов; нет лимита раскрытий и backoff (§4) | CONFIRMED | `SearchNode`, `UpdateActiveAIDecisions_` | 05, 20 |
| 18 | Каталог → behavior → definition/routes/bindings; виды файлов (§5) | CONFIRMED | `GameplayAIDecisionAsset.cppm` | 06 |
| 19 | Registry с parser+compiler, `std::any`, `Parameters<T>` (§5) | CONFIRMED | `GameplayGOAPCompositionRegistry`, `GameplayAIComponentParsers` | 06 |
| 20 | Registry отвергает конфликтующую регистрацию; capability — уникальный action ID (§5) | CONFIRMED | `RegisterCapability` | 06 |
| 21 | Поля встроенных компонентов проверяются parser (§5) | CONFIRMED (уточнение) | `Fields(...)` запрещает неизвестные поля в behavior/bindings/catalog/routes и компонентах; `.goap.json` неизвестные поля не проверяет | 06 |
| 22 | Template проверяет модель, типы, routeGraph, уникальность и покрытие контекстов (§5) | CONFIRMED | конструктор `GameplayGOAPDecisionTemplate` | 06 |
| 23 | Транзакционная регистрация во временной копии registry (§5) | CONFIRMED | `RegisterGameplayAIDecisionAssets` (`auto pending = destination`) | 06 |
| 24 | Нет hot reload; instances держат свою конфигурацию (§5) | CONFIRMED | `shared_ptr<const GameplayGOAPDecisionTemplate>` | 06 |
| 25 | Разрешение ролей: однозначный node, конечная позиция, ≤1 entity; роль без entity допустима (§5) | CONFIRMED | `ResolveRoles` | 06 |
| 26 | Cost overrides: конечные, неотрицательные, без повторов и неизвестных пар (§5) | CONFIRMED | `InstanceDefinition` | 06 |
| 27 | Роль — снимок позиции и handle; нет перепривязки (§5) | CONFIRMED | `GameplayAIResolvedRole` | 06, 14 |
| 28 | Ровно один writer на факт, топологический порядок «первый готовый» (§6) | CONFIRMED | `OrderGameplayGOAPObservations` | 07 |
| 29 | Встроенные наблюдения: таблица особенностей (§6) | CONFIRMED | `GameplayGOAPSpatialComponents.cppm`, `GameplayGOAPResourceComponents.cppm` | 07 |
| 30 | `nearest_location` при равенстве выбирает более поздний элемент (§6) | CONFIRMED | `candidate <= distance` | 07 |
| 31 | Ledger: только свой агент и предмет, без повторного начисления, широкий промежуточный тип (§6) | CONFIRMED | `ResourceLedgerObservation::Observe` (`std::int64_t`) | 17 |
| 32 | `PurchaseRuntime::Start`: проверки и `ResourcePurchased`; факты не пишет (§6) | CONFIRMED | `PurchaseRuntime::Start` | 17 |
| 33 | Capability связывает модель покупки с исполнением (§6) | CONFIRMED | `PurchaseCapability` | 17 |
| 34 | `hide_on_purchase` один раз на instance (§6) | CONFIRMED | `HideOnPurchaseReaction::emitted_` | 17 |
| 35 | Два прохода событий; `ObserveActionEvents` по умолчанию пуст; `pickup_available` не переопределяет (§6) | CONFIRMED | `IGameplayGOAPObservation`, `PickupAvailabilityObservation` | 07 |
| 36 | Lifecycle decision update, invalidation, план «Ready» на следующем update (§7) | CONFIRMED | `AIDecisionRuntime::Update`, `InstallPlan_` | 08 |
| 37 | При смене цели continuation старого шага не перепроверяется (§7) | CONFIRMED | `bPlanInvalidated` требует `!bGoalChanged` | 08 |
| 38 | Explicit Cancel блокирует автопродолжение; `Plan(...)` снимает флаг; `CancelAIDecision(entity)` удаляет instance (§7) | CONFIRMED | `AIDecisionRuntime::Cancel`, `Plan`, `GameplayRuntime::CancelAIDecision` | 08 |
| 39 | Target Recovery: радиус 0.4, slowing 1.5, недоступность ~на половине пути на 5 с (§7) | CONFIRMED | `target_recovery.behavior.json`, `ai_goap_target_recovery.scenario.json` (`distanceFraction` 0.5 по 3D-квадрату расстояния до цели, таймер — реальное время `steady_clock`) | 08, 18 |
| 40 | `StartAIDecision` синхронный первый update; неуспех не сохраняет instance (§7) | CONFIRMED | `GameplayRuntime::StartAIDecision` | 08 |
| 41 | Решения обновляются по возрастанию entity handle; terminal-статус не удаляет запись (§7) | CONFIRMED | `UpdateActiveAIDecisions_` | 08, 10 |
| 42 | Плановая цепочка: execution → bridge → binding → runtime (§8) | CONFIRMED | `AIPlanExecutionBridge::StartReadyPlanStep` | 09 |
| 43 | Порядок полей instance обеспечивает уничтожение GOAP до контекста; копирование/перемещение запрещены (§8) | CONFIRMED | `GameplayGOAPDecisionInstance` | 09 |
| 44 | Порядок кадра (рис. 5 и текст) (§8) | CONFIRMED (дополнено) | `AppLifecycle.cpp`, `PrePhysicsUpdate`; в атласе добавлены: Start+Tick в одном кадре, пауза в один кадр без команды между шагами плана | 10 |
| 45 | Порядок внутри decision update (§8) | CONFIRMED | `GameplayGOAPDecisionInstance::Update` | 07, 10 |
| 46 | Follow/Flee переиздают кешированную команду каждый tick (§8, §12) | CONFIRMED | `ApplyCurrentMovement_` | 14 |
| 47 | `GameplayRoute`: N точек, N−1 аннотаций; follower не добавляет сегмент к anchor (§9) | CONFIRMED | `GameplayRoute::IsValid`, комментарий `GameplayRouteFollower::Start` | 11, 12 |
| 48 | Ребро без cost — евклидово расстояние (§9) | CONFIRMED (уточнение) | 3D-длина `mathUtils::Length(to − from)` между позициями ролей | 11 |
| 49 | Без route graph — граф из двух узлов и ребра 1; `routeCost` запрещён; нет NavMesh (§9) | CONFIRMED | `MoveToCapability` | 11 |
| 50 | `sourceRadius` (§9) | CONFIRMED (уточнение) | ближайший узел по 3D, сравнение `<`, при равенстве — первый узел графа | 11 |
| 51 | Reservation цели в MoveTo (§9) | CONFIRMED (дополнено) | `ReservedAIMoveToActionRuntime`; `Start` отклоняет и собственную существующую бронь | 17 |
| 52 | `pickup_available` учитывает reservations (§9) | CONFIRMED (дополнено) | при этом `GameplayPickupSystem` reservations не учитывает | 17 |
| 53 | Maze: прямой `StartAIFollowRoute`, Detour corridor/straight path, NoPath/BufferTooSmall (§9) | CONFIRMED | `World::FindPath`, `StartNavigationPathOperation` | 11 |
| 54 | Параметры Maze (§9) | CONFIRMED | `navigation_pathfinding_maze.scenario.json` | 11, 18 |
| 55 | RouteFollower: slowing = acceptance на промежуточных, look-ahead 0.75, ограничения (§10) | CONFIRMED | `GameplayRouteFollower::Tick`, `TryBuildLookAheadTarget_` | 12 |
| 56 | Пример look-ahead L=0.75, d=0.25 → +0.5 (§10) | CONFIRMED | пересчитано | 12 |
| 57 | Плоскость перехода по сумме направлений; пример (−0.2, 0.3) (§10) | CONFIRMED | `HasPassedCurrentCorner_`; пересчитано | 12 |
| 58 | Seek/Flee/Arrival формулы, пример (3,0,4) (§11) | CONFIRMED | `GameplaySteering.cppm`; пересчитано | 13 |
| 59 | Пример Arrival r=0.5, R=2.5, m_min=0.1 (§11) | CONFIRMED как учебный | пересчитано: d=1.5 → 0.5; d=0.6 → 0.1 | 13 |
| 60 | `ApplyGameplayMovementIntent` (§11) | CONFIRMED | `ApplyGameplayMovementIntent` | 13 |
| 61 | Observation 3D против steering XZ (§11) | CONFIRMED (дополнено) | в атласе показано числами для Marker Visit/Target Recovery, где r = R и цель на 0.2 выше: подтверждение не гарантировано (INFERRED, NOT_VERIFIED в приложении) | 13, 19 |
| 62 | Flee hysteresis, defaults 2/3/0.05, Playground 5/7 (§12) | CONFIRMED | `AIFleeTargetSettings`, `ai_steering_playground.scenario.json` | 14 |
| 63 | Avoidance: адаптер, origin на половине высоты, фактическая скорость, склон (§13) | CONFIRMED | `ApplyGameplayAgentObstacleAvoidance` | 15 |
| 64 | Три feelers, defaults 30°/2/3, τ=0.75, margin 0.02, пример v=6 → 4.5 (§13) | CONFIRMED | `GameplayObstacleAvoidanceSettings`, `Sanitize` | 15 |
| 65 | Escape = 0.6/0.4/0.3, ориентация касательной (§13) | CONFIRMED (уточнение) | касательная ориентируется по стабильной оси `rightAxis = (−f.z, 0, f.x)`, а не по feeler | 15 |
| 66 | Support по итоговому escape, длина 0.60, порог cos 45° (§13) | CONFIRMED | `HasSupport` | 15 |
| 67 | Выбор стороны и гистерезис 0.15; пример 1.00/1.10/1.20 (§13) | CONFIRMED | `ApplyGameplayObstacleAvoidance` | 15 |
| 68 | `ProbeSupport` по умолчанию — синтетическая плоская опора (§13) | CONFIRMED | `IGameplayObstacleQuery::ProbeSupport` | 15 |
| 69 | Avoidance локален (§13) | CONFIRMED (дополнено) | физический query по умолчанию видит только `StaticWorld` | 15, 20 |
| 70 | Traversal handoff; CompleteTraversal = «достигнут конец сегмента» (§14) | CHANGED | это контракт-комментарий; `DoorTraversalExecutor` фактически завершается у InteractionPoint двери, конец сегмента физически не посещается (INFERRED) | 16 |
| 71 | `physicallyBlocked` — диагностика без recovery (§14) | CONFIRMED (дополнено) | формула: опора, desired > 0.1, actual < 10 %, ≥ 0.25 с | 16 |
| 72 | GOAP MoveTo не получает obstacle query/debug; таблица входов (§14) | CONFIRMED | `MoveToCapability::CreateBinding`, `GameplayAIDecisionCreationContext` | 09, 16 |
| 73 | Marker Visit JSON (§15) | CONFIRMED | `assets/ai/*/marker_visit.*` | 18 |
| 74 | Таблица каталога/демонстраций (§15) | CONFIRMED | `catalog.json` (5 решений), `assets/development/*` | 18 |
| 75 | Lifecycle сценария setup/start/update/stop/reset (§15) | CONFIRMED | `DevelopmentScenarioRunner` | 18 |
| 76 | Debug: AIDebugViewModel, inspection, registries, флаги draw*, NavMesh overlay default-профиля (§16) | CONFIRMED | `AppDebugVisualization.cpp`, `AppNavigationCoordinator.cpp` | 19 |
| 77 | 623 успешные проверки `run_goap_asset_portable_checks.py` (§17) | NOT_VERIFIED (HISTORICAL) | результат относится к 08.09.2026; в этой работе не перезапускался (требуется GCC) | 19 |
| 78 | Полный набор GoogleTest не запускался (§17) | HISTORICAL | см. `audit/validation.md` о текущем запуске | 19 |
| 79 | Ограничения дизайна (таблица §17) | CONFIRMED | все пункты подтверждены кодом | 20 |
| 80 | Публичные точки входа (§18) | CONFIRMED | `GameplayRuntime.cppm` | Reference |
| 81 | Правила `COREENGINE_DOCUMENTATION_RULES (1).md` и `CoreEngine_Development_Test_Scenario_Rules.txt` учтены (§18) | NOT_VERIFIED | файлы отсутствуют в workspace | 00 |

## Новые утверждения атласа, которых не было в PDF

- Пауза в один кадр без команды между шагами плана (глава 10).
- `GameplayRuntime()` без аргументов — пустой каталог (глава 06).
- Pickup system не учитывает reservations (глава 17).
- Jump executor: фазы, допуски, тайм-аут 10 с, временная подмена `jumpVerticalSpeed` (глава 16).
- Реальный план Access Key с route-costs (глава 05, 11).
- Количественная проверка несогласованности arrival-контрактов в ассетах (глава 13, 19).
