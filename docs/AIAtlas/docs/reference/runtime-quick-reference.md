# Runtime: быстрый справочник

Компактная шпаргалка по исполнению AI на снимке `8c6c0dc`. Подробности, теория и ограничения — в главах; каждая
строка ссылается на реестр источников. Схемы масштабируются и раскрываются кнопками над ними.

## Публичные точки входа (`GameplayRuntime`)

<div data-filter-table markdown>

| Вызов | Вид | Когда / триггер | Результат | Глава | Источник |
|---|---|---|---|---|---|
| `StartAIDecision(agent, id, diag)` | решение | сценарий/игра, режим Game | `true` + instance после успешного первого update; иначе `false` без instance | [08](../chapters/08-decision-runtime.md) | [[GR-START-DECISION]] |
| `CancelAIDecision(agent)` | решение | остановка | `Cancel` + удаление instance | [08](../chapters/08-decision-runtime.md) | [[GR-CANCEL-DECISION]] |
| `GetAIDecisionStatus(agent)` | решение | опрос | `AIPlanExecutionStatus` или `NotStarted` | [08](../chapters/08-decision-runtime.md) | [[GR-START-DECISION]] |
| `GetAIDecisionObservedState(agent)` | решение | отладка | указатель на факты | [07](../chapters/07-observations-events.md) | [[GR-DEBUG-VIEWS]] |
| `StartAIFollowRoute(agent, route, steering)` | действие | прямой запуск | статус старта; query + debug переданы | [11](../chapters/11-spatial-routes.md) | [[GR-START-ROUTE]] |
| `StartAIMoveTo(agent, graph, start, goal, steering)` | действие | прямой запуск | Дейкстра → FollowRoute | [11](../chapters/11-spatial-routes.md) | [[GR-START-MOVETO]] |
| `StartAIFollowTarget(agent, target, settings)` | действие | прямой запуск | Seek с выборкой, бесконечно `Running` | [14](../chapters/14-temporal-behaviors.md) | [[GR-START-FOLLOW]] |
| `StartAIFleeTarget(agent, target, settings)` | действие | прямой запуск | гистерезис 2/3 по умолчанию | [14](../chapters/14-temporal-behaviors.md) | [[GR-START-FLEE]] |
| `CancelAIAction(agent)` / `ClearAIAction(agent)` | действие | остановка | отмена (задача остаётся) / отмена и удаление | [09](../chapters/09-execution-lifetime.md) | [[GR-CANCEL-ACTION]] |
| `CreateAIMoveToActionBinding(provider)` | binding | программный GOAP | MoveTo binding с query и debug | [09](../chapters/09-execution-lifetime.md) | [[GR-CREATE-BINDING]] |
| `RegisterGameplayTraversalLink` / `...Executor` | traversal | до создания instance | регистрация link/executor | [16](../chapters/16-traversal-physics.md) | [[TV-LINK-REG]] |
| `SetObstacleQuery(query)` | сервис | App при создании runtime | физический query для прямых запусков | [15](../chapters/15-obstacle-avoidance.md) | [[AP-SET-QUERY]] |

</div>

## Ключевые типы и методы

<div data-filter-table markdown>

| Тип | Домен | Метод | Вход → выход | Кто вызывает / фаза |
|---|---|---|---|---|
| [[cls:FindAIPlan]] | Ядро GOAP | функция | состояние, цель, действия → `optional<AIPlan>` | `AIDecisionRuntime::Update`, `Plan` |
| [[cls:SelectAIGoal]] | Ядро GOAP | функция | состояние, кандидаты → цель или «нет» | `AIDecisionRuntime::Update` |
| [[cls:AIDecisionRuntime]] | Исполнение плана | `Update(...)` | факты, цели, действия, bindings → статус | `GameplayGOAPDecision::Update` в `UpdateActiveAIDecisions_` |
| [[cls:AIPlanExecutionBridge]] | Исполнение плана | `StartReadyPlanStep`, `SynchronizeRunningPlanStep` | шаг → задача `AISystem` / статус | `AIDecisionRuntime` |
| [[cls:AISystem]] | Action runtime | `StartAction`, `Update`, `CancelAction` | runtime → задача; тик задач | решения; `PrePhysicsUpdate` |
| [[cls:IAIActionRuntime]] | Action runtime | `Start`, `Tick`, `Cancel` | контекст → `Running/Succeeded/Failed` | `AIActionTask` |
| [[cls:GameplayGOAPDecisionInstance]] | Решение (instance) | `Update(aiSystem, observation)` | мир + события → факты, план, события | `GameplayRuntime::UpdateActiveAIDecisions_` |
| [[cls:GameplayGOAPDecisionTemplate]] | Композиция | конструктор, `InstanceDefinition` | ассеты → снимок; overrides → определение | регистрация каталога; создание instance |
| [[cls:AIFollowRouteActionRuntime]] | Навигационные действия | `Tick` | позиция → команда / traversal | `AISystem::Update` |
| [[cls:GameplayRouteFollower]] | Маршруты | `Tick`, `CompleteTraversal` | позиция → намерение / статус | route runtime |
| [[cls:BuildGameplayArrivalSteering]] | Steering | функция | позиция, цель, r, R, m_min → намерение | follower, executors |
| [[cls:ApplyGameplayObstacleAvoidance]] | Obstacle avoidance | функция | намерение, query, состояние → намерение | агентный адаптер в runtimes |
| [[cls:JumpTraversalExecutor]] | Traversal | `Start/Tick/Cancel` | link → фазы прыжка | route runtime |
| [[cls:GameplayObjectReservationSystem]] | Ресурсы и события | `TryReserve/Release` | объект, агент → bool | MoveTo с бронью, Door, наблюдения |

</div>

## Порядок кадра

@@diagram d10-frame-sequence | Один тик App: где обновляются решения и задачи AI относительно ввода, физики и анимации.

## Контур решения

@@diagram d01-ai-loop | Замкнутый контур AI.

@@diagram d08-update-branches | Ветви `AIDecisionRuntime::Update`.

## Исполнение шага и traversal

@@diagram d09-action-lifecycle | Старт, тики и отмена шага.

@@diagram d16-traversal-handoff | Передача сегмента traversal-исполнителю.

## Маршруты

@@diagram d11-route-sources | Авторский граф и NavMesh сходятся в одном исполнителе.

## Значения по умолчанию

| Параметр | Значение | Источник |
|---|---|---|
| Arrival r / R / m_min | 0.25 / 1.0 / 0.1 | [[ST-ARRIVAL-SETTINGS]] |
| `move_to` acceptance / slowing (ассет) | 0.4 / 1.5 | [[CP-MOVE-ASSET]] |
| look-ahead L | 0.75 | [[RF-SETTINGS]] |
| Follow: acceptance / интервал | 0.25 / 0.05 с | [[NA-FOLLOW-SETTINGS]] |
| Flee: trigger / safe / интервал | 2 / 3 / 0.05 с | [[NA-FLEE-SETTINGS]] |
| Avoidance: forward / side / угол / смена стороны / margin | 3 / 2 / 30° / 0.15 / 0.02 | [[OA-SETTINGS]] |
| Support: вынос / подъём / падение | 0.75 / 0.25 / 0.35 | [[OA-SETTINGS]] |
| Мотор: ходьба / бег / разгон / торможение | 2.0 / 4.5 / 12 / 16 | [[CM-MOTOR]] |
| physicallyBlocked: желаемая / доля / время | 0.1 м/с / 10 % / 0.25 с | [[PH-BLOCKED]] |
| Jump тайм-аут | 10 с | [[TV-JUMP-TIMEOUT]] |
| Ёмкость фактов | 128 bool + 128 int32 | [[WS-CAPACITY]] |
| ID действий | FollowRoute 1, MoveTo 2, Purchase 3, FollowTarget 4, FleeTarget 5 | [[NA-ROUTE-ID]], [[NA-MOVETO-ID]], [[CP-PURCHASE-ID]] |
