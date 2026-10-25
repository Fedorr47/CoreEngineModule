# AIPlanExecution

**class** · домен «Исполнение плана» · `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:25–138` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Конечный автомат исполнения плана по индексу шага.

## Архитектурный разбор

**Владелец и время жизни.** optional внутри AIDecisionRuntime; пересоздаётся при установке нового плана.

**Runtime-методы, входы и выходы.** Start, MarkCurrentStepStarted, MarkCurrentStepStartFailed, ApplyCurrentStepStatus, Cancel, GetCurrentStep.

**Кто вызывает и в какой фазе.** AIDecisionRuntime, AIPlanExecutionBridge.

**Ошибки, terminal states, отмена.** Невалидная цель/шаг в конструкторе → Failed; Cancel только из нетерминального состояния.

**Ссылки.** [теория: 08-decision-runtime](../../chapters/08-decision-runtime.md) · [схема d08-execution-states](../diagrams.md#d08-execution-states) · [PE-CLASS](../source-references.md#pe-class) · [PE-APPLY](../source-references.md#pe-apply) · тест `TestAIPlanExecution.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 28 | конструктор | public | `explicit AIPlanExecution(AIPlan plan) noexcept` |
| 29 | метод | public | `: plan_(std::move(plan))` |
| 41 | метод | public | `[[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept { return status_; }` |
| 42 | метод | public | `[[nodiscard]] const AIPlan& GetPlan() const noexcept { return plan_; }` |
| 43 | метод | public | `[[nodiscard]] std::size_t GetCurrentStepIndex() const noexcept { return currentStepIndex_; }` |
| 44 | метод | public | `[[nodiscard]] bool IsReadyToStartStep() const noexcept` |
| 48 | метод | public | `[[nodiscard]] bool IsRunningStep() const noexcept` |
| 52 | метод | public | `[[nodiscard]] bool IsTerminal() const noexcept` |
| 58 | метод | public | `[[nodiscard]] bool HasCurrentStep() const noexcept { return GetCurrentStep() != nullptr; }` |
| 59 | метод | public | `[[nodiscard]] const AIPlanStep* GetCurrentStep() const noexcept` |
| 68 | метод | public | `AIPlanExecutionStatus Start() noexcept` |
| 80 | метод | public | `AIPlanExecutionStatus MarkCurrentStepStarted() noexcept` |
| 89 | метод | public | `[[nodiscard]] AIPlanExecutionStatus MarkCurrentStepStartFailed() noexcept` |
| 98 | метод | public | `AIPlanExecutionStatus ApplyCurrentStepStatus(const AIActionExecutionStatus actionStatus) noexcept` |
| 126 | метод | public | `void Cancel() noexcept` |
| 135 | поле | private | `AIPlan plan_{};` |
| 136 | поле | private | `std::size_t currentStepIndex_{};` |
| 137 | поле | private | `AIPlanExecutionStatus status_{ AIPlanExecutionStatus::NotStarted };` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:25–85`.

```cpp linenums="25"
    class AIPlanExecution
    {
    public:
        explicit AIPlanExecution(AIPlan plan) noexcept
            : plan_(std::move(plan))
        {
            if (!plan_.goalId.IsValid()
                || !std::ranges::all_of(plan_.steps, [](const AIPlanStep& step)
                {
                    return step.actionId.IsValid();
                }))
            {
                status_ = AIPlanExecutionStatus::Failed;
            }
        }

        [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept { return status_; }
        [[nodiscard]] const AIPlan& GetPlan() const noexcept { return plan_; }
        [[nodiscard]] std::size_t GetCurrentStepIndex() const noexcept { return currentStepIndex_; }
        [[nodiscard]] bool IsReadyToStartStep() const noexcept
        {
            return status_ == AIPlanExecutionStatus::ReadyToStartStep;
        }
        [[nodiscard]] bool IsRunningStep() const noexcept
        {
            return status_ == AIPlanExecutionStatus::RunningStep;
        }
        [[nodiscard]] bool IsTerminal() const noexcept
        {
            return status_ == AIPlanExecutionStatus::Succeeded
                || status_ == AIPlanExecutionStatus::Failed
                || status_ == AIPlanExecutionStatus::Cancelled;
        }
        [[nodiscard]] bool HasCurrentStep() const noexcept { return GetCurrentStep() != nullptr; }
        [[nodiscard]] const AIPlanStep* GetCurrentStep() const noexcept
        {
            if ((!IsReadyToStartStep() && !IsRunningStep()) || currentStepIndex_ >= plan_.steps.size())
            {
                return nullptr;
            }
            return &plan_.steps[currentStepIndex_];
        }

        AIPlanExecutionStatus Start() noexcept
        {
            if (status_ != AIPlanExecutionStatus::NotStarted)
            {
                return status_;
            }
            status_ = plan_.steps.empty()
                ? AIPlanExecutionStatus::Succeeded
                : AIPlanExecutionStatus::ReadyToStartStep;
            return status_;
        }

        AIPlanExecutionStatus MarkCurrentStepStarted() noexcept
        {
            if (status_ == AIPlanExecutionStatus::ReadyToStartStep)
            {
                status_ = AIPlanExecutionStatus::RunningStep;
            }
```
