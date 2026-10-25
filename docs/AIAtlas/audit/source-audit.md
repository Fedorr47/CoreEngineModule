# Аудит исходников AI CoreEngine

Снимок: ветка `Deferred`, HEAD `8c6c0dc37ab369e201d1b3252448f96d1e81cd37` (10.09.2026), рабочее дерево чистое по
всем прочитанным файлам (`git status` показывает только пользовательские untracked-файлы вне `src/` и `assets/`).
Дата аудита: 2026-10-13. Метод: построчное чтение объявлений, реализаций, мест вызова и регистраций; без запуска
движка. Статусы: `SOURCE_VERIFIED` (прочитано), `INFERRED` (вывод из нескольких прочитанных контрактов, основание
указано), `NOT_VERIFIED`.

Важно: `git diff 55de4ab HEAD -- src/Gameplay assets/ai assets/development src/Navigation src/App/Development
src/App/LevelNavigation.cppm src/App/GameplayPhysics*` пуст. AI-код текущего HEAD совпадает со снимком старого PDF;
после него только выделен `src/App/AppNavigationCoordinator.cpp` (коммит `b157d6f`) и поправлены include (`87a777d`).
Поэтому расхождения с PDF — это в основном неточности самого PDF, а не эволюция кода.

Якоря ниже — устойчивые фрагменты текста; номера строк вычисляет `tools/references.py` при сборке атласа.

## 1. Карта доменов

| Домен | Файлы | Ответственность |
|---|---|---|
| Ядро GOAP (домен-нейтральное) | `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm`, `AIDecisionContracts.cppm`, `AIStateOperations.cppm`, `AIPlanner.cppm`, `AIGoalSelection*.cppm` | факты, условия/эффекты, поиск плана, выбор цели |
| Исполнение плана | `GOAP/AIPlanExecution.cppm`, `AIPlanExecutionBridge.cppm`, `AIDecisionRuntime.cppm` | машина состояний плана, мост к `AISystem`, monitored loop/replanning |
| Action runtime | `src/Gameplay/AI/Runtime/AIActionContracts.cppm`, `AIActionRuntime.cppm`, `AIActionTask.cppm`, `AIActionBinding.cppm`, `AISystem.cppm` | интерфейс Start/Tick/Cancel, одна задача на агента, тик задач |
| Обёртка решения | `GOAP/GameplayGOAPDecision.cppm`, `GameplayGOAPDecisionInstance.cppm`, `GameplayGOAPDecisionSetup.cppm`, `Runtime/GameplayAIDecision*.cppm` | instance с фактами, наблюдением, реакциями, буферами событий; factory registry |
| Авторинг | `GOAP/Authoring/GameplayGOAPDefinitionAsset.cppm`, `GameplayGOAPDefinitionCompiler.cppm`, `GameplayAIAssetParsing.cppm`, `GameplayAIDecisionAsset.cppm` | JSON → типизированные ассеты → скомпилированное определение |
| Композиция | `src/Gameplay/AI/Composition/*.cppm`, `Composition/Components/*.cppm` | registry компонентов, template, порядок наблюдений, сборка instance, встроенные компоненты |
| Навигационные действия AI | `src/Gameplay/AI/Navigation/*.cppm` | MoveTo, FollowRoute, FollowTarget, FleeTarget, адаптер avoidance |
| Пространственный слой Gameplay | `src/Gameplay/Navigation/*.cppm` | route, route search (Dijkstra), route follower, steering, obstacle avoidance, debug registries |
| Traversal | `src/Gameplay/Traversal/*.cppm` | link/registry, executor/registry, Door, Jump |
| Navigation (Recast/Detour) | `src/Navigation/Navigation.cppm`, `src/Navigation/Recast/Navigation.cpp`, `src/App/LevelNavigation.cppm`, `src/App/AppNavigationCoordinator.cpp` | bake NavMesh по профилям агентов, FindPath |
| Интеграция App | `src/App/AppLifecycle.cpp`, `AppLifecycleImpl/AppLifecycle_TickImpl.inl`, `GameplayPhysicsObstacleQuery.h`, `GameplayPhysicsCharacterIntegration.cpp` | порядок кадра, физический query, обратная связь физики |
| Runtime-фасад | `src/Gameplay/Runtime/GameplayRuntime.cppm`, `GameplayRuntime_PublicApi.inl`, `GameplayRuntime_ModeAndBootstrap.inl`, `GameplayRuntime_FrameLifecycle.inl` | публичный API, BeginFrame/PrePhysics/PostPhysics, владение решениями |
| Development-сценарии | `src/App/Development/DevelopmentScenario.cpp`, `AppDevelopmentScenarioRuntime.cpp`, `assets/development/*.scenario.json` | воспроизводимые ручные сценарии поверх production API |
| Ассеты AI | `assets/ai/{decisions,goap,behaviors,bindings,routes}/*.json` | каталог, определения, поведения, привязки ролей, графы маршрутов |

## 2. Точки входа и композиция

- `MakeDefaultGameplayAIDecisionFactories()` (`Composition/GameplayAIBuiltinDecisions.cppm`) загружает
  `ai/decisions/catalog.json` и вызывает `RegisterGameplayAIDecisionAssets(...)` с `MakeDefaultGameplayGOAPComponents()`.
  Вызывается в `App/AppLifecycle.cpp` при создании `GameplayRuntime` (две точки: начальная загрузка и смена уровня),
  сразу после — `SetObstacleQuery(physicsState.obstacleQuery.get())`. `SOURCE_VERIFIED`.
- `GameplayRuntime()` по умолчанию создаётся с **пустым** factory registry; каталог есть только у экземпляров,
  созданных App с `MakeDefaultGameplayAIDecisionFactories()`. `SOURCE_VERIFIED`.
- `RegisterGameplayAIDecisionAssets`: проверка ID → копия `pending = destination` → для каждой записи загрузка
  behavior/bindings/definition/routeGraph и `make_shared<const GameplayGOAPDecisionTemplate>` → регистрация фабрики в
  `pending` → `destination = std::move(pending)`. Исключение на любой записи прерывает функцию до присваивания.
  `SOURCE_VERIFIED`.
- Фабрика при вызове выполняет `CreateGameplayGOAPDecisionFromTemplate(context, *prepared)` и превращает исключение в
  `nullptr` + diagnostic. `SOURCE_VERIFIED`.
- `GameplayRuntime::StartAIDecision(agent, id, diagnostic)`: требует Game-режим (`lastMode_`), отсутствие активного
  решения у агента, компоненты AI/Transform/CharacterCommand/Motor/MovementState и уровень; создаёт instance, сразу
  выполняет один `Update` и **не сохраняет** instance со статусом NotStarted/Failed/Cancelled. `SOURCE_VERIFIED`.
- Прямые входы движения: `StartAIFollowRoute`, `StartAIMoveTo`, `StartAIFollowTarget`, `StartAIFleeTarget` передают
  `obstacleQuery_` и debug registries runtime. `SOURCE_VERIFIED`.

### Сборка instance (`CreateGameplayGOAPDecisionFromTemplate`)

1. `ResolveRoles`: для каждой роли — ровно один живой `level.nodes[i]` с именем, конечная позиция, не более одной
   gameplay-сущности с `NodeLink` на этот индекс (сущность может отсутствовать → `kNullEntity`).
2. `GameplayGOAPCompositionContext{services, compiled, roles, source, observations, routeGraph, &bindings}`.
3. `ComposedContext` или `InspectedContext` (если `inspectPath`).
4. Компиляция наблюдений → `OrderGameplayGOAPObservations` (единственный writer, все факты покрыты, топологический
   порядок с выбором первого готового).
5. Реакции.
6. Для каждой группы capability: `registration.compile(assets, context)` → provider; в setup — фабрика binding,
   захватывающая сырой указатель на provider; `CostOverrides()` собираются.
7. `prepared.InstanceDefinition(costOverrides)` — копия определения с заменой стоимостей.
8. `CreateGameplayGOAPDecision(agent, setup)` → `GameplayGOAPDecisionInstance` или `DecisionWithPathInspection`
   (если context реализует `IGameplayGOAPPlannedPathProvider`); `nullptr`, если bindings неполны.

## 3. Порядок кадра (`App/AppLifecycle.cpp`, `AppLifecycle_TickImpl.inl`, `GameplayRuntime_PublicApi.inl`)

```
App::Tick
  DriveAssetStreaming
  appNavigationCoordinator::Advance          (bake NavMesh, когда геометрия готова)
  developmentScenarioRuntime.Update           (сценарные вмешательства, до BeginFrame)
  UpdateFrameTimingAndLoadingOverlay → deltaSeconds
  UpdateInputAndCamera, UpdateEditorViewportInteraction
  UpdateGameplayBeforePhysics:
    GameplayRuntime::BeginFrame               (очистка world events, InputIntent, CharacterCommand, notify, graph frame state)
    EnsureGameplayPhysicsCharacters           (Game + физика)
    GameplayRuntime::PrePhysicsUpdate:
      RemoveDeadNodeBoundEntities_, EnsureBootstrapEntity_, смена режима
      [только Game] UpdateFollowCamera_, UpdateGameplayIntentSources, BuildGameplayCharacterCommands
      objectReservationSystem_.CleanupInvalidReservations
      pickupSystem_.Update → PickupCollected events
      hideWorldEventSubjects(0)
      UpdateActiveAIDecisions_               (по возрастанию EntityHandle)
      hideWorldEventSubjects(с индекса до решений)
      aiSystem_.Update(world, dt)             (Start NotStarted + Tick Running)
      UpdateGameplayCombatRequests, UpdateGameplayInteractionRequests, ExecuteGameplayGraphs_
      UpdateGameplayCharacterMovement         (command → motor.desiredVelocity)
    SubmitGameplayPhysicsCharacterVelocities
  UpdatePhysics: SynchronizeBeforePhysics → JoltPhysicsWorld::Update(dt) → SynchronizeAfterPhysics
  UpdateGameplayAfterPhysicsAndAnimation:
    ApplyGameplayPhysicsCharacterFeedback     (позиция, grounded, physicallyBlocked)
    PostPhysicsUpdate: locomotion, SyncGameplayTransformsToRuntime, camera, PushGameplayStateToAnimation, sync
    Scene::UpdateSkinned(dt)
    PostAnimationUpdate
```

`SOURCE_VERIFIED`. Следствия (`INFERRED` из порядка вызовов):
- действие, запущенное решением, получает `Start` и первый `Tick` в одном кадре (`StartAction` синхронно вызывает
  `Start`, затем `aiSystem_.Update` тикает `Running`-задачу);
- результат физического шага кадра N виден наблюдениям только в кадре N+1;
- после успешного шага плана следующий шаг стартует не раньше следующего decision update: в кадре N tick
  возвращает `Succeeded`; в кадре N+1 `Update` синхронизирует шаг → `ReadyToStartStep`, проверяет остаток плана и
  возвращает управление (ветка `if (bSynchronizedRunningExecution) return status_;`); в кадре N+2 шаг стартует.
  В кадре N+1 у агента нет команды движения (команда очищена в `BeginFrame`, задача в `AISystem` уже terminal).

## 4. Ответы на контрольные вопросы задания

### 4.1 World state, ID, равенство, hashing, numeric validation — `SOURCE_VERIFIED`
- `AIAgentWorldState`: `std::bitset<128>` + `std::array<int32_t,128>`; `AIWorldFactId`/`AIWorldIntegerFactId` —
  разные структуры с `uint16_t index` (invalid = 65535). Невалидный индекс: `assert` + безопасный возврат
  (`false`/`0`/no-op). Якорь: `static constexpr std::size_t FactCapacity = 128u;`.
- Равенство: побитовое сравнение bitset и массива. Хеш `AIAgentWorldStateHash`: FNV-1a по 128 битам (по одному
  байту 0/1 на факт) и 128×4 байтам чисел (little-endian порядок байтов `value >> (byteIndex*8)`).
- Нет состояния «неизвестно»: `false`/`0` — значения. Факты, не имеющие writer, запрещены композицией.
- ID фактов назначаются компилятором по порядку объявления внутри типа; ID целей — по индексу цели; ID контекстов —
  по порядку действий (контекст уникален во всём определении: `duplicate action context`).
- Численные эффекты: `ApplyNumericEffects` работает на копии и при переполнении `Add` возвращает `false` (действие
  отбрасывается в поиске); `Set` перезаписывает. Числовые условия: `==,!=,<,<=,>,>=` через `operator<=>`.
- Цели содержат только булевы `desiredFacts` (числовых целей нет).

### 4.2 Алгоритм планировщика — `SOURCE_VERIFIED`
- `FindAIPlan`: uniform-cost search (Дейкстра по пространству состояний), без эвристики.
- `IsInputValid`: валидная цель; конечная неотрицательная `baseCost`; валидные ID/операторы; каждое continuation
  condition **точно** присутствует среди preconditions (числовое — совпадают fact/op/value); пары `(actionId,
  contextId)` уникальны. Невалидный вход → `nullopt` (не исключение).
- Если начальное состояние удовлетворяет цели — пустой план.
- Порядок действий: сортировка по `(actionId.value, contextId.value)`.
- Очередь: `priority_queue` с `SearchNodeGreater` — меньшая стоимость, при равенстве меньший `sequence`
  (порядок вставки). Цель проверяется при извлечении узла.
- Дубликаты: `unordered_map<state, bestCost>`; потомок отбрасывается, если `known <= nextCost`; иначе
  `insert_or_assign` и новая запись (lazy deletion); устаревшие записи пропускаются при извлечении
  (`current.cost != currentBest->second`).
- Каждый узел хранит полное состояние (~528 байт полезных данных) и **копию вектора шагов**; нет лимита раскрытий,
  бюджета времени или backoff.
- Неконечная сумма стоимости отбрасывает переход.

### 4.3 Preconditions / effects / continuation — `SOURCE_VERIFIED`
- Preconditions (+numeric): применимость в поиске; в runtime — проверка остатка плана перед стартом шага
  (`IsRemainingPlanValid_`).
- Effects (+numeric): только в копиях поискового состояния и при прогнозе остатка плана; в мир не применяются.
  Подтверждение даёт наблюдение.
- Continuation (+numeric): проверяются только для `RunningStep` при неизменившейся цели
  (`IsRunningStepContinuationValid_`).
- Важно: проверка подмножества continuation ⊆ preconditions выполняется в `FindAIPlan::IsInputValid`, а не в
  компиляторе/template. Нарушение не ломает регистрацию, но делает **любой** поиск по этому определению
  неуспешным (`nullopt`) — решение будет Failed на первом update (`INFERRED` из кода компилятора и планировщика).

### 4.4 Выбор цели, проверка плана, replanning — `SOURCE_VERIFIED`
- `SelectAIGoal`: пропускает невалидные и уже удовлетворённые цели; score = `baseScore` + сумма `scoreDelta`
  выполненных правил; выбор строго `score > best` → при равенстве побеждает **первая** цель по порядку.
- Ассетный компилятор не создаёт `scoreRules` (только `score` → `baseScore`); правила доступны лишь программно
  (`AIGoalSelectionCandidate::scoreRules`).
- Нет перебора менее приоритетных целей при неудаче поиска для выбранной.
- `AIDecisionRuntime::Update` (последовательность):
  1. если `bExplicitlyCancelled_` — вернуть статус;
  2. если шаг `RunningStep` — синхронизировать с `AISystem` (`NotStarted` трактуется как `Failed`);
  3. выбрать цель; если нет — отменить исполнение, очистить, статус `Succeeded`;
  4. если шаг только что синхронизирован, план `Succeeded`, цель прежняя — вернуть `Succeeded`;
  5. `bPlanInvalidated` (только при прежней цели): Ready → проверка остатка плана; Running → continuation;
  6. валидное прежнее исполнение → либо вернуть (если был sync), либо `Update(bindings,...)` (старт/sync шага);
  7. иначе поиск нового плана **до** отмены старого; нет плана: при смене цели и валидном исполнении — сохранить
     старое (без повторной continuation-проверки), иначе отменить, `Failed`; есть план — `InstallPlan_` (отмена
     старого, `Start()` → `ReadyToStartStep`), шаг стартует на следующем update.
- Если план выполнен (`Succeeded`), а цель ещё не подтверждена наблюдением — выполняется новый поиск
  (`bNeedsPlan` при `Succeeded` execution).

### 4.5 Failed, Cancel, retained, удаление — `SOURCE_VERIFIED`
- `Failed` decision runtime продолжает получать update и может перепланировать.
- `AIDecisionRuntime::Cancel` ставит `bExplicitlyCancelled_`; `Update` после этого ничего не делает; снимает флаг
  только успешный `Plan(...)`.
- `GameplayRuntime::CancelAIDecision(entity)` вызывает `Cancel` и удаляет instance из `activeAIDecisions_`.
- При первом неуспешном старте instance не сохраняется (см. §2).
- Переход в Editor: `CancelAllAIDecisions_`, `aiSystem_.Reset()`, reservations reset, traversal links reset,
  внешние executors удаляются.
- Мёртвые агенты: `UpdateActiveAIDecisions_` пропускает невалидные сущности, но не удаляет записи (`INFERRED`).

### 4.6 Registry, parsers/compilers, template/instance — `SOURCE_VERIFIED`
- `GameplayGOAPCompositionRegistry::Register{Observation,Reaction,Capability}<T>`: тип непустой, parser/compiler
  непусты, тип не занят; для capability — уникальный валидный `AIActionId`; транзакционно через копию parsers.
- Параметры — `std::any` с типизированным payload; извлечение `context.Parameters<T>(asset)`.
- Имя типа capability = имя семантического действия в `.goap.json` (`SemanticActions()` возвращает
  `{type, actionId}`).
- Behavior/bindings/catalog/routes: `version == 1`, **неизвестные поля запрещены** (`Fields`). `.goap.json`
  разбирается без проверки версии и без запрета лишних полей.
- Template проверяет: `model == "goap"`, наличие routeGraph при ссылке, компиляцию определения, известность типов
  observation/reaction/capability, по одной capability на контекст, совпадение `asset.type` с `action` определения,
  покрытие каждого контекста capability.
- Instance-специфичные проверки (роли, writers, cost overrides, параметры компонентов) выполняются при создании.
- Шаблон неизменяем; hot reload отсутствует; при пересоздании registry живые instance сохраняют свои данные.

### 4.7 Наблюдения и события — `SOURCE_VERIFIED`
- Writers: каждый bool/int факт ровно один writer; неизвестный индекс или повтор — ошибка; факт без writer — ошибка.
- Порядок: на каждом шаге первый наблюдатель (в исходном порядке), все входы которого «готовы» (выход уже
  поставлен); иначе «cyclic or unresolved».
- `GameplayGOAPDecisionInstance::Update`: `reactionEvents_.clear()` → `Observe(внешние)` → реакции на внешние →
  `runtimeEvents_.clear()` → `goap_.Update` → `ObserveActionEvents(runtimeEvents_)` → реакции на runtime events →
  дописывание runtime и reaction events в `eventOutput` (общий `currentWorldEvents_`).
- `ObserveActionEvents` по умолчанию пуст; переопределён в `SpatialObservation`, `NearestLocationObservation`,
  `ResourceLedgerObservation`; не переопределён в `PickupAvailabilityObservation`.
- Event confirmation против predicted effects: ledger меняет факты только по событиям `PickupCollected`
  (instigator == агент, subject == настроенный pickup, ещё не подтверждён) и `ResourcePurchased`
  (subject+receiptId, результат ещё не подтверждён, баланс ≥ цены).

### 4.8 Ledger, purchase, receipts, reactions, reservations — `SOURCE_VERIFIED`
- Ledger: int64-промежуточная сумма, переполнение → кредит пропускается целиком.
- `PurchaseRuntime::Start`: actionId, живой продавец, результат не получен, баланс ≥ цены, 3D-дистанция до
  сохранённой позиции продавца ≤ radius (по умолчанию 0.6) → событие `ResourcePurchased` в `runtimeEvents_`,
  `Succeeded`. Факты сам не меняет.
- `PurchaseCapability` проверяет соответствие модели: ровно один bool-эффект = результат receipt, ровно один
  numeric-эффект `add -price` по ресурсу ledger, и numeric-precondition `resource >= price`.
- `HideOnPurchaseReaction`: один раз на instance; при подтверждённом результате и событии покупки с тем же
  receipt — `HideEntityRequested` (target может отличаться от продавца; нужен node-bound entity).
- Reservations (`GameplayObjectReservationSystem`): владелец на объект; объект должен иметь InteractionPoint, агент —
  AI. `TryReserve` повторно для того же владельца — успех. Очистка невалидных записей в начале PrePhysics.
- `ReservedAIMoveToActionRuntime`: Start требует, чтобы цель **никем** не была зарезервирована
  (`IsReserved(target_)` → Failed даже для собственной брони), затем `TryReserve`; Tick проверяет владение;
  освобождение при terminal/Cancel.
- `pickup_available` с `respectReservations`: чужая бронь → false.
- `GameplayPickupSystem` **не учитывает reservations**: собирает любой AI-коллектор в 3D-радиусе; при нескольких —
  первый по порядку `CollectAIEntities` (`INFERRED`: гарантия «бронь = эксклюзивный сбор» отсутствует).

### 4.9 Маршруты: источники и стоимость в GOAP — `SOURCE_VERIFIED`
- Авторский граф: `.routes.json` (`nodes` — имена **ролей**, `edges` from/to/[cost]/[traversal]); стоимость ребра —
  `cost` или 3D-длина между позициями ролей; traversal → `GameplayRouteSegmentAnnotation` с проверкой привязки,
  типа (`jump`/`door`), link в registry и целевой сущности.
- Без графа: для каждой capability граф из двух узлов и одного ребра стоимостью 1; `routeCost` запрещён.
- `routeCost: true` → `CostOverrides` = полная стоимость `FindWeightedGameplayRoute` на момент создания instance.
- `FindWeightedGameplayRoute`: Дейкстра по узлам, predecessor-массивы, tie-break по порядку вставки, start == goal →
  маршрут из одной точки и стоимостью 0.
- NavMesh: только прямые development-операции (`startNavigationPath` в Maze) → `World::FindPath` → points →
  `GameplayRoute` без аннотаций → `StartAIFollowRoute`.
- `sourceRadius > 0`: ближайший узел графа к текущей 3D-позиции должен быть `start`, а расстояние ≤ sourceRadius,
  иначе `ResolveRequest` → `nullopt` → binding `nullptr` → шаг Failed.

### 4.10 Роли: snapshot — `SOURCE_VERIFIED`
- `GameplayAIResolvedRole{role, entity, position}` формируется один раз при создании instance из
  `LevelAsset.nodes[i].transform.position`. Spatial-наблюдения, MoveTo-графы, purchase используют сохранённую позицию;
  проверки доступности — сохранённый handle. Пересоздание сущности не перепривязывает роль.

### 4.11 Передача сервисов — `SOURCE_VERIFIED`
- `GameplayAIDecisionCreationContext`: agent, level, world, traversalLinkRegistry, traversalExecutorRegistry,
  reservationSystem, diagnostic. Нет obstacle query и debug registries.
- `MoveToCapability::CreateBinding` → `AIMoveToActionBinding(world, links, executors, *this, reservations, *this)` —
  obstacle query, steering debug, navigation debug = `nullptr`.
- Прямые `StartAI*` и `CreateAIMoveToActionBinding` передают `obstacleQuery_`, `&steeringDebugRegistry_`,
  `&navigationDebugRegistry_` (Follow/Flee — без navigation registry).

### 4.12 Порядок кадра — см. §3.

### 4.13 Координаты, единицы, arrival-контракты — `SOURCE_VERIFIED`
- Мир: правосторонняя система, +Y вверх (комментарий в `Navigation.cppm`: «right-handed, Y-up»). Steering —
  плоскость XZ (`planarDelta.y = 0`). Единицы — метры/секунды (по значениям скоростей 2.0/4.5 м/с,
  `INFERRED`).
- Transform gameplay-персонажа — основание (ступни): позиция капсулы минус половина полной высоты
  (`visualRootOffset`).
- Контракты прибытия: Seek/Arrival — горизонтальная дистанция ≤ r + 0.001; route follower — промежуточные углы с
  `slowingRadius = acceptanceRadius`; `within_distance`/`nearest_location`/pickup/purchase — **3D** расстояние;
  Jump landing — горизонтальная и вертикальная допуски отдельно.

### 4.14 Look-ahead, corner transition, Follow/Flee, hysteresis, avoidance, support, traversal — `SOURCE_VERIFIED`
- Look-ahead (`TryBuildLookAheadTarget_`): L по умолчанию 0.75; работает только если есть следующий сегмент без
  traversal, оба сегмента невырождены в XZ, расстояние до угла `d < L`; цель
  `q = c + û_out · min(L − d, |out|)`, `q.y = c.y`; Seek с acceptance 0.
- Corner transition (`HasPassedCurrentCorner_`): `d ≤ L` и `dot(p − c, n̂) ≥ 0`, `n̂ = norm(û_in + û_out)` или `û_in`
  при вырождении; не более одного продвижения look-ahead за tick; while-цикл продвигает и по обычному arrival.
- Follow: Seek (без Arrival) с acceptance (по умолчанию 0.25), остаётся Running; интервал 0.05 с, `fmod`-остаток;
  кеш намерения переиздаётся каждый tick.
- Flee: XZ-дистанция; спокойно → `d ≤ trigger`; бегство → `d < safe`; safe = max(safe, trigger); defaults 2/3/0.05.
- Avoidance: probes forward/left/right всегда (при движении); forward distance = max(L0, v·τ); escape = norm(0.6·feeler
  + 0.4·tangent + 0.3·n); support по escape-направлениям; выбор стороны с приоритетом опоры и гистерезисом 0.15.
- Traversal: аннотация → `TraversalRequired` → executor Start/Tick; синхронный успех продвигает follower, движение —
  на следующем tick.

### 4.15 physicallyBlocked — `SOURCE_VERIFIED`
- Вычисляется в `ApplyGameplayPhysicsCharacterFeedback`: на каждом фиксированном шаге 1/60 с при опоре, желаемой
  планарной скорости > 0.1 и фактической < 10 % желаемой накапливается время; флаг при ≥ 0.25 с.
- `AIFollowRouteActionRuntime` только копирует флаг в `physicallyBlocked_` (`IsPhysicallyBlocked()`); recovery нет.
- Отладочная визуализация читает флаг (`AppDebugVisualization.cpp`).

## 5. Дополнительные находки (не отражены или неточны в PDF)

1. Межшаговая пауза в один кадр без команды движения между шагами плана (§3). `INFERRED`.
2. Continuation ⊆ preconditions проверяется только планировщиком; ошибка автора проявляется как вечный `Failed`, а не
   как ошибка регистрации (§4.3). `INFERRED`.
3. Pickup system игнорирует reservations (§4.8). `SOURCE_VERIFIED` + `INFERRED` последствие.
4. `ReservedAIMoveToActionRuntime::Start` отклоняет уже собственную бронь. `SOURCE_VERIFIED`.
5. Door executor ведёт агента к InteractionPoint двери, открывает дверь логически и завершает сегмент; конечная точка
   аннотированного сегмента физически не посещается — follower сразу переходит к следующему сегменту
   (`INFERRED` из `DoorTraversalExecutor::Tick` и `CompleteTraversal`).
6. Jump executor: подход к takeoff с collapse slowing interval → запрос `kGameplayActionJump` (Script, priority 200) и
   временная подмена `motor->jumpVerticalSpeed` → ожидание Airborne → приземление с допусками → тайм-аут 10 с.
7. Физический obstacle query по умолчанию использует `PhysicsQueryLayerMask::StaticWorld` — другие агенты и
   динамические тела не видны avoidance. `SOURCE_VERIFIED`.
8. `GameplayRuntime()` без аргументов → пустой каталог решений. `SOURCE_VERIFIED`.
9. `temporarilyRemoveTargetAtMidpoint` использует `std::chrono::steady_clock` (реальное время, не время симуляции) и
   3D-дистанцию. `SOURCE_VERIFIED`.
10. Несогласованность контрактов прибытия в ассетах Marker Visit и Target Recovery: follower останавливается при
    горизонтальной дистанции ≤ r + 0.001, а факт подтверждается при 3D-дистанции ≤ R, где r = R (0.8 и 0.4), а цель
    поднята на 0.2 над узлом старта. Если основание агента на уровне y≈0, остановка на краю acceptance даёт 3D-дистанцию
    √(r² + 0.2²) > R, факт может не подтвердиться, и решение будет перепланировать каждые несколько кадров.
    `INFERRED`, `NOT_VERIFIED` в приложении (высота пола и фактическая точка остановки не измерялись).
11. `IsMoving()` сравнивает `Dot(moveWorld, moveWorld) > kLengthEpsilon` (не `Sq`) — порог 1e-6 для квадрата длины.
    Косметика, на поведение практически не влияет. `SOURCE_VERIFIED`.

## 6. Тесты (наличие, не запуск)

GoogleTest-сьюты AI/навигации в `tests/unit/GameplayTests/**`, `tests/unit/NavigationTests/TestNavigation.cpp`,
`tests/integration/AIPhysicsSmoke/TestAIPhysicsSmoke.cpp`. Количество TEST по сьютам (grep): AIPlanner 11,
AIDecisionRuntime 13, AIGoalSelection 13, AIAgentWorldState 7, AIPlanExecution 10, AISystem 13, AIActionTask 12,
GameplayAIAssetComposition 14, GameplayGOAPResources 14, GameplayGOAPDecisionTemplate 4, GameplayGOAPDefinitionCompiler 7,
GameplayRouteSearch 41, GameplayRouteFollower 34, GameplaySteering 24, GameplayObstacleAvoidance 39,
AIFollowRouteActionRuntime 20, JumpTraversalExecutor 15, TestDoorTraversalExecutor 14 и др.
Статус выполнения: `NOT_RUN` на момент аудита. Готовый `out/build/x64-Debug-DX12/tests/CoreEngineModuleTests.exe`
датирован 30.08.2026 — собран из старого снимка и не доказывает поведение текущего HEAD.

Python-проверки: `tests/tools/run_goap_asset_portable_checks.py` (требует GCC), `tests/tools/check_goap_asset_composition.py`.

## 7. Сценарии

| Сценарий | Файл | Вход в AI | Уровень |
|---|---|---|---|
| Marker Visit | `ai_goap_marker_visit.scenario.json` | `startAIDecision marker_visit` | `ai_goap_marker_visit_development.level.json` |
| Target Recovery | `ai_goap_target_recovery.scenario.json` | `startAIDecision target_recovery` + `temporarilyRemoveTargetAtMidpoint` (0.5, 5 с) | `ai_goap_target_recovery_development.level.json` |
| Access Key | `ai_goap_access_key.scenario.json` | `access_key` | `ai_goap_access_key_development.level.json` |
| Access Key Replanning | `ai_goap_access_key_replanning.scenario.json` | `access_key` + coinA собирается сценарием после сбора coinC | `..._replanning_development.level.json` |
| Access Key Reservation | `ai_goap_access_key_reservation.scenario.json` | два агента, `access_key_reserved` | `..._reservation_development.level.json` |
| Access Key Jump | `ai_goap_access_key_jump.scenario.json` | `access_key_jump` + jump link 9470001 | `..._jump_development.level.json` |
| Steering Playground | `ai_steering_playground.scenario.json` | `startFollowTarget`, `startFleeTarget` (5/7), `startMoveTo` | `ai_steering_playground.level.json` |
| Navigation Maze | `navigation_pathfinding_maze.scenario.json` | `startNavigationPath` (extents 1,2,1; r 0.25; R 0.9) | `navigation_pathfinding_maze.level.json` |

Реальный пример (пересчитан независимой Python-моделью, `audit/calc/`): Access Key, позиции из уровня:
start (0,0.03,0), coinA (−6,0.35,−3), coinB (6,0.35,1), coinC (−5,0.35,6), shop (0,0.35,−7), goal (0,0.08,10).
Оптимальный план: `start_to_coin_c` 7.8168 → `coin_c_to_coin_a` 9.0554 → `coin_a_to_access_key_shop` 7.2111 →
`buy_key` 1 → `access_key_shop_to_goal` 17.0021; сумма 42.0854; раскрыто 22 состояния. После сбора coinC и
исчезновения coinA: `coin_c_to_coin_b` → `coin_b_to_access_key_shop` → `buy_key` → `access_key_shop_to_goal`,
стоимость 40.0852.
