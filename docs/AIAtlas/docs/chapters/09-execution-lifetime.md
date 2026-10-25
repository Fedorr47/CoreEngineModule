# 09. Исполнение и lifetime: bridge, bindings, AISystem и владение

!!! abstract "Коротко"
    Семантический шаг плана `(actionId, contextId)` становится исполняемым через **binding**: он создаёт runtime
    (`IAIActionRuntime`), который `AISystem` помещает в `AIActionTask` — одну задачу на агента. Задачи стартуют
    синхронно и тикают в `aiSystem_.Update`. Instance решения владеет контекстом, провайдерами, bindings и буферами
    событий; bindings и runtimes **заимствуют** их ссылки, поэтому порядок уничтожения и порядок отмены — часть
    контракта. Статус: [[SV]].

## 1. Задача и интуиция

План говорит «`move_to` в контексте `start_to_coin_c`», но не знает, что это значит в мире: какой маршрут, какие
радиусы, нужна ли бронь. Нужен механизм позднего связывания «семантика → исполнитель» и объект, который живёт,
пока шаг выполняется, и умеет корректно остановиться. Эта глава объясняет цепочку объектов и правила их жизни.

## 2. Теоретическая выкладка

### 2.1. Разделение «что» и «как»: binding как фабрика

Шаг плана — **команда в символическом языке**. Binding — фабрика, переводящая команду и контекст агента в
конкретный объект исполнения:

$$
\text{binding}_{\text{actionId}} : (\text{agent}, \text{actionId}, \text{contextId}) \;\mapsto\; \text{runtime} \;|\; \varnothing .
$$

Один binding обслуживает **все контексты** своего семантического действия: `MoveToActionBinding` по `contextId`
находит маршрут конкретного контекста. Реестр bindings ключуется только `actionId` ([[AB-REGISTRY]]). Плюс подхода:
планировщику и исполнению не нужно знать о маршрутах, бронях и покупках; минус — ошибка связывания проявляется
только при старте шага (binding вернул `nullptr` → шаг `Failed`).

### 2.2. Интерфейс runtime как протокол

`IAIActionRuntime` задаёт протокол из трёх вызовов ([[AR-IFACE]]):

- `Start(context)` → `Running | Succeeded | Failed` — однократно;
- `Tick(context, dt)` → то же — только после `Running`;
- `Cancel(context)` — явная граница прерывания и очистки, **не применяет эффектов планировщика**.

Владелец протокола — `AIActionTask`: он гарантирует порядок (Tick не вызывается до успешного Start, Cancel — только
в `Running`) и хранит статус ([[AT-CLASS]]). Runtime может быть «мгновенным» (покупка: `Start` → `Succeeded`) или
«длительным» (движение: `Start` → `Running`, затем серия Tick).

### 2.3. Одна задача на агента

`AISystem` хранит `unordered_map<EntityHandle, AIActionTask>`: у агента **не более одной** задачи. `StartAction`
отменяет и удаляет прежнюю задачу, затем создаёт новую и сразу вызывает `Start` ([[AS-START]]). Terminal-задача
остаётся в карте, пока её не заменят или агент не перестанет быть AI-сущностью — именно поэтому решение может
прочитать `Succeeded` при следующей синхронизации ([[AS-CLEANUP]]).

### 2.4. Время жизни и заимствование

Правило безопасности ссылок: объект может хранить ссылку только на объект, который **гарантированно живёт
дольше**. В instance решения это обеспечено порядком полей: члены уничтожаются в обратном порядке объявления, а
`goap_` (с bindings) объявлен последним — значит, уничтожается первым, раньше контекста с провайдерами и буферов
событий ([[GG-FIELD-ORDER]]). Копирование и перемещение instance запрещены, чтобы адреса фактов и буферов, которые
bindings получили при создании, не менялись.

Runtimes в `AISystem` тоже заимствуют: покупка — факты и буфер событий instance, движение — мир, реестры traversal,
query. Поэтому владелец решения **обязан отменить задачу до уничтожения instance**. `GameplayRuntime` делает это во
всех путях: `CancelAIDecision`, `CancelAllAIDecisions_`, переход в Editor, `Shutdown` ([[GR-CANCEL-DECISION]],
[[GR-MODE-EDITOR]]).

## 3. Визуальное объяснение

@@diagram d09-ownership | Владение (ромб) и заимствование (пунктир). Instance уничтожает GOAP-обёртку с bindings раньше контекста с провайдерами; задачи `AISystem` заимствуют данные instance и должны быть отменены до его удаления.

@@diagram d09-action-lifecycle | Старт шага через реестр bindings и `AISystem`, тик в том же кадре, синхронизация статуса в следующем update и путь отмены.

@@diagram d09-task-states | Состояния задачи. Terminal-задача не удаляется сразу: её статус читает решение.

## 4. Как это реализовано в CoreEngine

### 4.1. Задача и система

@@code AS-UPDATE 51

Ветка `NotStarted → Start()` в `Update` защитная: `StartAction` уже вызвал `Start`, поэтому задача в карте всегда
имеет статус после старта [[INF]].

### 4.2. Binding MoveTo и передача сервисов

@@code NA-MOVETO-BINDING 81

| Вход в движение | obstacle query | steering debug | navigation debug | Источник |
|---|---|---|---|---|
| `move_to` из ассетной GOAP-композиции | **нет** (`nullptr`) | нет | нет | [[CP-MOVE-BINDING]] |
| `GameplayRuntime::CreateAIMoveToActionBinding` (программный) | да | да | да | [[GR-CREATE-BINDING]] |
| `StartAIFollowRoute`, `StartAIMoveTo` | да | да | да | [[GR-START-ROUTE]], [[GR-START-MOVETO]] |
| `StartAIFollowTarget`, `StartAIFleeTarget` | да | да | — | [[GR-START-FOLLOW]], [[GR-START-FLEE]] |

Причина первой строки — состав `GameplayAIDecisionCreationContext` ([[GD-CREATION]]): в нём нет obstacle query и
debug-реестров, и `MoveToCapability` создаёт binding без них. Общий `AIFollowRouteActionRuntime` умеет avoidance
(глава [15](15-obstacle-avoidance.md)), но ассетный GOAP-агент его не получает [[SV]].

### 4.3. Instance: порядок полей

@@code GG-FIELD-ORDER 9

## 5. Разобранный пример: жизнь шага `buy_key` и шага `move_to`

**`buy_key` (мгновенный).** Решение в `ReadyToStartStep` вызывает мост → реестр находит `PurchaseBinding` → он
по `contextId` выбирает транзакцию receipt `unlock` и создаёт `PurchaseRuntime`, заимствующий факты и
`runtimeEvents_` instance → `AISystem::StartAction` отменяет предыдущую (уже `Succeeded`) задачу движения, создаёт
новую и вызывает `Start` → покупка проверяет условия и пишет событие → `Succeeded` → мост сразу продвигает план
к следующему шагу (`ReadyToStartStep`). В том же update instance делает второй проход наблюдения и подтверждает
покупку (глава [07](07-observations-events.md)).

**`access_key_shop_to_goal` (длительный).** На следующем update шаг стартует: `MoveToCapability::ResolveRequest`
проверяет `sourceRadius = 0.6` (ближайший узел графа к агенту — `shop`, дистанция ≤ 0.6) и возвращает указатель на
граф (живёт в провайдере) и узлы → `AIMoveToAction::CreateRuntime` ищет путь (Дейкстра) и передаёт
**копию маршрута** во владение `AIFollowRouteActionRuntime` → `Start` публикует маршрут в navigation debug (здесь —
`nullptr`, публикации нет) и запускает follower → `Running`; `aiSystem_.Update` того же кадра делает первый `Tick`.

**Отмена.** Сценарий вызывает `cancelAIDecision`: `GameplayRuntime::CancelAIDecision` → `Cancel` решения →
`CancelPlanExecution` → `AISystem::CancelAction` → `AIActionTask::Cancel` → `AIFollowRouteActionRuntime::Cancel`
(отмена traversal, обнуление команды и горизонтальной скорости, очистка debug) → instance удаляется из карты.
Задача со статусом `Cancelled` остаётся в `AISystem` до следующего `StartAction` или `ClearAIAction`.

## 6. Типичные ошибки, границы и проверка

- **Два источника задач на одного агента.** Прямой `StartAI*` заменит задачу решения (и наоборот).
- **Уничтожить instance без отмены.** Runtime с заимствованными ссылками останется в `AISystem` — висячие ссылки.
  Используйте API `GameplayRuntime`, которые отменяют задачи.
- **Ожидать avoidance у ассетного GOAP.** Query не передаётся; агент полагается на физический контроллер и
  авторский граф.
- **Ожидать удаления terminal-задачи.** Она остаётся до замены; `GetAIActionStatus` вернёт её статус.
- **Graph-указатель запроса.** `AIMoveToActionRequest::routeGraph` невладеющий и действителен только во время
  создания runtime; маршрут runtime копирует.

Проверки: `TestAISystem.cpp` (13), `TestAIActionTask.cpp` (12), `TestAIActionBinding.cpp` (2),
`TestAIMoveToActionBinding.cpp` (10), `TestAIPlanExecutionBridge.cpp` (6) — наличие [[SV]].

Далее: [10. Порядок кадра](10-frame-order.md).
