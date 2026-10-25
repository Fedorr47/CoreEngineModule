# AIPlanExecutionBridge

**class** · домен «Исполнение плана» · `src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:16–117` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Мост от семантического шага к задаче AISystem: binding → runtime → StartAction; синхронизация и отмена.

## Архитектурный разбор

**Runtime-методы, входы и выходы.** StartReadyPlanStep (мгновенный успех сразу продвигает план), SynchronizeRunningPlanStep (NotStarted трактуется как Failed), CancelPlanExecution.

**Кто вызывает и в какой фазе.** AIDecisionRuntime.

**Что вызывает сама сущность.** AIActionBindingRegistry::Find, IAIActionBinding::CreateRuntime, AISystem::StartAction/GetActionStatus/CancelAction.

**Ошибки, terminal states, отмена.** Нет binding/runtime или неуспешный старт → шаг Failed.

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [схема d09-action-lifecycle](../diagrams.md#d09-action-lifecycle) · [BR-START](../source-references.md#br-start) · [BR-SYNC](../source-references.md#br-sync) · [BR-CANCEL](../source-references.md#br-cancel) · тест `TestAIPlanExecutionBridge.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 19 | метод | public | `[[nodiscard]] static AIPlanExecutionStatus StartReadyPlanStep(` |
| 76 | метод | public | `[[nodiscard]] static AIPlanExecutionStatus SynchronizeRunningPlanStep(` |
| 101 | метод | public | `static void CancelPlanExecution(` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanExecutionBridge.cppm:16–76`.

```cpp linenums="16"
    class AIPlanExecutionBridge
    {
    public:
        [[nodiscard]] static AIPlanExecutionStatus StartReadyPlanStep(
            AIPlanExecution& execution,
            const AIActionBindingRegistry& bindings,
            AISystem& aiSystem,
            const GameplayWorld& world,
            const EntityHandle agentEntity)
        {
            if (!execution.IsReadyToStartStep())
            {
                return execution.GetStatus();
            }

            const AIPlanStep* step = execution.GetCurrentStep();
            if (step == nullptr)
            {
                return execution.MarkCurrentStepStartFailed();
            }

            const AIActionRuntimeContext context{
                .agentEntity = agentEntity,
                .actionId = step->actionId,
                .contextId = step->contextId};
            if (!context.IsValid())
            {
                return execution.MarkCurrentStepStartFailed();
            }

           IAIActionBinding* binding = bindings.Find(step->actionId);
           if (binding == nullptr)
           {
               return execution.MarkCurrentStepStartFailed();
           }

            std::unique_ptr<IAIActionRuntime> runtime = binding->CreateRuntime(context);
            if (runtime == nullptr)
            {
                return execution.MarkCurrentStepStartFailed();
            }

            const AIActionExecutionStatus actionStatus =
                aiSystem.StartAction(world, context, std::move(runtime));
            if (actionStatus == AIActionExecutionStatus::Failed
                || actionStatus == AIActionExecutionStatus::NotStarted
                || actionStatus == AIActionExecutionStatus::Cancelled)
            {
                return execution.MarkCurrentStepStartFailed();
            }

            execution.MarkCurrentStepStarted();
            if (actionStatus == AIActionExecutionStatus::Succeeded)
            {
                return execution.ApplyCurrentStepStatus(actionStatus);
            }

            return execution.GetStatus();
        }

        [[nodiscard]] static AIPlanExecutionStatus SynchronizeRunningPlanStep(
```
