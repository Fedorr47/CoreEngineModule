# Словарь

Термины в смысле этого атласа. Ссылка ведёт в главу с теорией.

<div data-filter-table markdown>

| Термин | Значение | Глава |
|---|---|---|
| Факт (fact) | типизированное bool/int значение модели знания агента; пишется только наблюдением | [03](../chapters/03-state-model.md) |
| Состояние (world state) | `AIAgentWorldState`: 128 bool + 128 int32; равенство и хеш по всем ячейкам | [03](../chapters/03-state-model.md) |
| Цель (goal) | требуемые bool-факты + оценка `score`; выбирается невыполненная с максимумом | [04](../chapters/04-goals-actions.md) |
| Семантическое действие | вид возможности (`move_to`, `purchase`), `AIActionId`; совпадает с типом capability | [06](../chapters/06-assets-composition.md) |
| Контекст | конкретизация действия в определении (`start_to_coin_c`), `AIActionContextId`; уникален в определении | [04](../chapters/04-goals-actions.md) |
| Предусловия (preconditions) | можно ли начать; проверяются в поиске и перед стартом шага | [04](../chapters/04-goals-actions.md) |
| Эффекты (effects) | прогноз после успеха; применяются только к копиям в поиске | [02](../chapters/02-goap-strips.md) |
| Условия продолжения (continuation) | что должно оставаться истинным во время шага; подмножество предусловий | [04](../chapters/04-goals-actions.md) |
| План | последовательность шагов (actionId, contextId) | [05](../chapters/05-planner.md) |
| Uniform-cost search | поиск по возрастанию накопленной стоимости без эвристики (алгоритм `FindAIPlan`) | [05](../chapters/05-planner.md) |
| Наблюдение (observation) | компонент, единственный writer своих фактов; объявляет входы | [07](../chapters/07-observations-events.md) |
| Latch | память факта: однажды истинный остаётся истинным | [07](../chapters/07-observations-events.md) |
| Реакция (reaction) | обработчик подтверждённых событий, создающий запросы мира | [07](../chapters/07-observations-events.md) |
| Capability / provider | компонент, связывающий контексты действия с исполнением и стоимостями | [06](../chapters/06-assets-composition.md) |
| Binding | фабрика runtime для шага плана по `actionId` | [09](../chapters/09-execution-lifetime.md) |
| Runtime действия | объект Start/Tick/Cancel, исполняющий шаг | [09](../chapters/09-execution-lifetime.md) |
| Задача (`AIActionTask`) | владеющая обёртка runtime со статусом; одна на агента в `AISystem` | [09](../chapters/09-execution-lifetime.md) |
| Template | неизменяемый снимок конфигурации решения без мира и агента | [06](../chapters/06-assets-composition.md) |
| Instance | решение одного агента: факты, наблюдения, providers, bindings, события | [06](../chapters/06-assets-composition.md) |
| Роль | имя → узел уровня; снимок позиции и сущности при создании instance | [06](../chapters/06-assets-composition.md) |
| Failed (решение) | нет плана или шаг упал; instance жив и может восстановиться | [08](../chapters/08-decision-runtime.md) |
| Explicit cancel | `AIDecisionRuntime::Cancel`; автопродолжения нет | [08](../chapters/08-decision-runtime.md) |
| Маршрут (route) | точки + аннотации сегментов (N ↔ N−1) | [11](../chapters/11-spatial-routes.md) |
| Route cost | стоимость кратчайшего пути, подставленная в стоимость `move_to` | [11](../chapters/11-spatial-routes.md) |
| Look-ahead | виртуальная цель на исходящем сегменте у угла | [12](../chapters/12-route-follower.md) |
| Плоскость перехода | плоскость через угол с нормалью-биссектрисой; критерий «угол пройден» | [12](../chapters/12-route-follower.md) |
| Steering | локальная команда: направление в XZ, magnitude [0,1], бег | [13](../chapters/13-steering.md) |
| Arrival | steering с замедлением между r и R и нижней границей m_min | [13](../chapters/13-steering.md) |
| Magnitude | доля максимальной скорости мотора, не сила и не скорость | [13](../chapters/13-steering.md) |
| Гистерезис | два порога и память состояния (Flee, выбор стороны обхода) | [14](../chapters/14-temporal-behaviors.md) |
| Feeler / probe | sweep сферы для оценки просвета | [15](../chapters/15-obstacle-avoidance.md) |
| Escape-кандидат | смесь feeler, касательной и нормали для обхода | [15](../chapters/15-obstacle-avoidance.md) |
| Support | проверка опоры под будущей позицией кандидата | [15](../chapters/15-obstacle-avoidance.md) |
| Traversal | исполнение аннотированного сегмента внешним executor (Jump, Door) | [16](../chapters/16-traversal-physics.md) |
| physicallyBlocked | признак отсутствия физического прогресса ≥ 0.25 с; только диагностика | [16](../chapters/16-traversal-physics.md) |
| Ledger | наблюдение-бухгалтер ресурса по подтверждённым событиям | [17](../chapters/17-resources-multiagent.md) |
| Receipt | идентификатор результата покупки у продавца | [17](../chapters/17-resources-multiagent.md) |
| Бронь (reservation) | необязательная блокировка InteractionPoint агентом | [17](../chapters/17-resources-multiagent.md) |
| Development-сценарий | воспроизводимый ручной эксперимент через production API | [18](../chapters/18-authoring-scenarios.md) |
| NavMesh-профиль | NavMesh, запечённый под размеры конкретного агента | [11](../chapters/11-spatial-routes.md) |

</div>
