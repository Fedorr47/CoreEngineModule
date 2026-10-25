"""Ручной архитектурный разбор ключевых сущностей для карточек reference/classes/<Name>.md.

Ключ — имя сущности из docs/data/class-index.json (неизвестное имя ломает генерацию).
Поля: purpose, owner, data, methods, callers, calls, errors, theory (имена глав без .md), diagrams, refs, tests.
Каждое утверждение проверено по исходникам снимка; источники — в refs.
"""
NOTES = {
    'AIAgentWorldState': dict(
        purpose='Вектор знания агента: 128 bool (bitset) и 128 int32; единица учёта состояний в поиске.',
        owner='Значение. Факты агента принадлежат GameplayGOAPDecision (по одной копии на instance); копии создаёт поиск и проверка остатка плана.',
        data='Пишут: наблюдения (Observe/ObserveActionEvents), эффекты в копиях поиска. Читают: выбор цели, поиск, PurchaseRuntime, отладка.',
        methods='IsFactSet/SetFact/ClearFact, GetIntegerFact/SetIntegerFact, Clear, operator== (полное равенство). Хеш — AIAgentWorldStateHash (FNV-1a по всем ячейкам).',
        callers='Наблюдения в update решения; FindAIPlan; AIDecisionRuntime::IsRemainingPlanValid_.',
        errors='Невалидный индекс: assert в debug, безопасная деградация (false/0, запись игнорируется).',
        theory=['03-state-model'], diagrams=['d03-state-layout'], refs=['WS-CLASS', 'WS-HASH', 'WS-SETFACT'],
        tests=['TestAIAgentWorldState.cpp']),
    'FindAIPlan': dict(
        purpose='Uniform-cost search плана от наблюдаемого состояния к цели; без эвристики A*.',
        owner='Свободная функция без состояния; узлы поиска — локальные (копия состояния и шагов).',
        methods='(initialState, goal, actions) → optional<AIPlan>; пустой план, если цель уже выполнена; nullopt при невалидном входе или недостижимости.',
        callers='AIDecisionRuntime::Update (смена цели, инвалидация, нет/terminal исполнения), AIDecisionRuntime::Plan.',
        calls='IsInputValid, AreFactConditionsSatisfied, AreNumericConditionsSatisfied, ApplyFactEffects, ApplyNumericEffects.',
        errors='Нет лимита раскрытий и бюджета; при бесконечном достижимом пространстве и недостижимой цели не завершится.',
        theory=['05-planner', '02-goap-strips'], diagrams=['d05-search-loop', 'g05-access-key-map'],
        refs=['PL-FIND', 'PL-INPUT', 'PL-RELAX', 'PL-STALE'], tests=['TestAIPlanner.cpp']),
    'SelectAIGoal': dict(
        purpose='Выбор невыполненной цели с максимальной оценкой; при равенстве — первая.',
        methods='(state, candidates) → AIGoalSelectionResult; оценка = baseScore + Σ scoreDelta выполненных правил.',
        callers='AIDecisionRuntime::Update каждый update.',
        errors='Невалидные цели и неконечные оценки пропускаются; «нет цели» → решение Succeeded.',
        theory=['04-goals-actions'], diagrams=['d04-goal-selection'], refs=['GS-SELECT', 'GS-TIE', 'AU-GOAL-BUILD'],
        tests=['TestAIGoalSelection.cpp']),
    'AIDecisionRuntime': dict(
        purpose='Мониторинговый контур одного агента: синхронизация шага, выбор цели, проверка плана, перепланирование, старт шагов.',
        owner='Член GameplayGOAPDecision (по значению); владеет optional<AIPlanExecution>. Факты — у вызывающего.',
        data='execution_, selectedGoalId_, status_, bExplicitlyCancelled_.',
        methods='Update(facts, goals, actions, bindings, aiSystem, world) → статус; Update(bindings, ...) — старт/синхронизация шага; Plan(...); Cancel(aiSystem).',
        callers='GameplayGOAPDecision::Update из GameplayGOAPDecisionInstance::Update (PrePhysicsUpdate, UpdateActiveAIDecisions_); GameplayRuntime::UpdateAIDecision (программно).',
        calls='SelectAIGoal, FindAIPlan, AIPlanExecutionBridge::{StartReadyPlanStep, SynchronizeRunningPlanStep, CancelPlanExecution}.',
        errors='Нет плана → Failed (instance жив); explicit Cancel блокирует автопродолжение до успешного Plan().',
        theory=['08-decision-runtime'], diagrams=['d08-update-branches', 'd08-execution-states', 'd08-recovery-sequence'],
        refs=['DR-UPDATE', 'DR-INVALIDATED', 'DR-KEEP-RUNNING', 'DR-CANCEL'], tests=['TestAIDecisionRuntime.cpp']),
    'AIPlanExecution': dict(
        purpose='Конечный автомат исполнения плана по индексу шага.',
        owner='optional внутри AIDecisionRuntime; пересоздаётся при установке нового плана.',
        methods='Start, MarkCurrentStepStarted, MarkCurrentStepStartFailed, ApplyCurrentStepStatus, Cancel, GetCurrentStep.',
        callers='AIDecisionRuntime, AIPlanExecutionBridge.',
        errors='Невалидная цель/шаг в конструкторе → Failed; Cancel только из нетерминального состояния.',
        theory=['08-decision-runtime'], diagrams=['d08-execution-states'], refs=['PE-CLASS', 'PE-APPLY'],
        tests=['TestAIPlanExecution.cpp']),
    'AIPlanExecutionBridge': dict(
        purpose='Мост от семантического шага к задаче AISystem: binding → runtime → StartAction; синхронизация и отмена.',
        methods='StartReadyPlanStep (мгновенный успех сразу продвигает план), SynchronizeRunningPlanStep (NotStarted трактуется как Failed), CancelPlanExecution.',
        callers='AIDecisionRuntime.',
        calls='AIActionBindingRegistry::Find, IAIActionBinding::CreateRuntime, AISystem::StartAction/GetActionStatus/CancelAction.',
        errors='Нет binding/runtime или неуспешный старт → шаг Failed.',
        theory=['09-execution-lifetime'], diagrams=['d09-action-lifecycle'], refs=['BR-START', 'BR-SYNC', 'BR-CANCEL'],
        tests=['TestAIPlanExecutionBridge.cpp']),
    'AISystem': dict(
        purpose='Владелец задач действий: не более одной задачи на агента; синхронный старт и покадровый тик.',
        owner='Член GameplayRuntime по значению; хранит unordered_map<EntityHandle, AIActionTask>.',
        methods='StartAction (отменяет и заменяет прежнюю задачу, вызывает Start), Update(world, dt) (очистка задач мёртвых агентов, Tick Running), CancelAction, ClearAction, Reset, GetActionStatus.',
        callers='Мост решений (старт/статус); GameplayRuntime::PrePhysicsUpdate (Update после решений); прямые StartAI*.',
        errors='Terminal-задача остаётся до замены или смерти агента — её статус читают решения.',
        theory=['09-execution-lifetime', '10-frame-order'], diagrams=['d09-action-lifecycle', 'd09-task-states'],
        refs=['AS-CLASS', 'AS-START', 'AS-UPDATE', 'AS-CLEANUP'], tests=['TestAISystem.cpp']),
    'AIActionTask': dict(
        purpose='Владеющая обёртка IAIActionRuntime со статусом и порядком протокола Start/Tick/Cancel.',
        owner='Значение в карте AISystem; владеет unique_ptr runtime.',
        methods='Start (однократно), Tick (только Running), Cancel (только Running).',
        theory=['09-execution-lifetime'], diagrams=['d09-task-states'], refs=['AT-CLASS', 'AT-CANCEL'],
        tests=['TestAIActionTask.cpp']),
    'IAIActionRuntime': dict(
        purpose='Протокол исполнения шага: Start → Running/Succeeded/Failed, Tick, Cancel (без применения эффектов).',
        owner='Реализации принадлежат AIActionTask.',
        callers='AIActionTask.',
        theory=['09-execution-lifetime'], refs=['AR-IFACE', 'AR-RESULT'], tests=['TestAIActionRuntime.cpp']),
    'IAIActionBinding': dict(
        purpose='Фабрика runtime для шага плана по контексту (позднее связывание семантики и исполнителя).',
        owner='Реализации принадлежат GameplayGOAPDecision (ownedBindings_); реестр хранит сырые указатели по actionId.',
        callers='AIPlanExecutionBridge::StartReadyPlanStep.',
        theory=['09-execution-lifetime'], refs=['AB-IFACE', 'AB-REGISTRY'], tests=['TestAIActionBinding.cpp']),
    'GameplayGOAPDecisionInstance': dict(
        purpose='Instance решения одного агента: контекст наблюдений, реакции, буферы событий и GOAP-обёртка.',
        owner='unique_ptr в GameplayRuntime::activeAIDecisions_. Порядок полей: goap_ уничтожается первым, контекст — последним. Копирование/перемещение запрещены.',
        methods='Update(aiSystem, observation): два прохода наблюдений и реакций вокруг GOAP-update; Cancel; GetStatus; интерфейс инспекции.',
        callers='GameplayRuntime::UpdateActiveAIDecisions_ (по возрастанию handle), StartAIDecision (первый update).',
        errors='Неполные bindings → configured_ = false → CreateGameplayGOAPDecision возвращает nullptr.',
        theory=['07-observations-events', '09-execution-lifetime'], diagrams=['d07-instance-update', 'd09-ownership'],
        refs=['GG-INSTANCE', 'GG-INSTANCE-UPDATE', 'GG-FIELD-ORDER', 'GG-CREATE'], tests=['TestGameplayAIAssetComposition.cpp']),
    'GameplayGOAPDecision': dict(
        purpose='GOAP-обёртка: AIDecisionRuntime + неизменяемое определение + факты агента + bindings.',
        owner='Член GameplayGOAPDecisionInstance по значению; владеет bindings.',
        methods='InstallActionBinding, HasCompleteActionBindings, Update, Cancel, BuildDebugViewModel (имена из metadata).',
        theory=['08-decision-runtime'], refs=['GG-DECISION'], tests=['TestGameplayAIDecision.cpp']),
    'GameplayGOAPDecisionTemplate': dict(
        purpose='Неизменяемый снимок конфигурации решения, независимый от мира и агента.',
        owner='shared_ptr<const> захвачен фабрикой каталога; разделяется всеми instance.',
        methods='Конструктор (model, routeGraph, компиляция, типы, покрытие контекстов), InstanceDefinition(overrides), аксессоры.',
        callers='RegisterGameplayAIDecisionAssets (создание), CreateGameplayGOAPDecisionFromTemplate (использование).',
        errors='Исключение с префиксом «AI behavior <source>» при любой ошибке валидации.',
        theory=['06-assets-composition'], diagrams=['d06-template-instance', 'd06-composition-classes'],
        refs=['CO-TEMPLATE', 'CO-INSTANCE-DEF'], tests=['TestGameplayGOAPDecisionTemplate.cpp']),
    'GameplayGOAPCompositionRegistry': dict(
        purpose='Реестр типов компонентов: parser (JSON → T) + compiler (T + контекст → объект) для наблюдений, capabilities и реакций.',
        owner='Значение; копия хранится в каждом template.',
        methods='RegisterObservation/Capability/Reaction<T>, SemanticActions (тип capability = имя действия), AssetParsers.',
        errors='Регистрация отвергается при пустом типе, повторе или повторном actionId.',
        theory=['06-assets-composition'], refs=['CO-REGISTRY', 'CO-REG-CAP', 'CO-SEMANTIC'],
        tests=['TestGameplayGOAPDecisionTemplate.cpp']),
    'OrderGameplayGOAPObservations': dict(
        purpose='Проверка единственного writer и топологический порядок наблюдений по метаданным входов/выходов.',
        callers='CreateGameplayGOAPDecisionFromTemplate.',
        errors='«unknown fact or multiple observation writers», «fact has no observation writer», «cyclic or unresolved observation dependencies».',
        theory=['07-observations-events'], diagrams=['d07-observation-deps'], refs=['CO-ORDER', 'CO-ORDER-PICK'],
        tests=['TestGameplayAIAssetComposition.cpp']),
    'RegisterGameplayAIDecisionAssets': dict(
        purpose='Транзакционная регистрация каталога решений: все templates или ничего.',
        callers='MakeDefaultGameplayAIDecisionFactories (App при создании GameplayRuntime).',
        errors='Ошибка любой записи → исключение до публикации реестра.',
        theory=['06-assets-composition'], refs=['CO-REGISTER-ASSETS', 'CO-PENDING'], tests=['TestGameplayAIAssetComposition.cpp']),
    'CreateGameplayGOAPDecisionFromTemplate': dict(
        purpose='Связывание template с уровнем и создание instance: роли, наблюдения, реакции, providers, cost overrides.',
        callers='Фабрика каталога из GameplayRuntime::StartAIDecision.',
        errors='Исключение → фабрика возвращает nullptr и пишет diagnostic.',
        theory=['06-assets-composition'], diagrams=['d06-template-instance'], refs=['CO-FROM-TEMPLATE', 'CO-RESOLVE-ROLES']),
    'MoveToCapability': dict(
        purpose='Provider move_to: компиляция графа маршрутов, route costs, запросы MoveTo, бронь цели, debug-маршрут.',
        owner='unique_ptr в ComposedContext instance; binding заимствует его.',
        methods='CreateBinding (без obstacle query и debug), CostOverrides, ResolveRequest (sourceRadius), ResolveReservationTarget, BuildDebugRoute.',
        errors='Ошибки создания: радиусы, нет пути, routeCost без графа, неверный traversal link.',
        theory=['11-spatial-routes', '09-execution-lifetime'], diagrams=['d11-route-sources'],
        refs=['CP-MOVE-CAP', 'CP-MOVE-GRAPH', 'CP-MOVE-RESOLVE', 'CP-MOVE-BINDING'], tests=['TestAIMoveToActionBinding.cpp']),
    'AIMoveToActionBinding': dict(
        purpose='Binding MoveTo: запрос у провайдера → обычный или бронирующий runtime.',
        methods='CreateRuntime(context): actionId 2, запрос; при цели брони — ReservedAIMoveToActionRuntime.',
        theory=['09-execution-lifetime'], refs=['NA-MOVETO-BINDING'], tests=['TestAIMoveToActionBinding.cpp']),
    'ReservedAIMoveToActionRuntime': dict(
        purpose='MoveTo с бронью InteractionPoint цели на время шага.',
        methods='Start (отклоняет любую существующую бронь, бронирует), Tick (потеря брони → отмена, Failed), Cancel (освобождение).',
        theory=['17-resources-multiagent'], diagrams=['d17-contention'], refs=['NA-RESERVED', 'NA-RESERVED-START']),
    'AIFollowRouteActionRuntime': dict(
        purpose='Исполнение маршрута: follower, avoidance (если есть query), команда и facing, traversal handoff, physicallyBlocked.',
        owner='Владеет копией маршрута и follower; заимствует мир, реестры traversal, query, debug-реестры.',
        methods='Start (публикация маршрута), Tick, Cancel, IsPhysicallyBlocked.',
        callers='AISystem::Update через AIActionTask.',
        errors='Невалидный агент/маршрут, неуспех executor → Failed с остановкой (вертикаль сохраняется).',
        theory=['12-route-follower', '16-traversal-physics'], diagrams=['d16-traversal-handoff'],
        refs=['NA-ROUTE-RT', 'NA-ROUTE-TICK', 'NA-ROUTE-TRAVERSAL', 'NA-ROUTE-APPLY'], tests=['TestAIFollowRouteActionRuntime.cpp']),
    'GameplayRouteFollower': dict(
        purpose='Автомат следования по ломаной: arrival промежуточных/финальной точек, look-ahead, плоскость перехода, traversal.',
        owner='Член AIFollowRouteActionRuntime; владеет маршрутом.',
        methods='Start, Tick(position) → намерение/статус, CompleteTraversal(handle), Reset.',
        theory=['12-route-follower'], diagrams=['d12-tick-flow', 'd12-follower-states', 'g12-lookahead', 'g12-corner-plane'],
        refs=['RF-TICK', 'RF-LOOKAHEAD', 'RF-PASSED', 'RF-COMPLETE'], tests=['TestGameplayRouteFollower.cpp']),
    'FindWeightedGameplayRoute': dict(
        purpose='Дейкстра по явному ориентированному графу с восстановлением пути по предшественникам.',
        callers='MoveToCapability (route costs при создании instance), AIMoveToAction::CreateRuntime (маршрут шага).',
        errors='InvalidGraph, InvalidRequest, NoRoute; start == goal → маршрут из одной точки.',
        theory=['11-spatial-routes'], refs=['RS-FIND', 'RS-SAME'], tests=['TestGameplayRouteSearch.cpp']),
    'BuildGameplayArrivalSteering': dict(
        purpose='Arrival в XZ: нулевая команда в r + 0.001, линейная шкала до R с нижней границей m_min.',
        callers='GameplayRouteFollower, Door/Jump executors.',
        theory=['13-steering'], diagrams=['g13-arrival-profile'], refs=['ST-ARRIVAL'], tests=['TestGameplaySteering.cpp']),
    'ApplyGameplayObstacleAvoidance': dict(
        purpose='Локальная коррекция намерения: три sweep-пробы, escape-кандидаты, опора, выбор стороны с памятью.',
        callers='ApplyGameplayAgentObstacleAvoidance из runtimes маршрута и Follow/Flee при наличии query.',
        errors='Нет опоры у обоих кандидатов → нулевая команда.',
        theory=['15-obstacle-avoidance'], diagrams=['g15-feelers', 'g15-support', 'd15-side-selection'],
        refs=['OA-APPLY', 'OA-ESCAPE', 'OA-SUPPORT', 'OA-SIDE'], tests=['TestGameplayObstacleAvoidance.cpp']),
    'IGameplayObstacleQuery': dict(
        purpose='Интерфейс геометрических запросов avoidance: Probe (sweep) и ProbeSupport (опора; по умолчанию синтетическая).',
        owner='Реализация GameplayPhysicsObstacleQuery принадлежит App (AppPhysicsState); runtime хранит невладеющий указатель.',
        theory=['15-obstacle-avoidance'], diagrams=['d15-pipeline'], refs=['OA-QUERY', 'OA-SUPPORT-DEFAULT', 'OA-PHYS-QUERY']),
    'AIFollowTargetActionRuntime': dict(
        purpose='Следование за живой целью: Seek с выборкой и переизданием команды; бесконечно Running.',
        theory=['14-temporal-behaviors'], diagrams=['d14-sampling'], refs=['NA-FOLLOW-RT', 'NA-FOLLOW-TICK']),
    'AIFleeTargetActionRuntime': dict(
        purpose='Бегство от живой цели с гистерезисом trigger/safe и выборкой.',
        theory=['14-temporal-behaviors'], diagrams=['g14-flee-hysteresis', 'd14-flee-states'],
        refs=['NA-FLEE-REFRESH', 'NA-FLEE-HYST', 'NA-FLEE-SETTINGS']),
    'JumpTraversalExecutor': dict(
        purpose='Прыжок по link: подход к takeoff, запрос kGameplayActionJump, ожидание взлёта, приземление в допусках, тайм-аут 10 с.',
        owner='Член GameplayRuntime; зарегистрирован runtime-owned в реестре executors.',
        theory=['16-traversal-physics'], diagrams=['d16-jump-states'], refs=['TV-JUMP', 'TV-JUMP-TICK', 'TV-JUMP-TIMEOUT'],
        tests=['TestJumpTraversalExecutor.cpp']),
    'DoorTraversalExecutor': dict(
        purpose='Дверь по link: бронь, подход к InteractionPoint, логическое открытие, освобождение.',
        theory=['16-traversal-physics'], refs=['TV-DOOR', 'TV-DOOR-TICK'], tests=['TestDoorTraversalExecutor.cpp']),
    'ResourceLedgerObservation': dict(
        purpose='Бухгалтерия ресурса по подтверждённым событиям сбора и покупки своего агента.',
        theory=['17-resources-multiagent', '07-observations-events'], diagrams=['d17-purchase-sequence'],
        refs=['CP-LEDGER', 'CP-LEDGER-OBSERVE'], tests=['TestGameplayGOAPResourceComponents.cpp']),
    'PurchaseRuntime': dict(
        purpose='Мгновенная покупка: проверки и событие ResourcePurchased; факты не меняет.',
        theory=['17-resources-multiagent'], diagrams=['d17-purchase-sequence'], refs=['CP-PURCHASE-RT', 'CP-PURCHASE-CAP']),
    'GameplayObjectReservationSystem': dict(
        purpose='Необязательные брони объектов с InteractionPoint агентами AI.',
        owner='Член GameplayRuntime; сбрасывается при переходе в Editor.',
        theory=['17-resources-multiagent'], diagrams=['d17-contention'], refs=['RV-SYSTEM', 'RV-TRY'],
        tests=['TestGameplayObjectReservationSystem.cpp']),
    'GameplayRuntime': dict(
        purpose='Фасад gameplay: владеет AISystem, решениями, реестрами traversal, бронями; задаёт порядок кадра.',
        owner='Принадлежит App (runtimeState.gameplayRuntime); создаётся с каталогом решений.',
        methods='BeginFrame, PrePhysicsUpdate, PostPhysicsUpdate, StartAIDecision, CancelAIDecision, StartAI*, BuildAIDebugAgentViews, SetObstacleQuery.',
        theory=['01-architecture', '10-frame-order'], diagrams=['d10-frame-sequence', 'd09-ownership'],
        refs=['GR-PREPHYS', 'GR-START-DECISION', 'GR-CTOR', 'GR-MEMBERS'], tests=['TestGameplayRuntimeModeTransitions.cpp']),
    'GameplayPhysicsObstacleQuery': dict(
        purpose='Адаптер Jolt для avoidance: sweep сферы / луч и луч вниз для опоры; маска StaticWorld по умолчанию.',
        theory=['15-obstacle-avoidance'], diagrams=['d15-pipeline'], refs=['OA-PHYS-QUERY', 'OA-PHYS-PROBE', 'OA-PHYS-SUPPORT']),
    'World': dict(
        purpose='NavMesh одного профиля агента: bake Recast и запросы Detour.',
        owner='ProfileRegistry (по одному на профиль), принадлежит App.',
        methods='Build, ProjectPoint, FindPath (частичный коридор → NoPath), BuildDebugGeometry.',
        theory=['11-spatial-routes'], diagrams=['d11-maze-sequence'], refs=['NV-FINDPATH', 'NV-PARTIAL', 'NV-CONFIG']),
}
