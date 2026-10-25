# AIDecisionRuntime

**class** · домен «Исполнение плана» · `src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:22–311` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Мониторинговый контур одного агента: синхронизация шага, выбор цели, проверка плана, перепланирование, старт шагов.

## Архитектурный разбор

**Владелец и время жизни.** Член GameplayGOAPDecision (по значению); владеет optional<AIPlanExecution>. Факты — у вызывающего.

**Основные данные: readers / writers.** execution_, selectedGoalId_, status_, bExplicitlyCancelled_.

**Runtime-методы, входы и выходы.** Update(facts, goals, actions, bindings, aiSystem, world) → статус; Update(bindings, ...) — старт/синхронизация шага; Plan(...); Cancel(aiSystem).

**Кто вызывает и в какой фазе.** GameplayGOAPDecision::Update из GameplayGOAPDecisionInstance::Update (PrePhysicsUpdate, UpdateActiveAIDecisions_); GameplayRuntime::UpdateAIDecision (программно).

**Что вызывает сама сущность.** SelectAIGoal, FindAIPlan, AIPlanExecutionBridge::{StartReadyPlanStep, SynchronizeRunningPlanStep, CancelPlanExecution}.

**Ошибки, terminal states, отмена.** Нет плана → Failed (instance жив); explicit Cancel блокирует автопродолжение до успешного Plan().

**Ссылки.** [теория: 08-decision-runtime](../../chapters/08-decision-runtime.md) · [схема d08-update-branches](../diagrams.md#d08-update-branches) · [схема d08-execution-states](../diagrams.md#d08-execution-states) · [схема d08-recovery-sequence](../diagrams.md#d08-recovery-sequence) · [DR-UPDATE](../source-references.md#dr-update) · [DR-INVALIDATED](../source-references.md#dr-invalidated) · [DR-KEEP-RUNNING](../source-references.md#dr-keep-running) · [DR-CANCEL](../source-references.md#dr-cancel) · тест `TestAIDecisionRuntime.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 25 | конструктор | public | `explicit AIDecisionRuntime(const EntityHandle agentEntity) noexcept` |
| 26 | метод | public | `: agentEntity_(agentEntity)` |
| 30 | метод | public | `[[nodiscard]] bool Plan(` |
| 54 | метод | public | `[[nodiscard]] AIPlanExecutionStatus Update(` |
| 144 | метод | public | `[[nodiscard]] AIPlanExecutionStatus Update(` |
| 168 | метод | public | `void Cancel(AISystem& aiSystem) noexcept` |
| 183 | метод | public | `[[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept` |
| 188 | метод | public | `[[nodiscard]] EntityHandle GetAgentEntity() const noexcept` |
| 193 | метод | public | `[[nodiscard]] const AIPlanExecution* GetPlanExecution() const noexcept` |
| 198 | метод | public | `[[nodiscard]] std::optional<AIGoalId> GetSelectedGoalId() const noexcept` |
| 204 | метод | private | `[[nodiscard]] static const AIGoalDefinition* FindGoalDefinition_(` |
| 220 | метод | private | `[[nodiscard]] bool HasValidExecution_() const noexcept` |
| 225 | метод | private | `[[nodiscard]] bool IsRunningStepContinuationValid_(` |
| 252 | метод | private | `[[nodiscard]] bool IsRemainingPlanValid_(` |
| 291 | метод | private | `void InstallPlan_(AIPlan plan, AISystem& aiSystem)` |
| 298 | метод | private | `void CancelActiveExecution_(AISystem& aiSystem) noexcept` |
| 306 | поле | private | `EntityHandle agentEntity_{};` |
| 307 | поле | private | `std::optional<AIPlanExecution> execution_{};` |
| 308 | поле | private | `std::optional<AIGoalId> selectedGoalId_{};` |
| 309 | поле | private | `AIPlanExecutionStatus status_{AIPlanExecutionStatus::NotStarted};` |
| 310 | поле | private | `bool bExplicitlyCancelled_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionRuntime.cppm:22–82`.

```cpp linenums="22"
    class AIDecisionRuntime
    {
    public:
        explicit AIDecisionRuntime(const EntityHandle agentEntity) noexcept
            : agentEntity_(agentEntity)
        {
        }

        [[nodiscard]] bool Plan(
            const AIAgentWorldState& observedState,
            const AIGoalDefinition& goal,
            const std::span<const AIActionDefinition> actions,
            AISystem& aiSystem)
        {
            std::optional<AIPlan> plan = FindAIPlan(observedState, goal, actions);
            if (!plan.has_value())
            {
                if (!HasValidExecution_())
                {
                    execution_.reset();
                    selectedGoalId_.reset();
                    status_ = AIPlanExecutionStatus::Failed;
                }
                return false;
            }

            InstallPlan_(std::move(*plan), aiSystem);
            selectedGoalId_ = goal.goalId;
            bExplicitlyCancelled_ = false;
            return status_ != AIPlanExecutionStatus::Failed;
        }

        [[nodiscard]] AIPlanExecutionStatus Update(
            const AIAgentWorldState& observedState,
            const std::span<const AIGoalSelectionCandidate> candidates,
            const std::span<const AIActionDefinition> actions,
            const AIActionBindingRegistry& bindings,
            AISystem& aiSystem,
            const GameplayWorld& world)
        {
            if (bExplicitlyCancelled_)
            {
                return status_;
            }

            const bool bSynchronizedRunningExecution = execution_.has_value() && execution_->IsRunningStep();
            if (bSynchronizedRunningExecution)
            {
                status_ = AIPlanExecutionBridge::SynchronizeRunningPlanStep(*execution_, aiSystem, agentEntity_);
            }

            const AIGoalSelectionResult selection = SelectAIGoal(observedState, candidates);
            const AIGoalDefinition* selectedGoal = FindGoalDefinition_(selection, candidates);
            if (selectedGoal == nullptr)
            {
                CancelActiveExecution_(aiSystem);
                execution_.reset();
                selectedGoalId_.reset();
                status_ = AIPlanExecutionStatus::Succeeded;
                return status_;
            }
```
