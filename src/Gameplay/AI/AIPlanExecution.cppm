module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

export module core:ai_plan_execution;

export import :ai_decision_contracts;

export namespace rendern
{
    enum class AIPlanExecutionStatus : std::uint8_t
    {
        NotStarted,
        ReadyToStartStep,
        RunningStep,
        Succeeded,
        Failed,
        Cancelled
    };

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
            return status_;
        }

        [[nodiscard]] AIPlanExecutionStatus MarkCurrentStepStartFailed() noexcept
        {
            if (status_ == AIPlanExecutionStatus::ReadyToStartStep)
            {
                status_ = AIPlanExecutionStatus::Failed;
            }
            return status_;
        }

        AIPlanExecutionStatus ApplyCurrentStepStatus(const AIActionExecutionStatus actionStatus) noexcept
        {
            if (status_ != AIPlanExecutionStatus::RunningStep)
            {
                return status_;
            }

            switch (actionStatus)
            {
            case AIActionExecutionStatus::Succeeded:
                ++currentStepIndex_;
                status_ = currentStepIndex_ < plan_.steps.size()
                    ? AIPlanExecutionStatus::ReadyToStartStep
                    : AIPlanExecutionStatus::Succeeded;
                break;
            case AIActionExecutionStatus::Failed:
                status_ = AIPlanExecutionStatus::Failed;
                break;
            case AIActionExecutionStatus::Cancelled:
                status_ = AIPlanExecutionStatus::Cancelled;
                break;
            case AIActionExecutionStatus::NotStarted: [[fallthrough]];
            case AIActionExecutionStatus::Running:
                break;
            }
            return status_;
        }

        void Cancel() noexcept
        {
            if (!IsTerminal())
            {
                status_ = AIPlanExecutionStatus::Cancelled;
            }
        }

    private:
        AIPlan plan_{};
        std::size_t currentStepIndex_{};
        AIPlanExecutionStatus status_{ AIPlanExecutionStatus::NotStarted };
    };
}