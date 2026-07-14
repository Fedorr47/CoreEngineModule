module;

#include <cassert>
#include <memory>
#include <utility>

export module core:ai_action_task;

export import :ai_action_runtime;


export namespace rendern
{
    class AIActionTask
    {
    public:
        AIActionTask(
            AIActionRuntimeContext context,
            std::unique_ptr<IAIActionRuntime> runtime) noexcept
        : context_(context)
        , runtime_(std::move(runtime))
        {
            assert(context_.IsValid() && "AIActionTask requires a valid runtime context.");
            assert(runtime_ != nullptr && "AIActionTask requires an owned runtime");
        }
        
        AIActionTask(const AIActionTask&) = delete;
        AIActionTask& operator=(const AIActionTask&) = delete;
        AIActionTask(AIActionTask&&) noexcept = default;
        AIActionTask& operator=(AIActionTask&&) noexcept = default;
        
        [[nodiscard]] AIActionExecutionStatus GetStatus() const noexcept
        {
            return status_;
        }
        
        [[nodiscard]] const AIActionRuntimeContext& GetContext() const noexcept
        {
            return context_;
        }
        
        [[nodiscard]] bool IsRunning() const noexcept
        {
            return status_ == AIActionExecutionStatus::Running;
        }
        
        [[nodiscard]] bool IsTerminal() const noexcept
        {
            return IsTerminalStatus(status_);
        }
        
        [[nodiscard]] AIActionExecutionStatus Start()
        {
            if (status_ != AIActionExecutionStatus::NotStarted || runtime_ == nullptr)
            {
                return status_;
            }
            
            status_ = MapRuntimeResultToExecutionStatus(runtime_->Start(context_));
            return status_;
        }
        
        [[nodiscard]] AIActionExecutionStatus Tick(const float deltaSeconds)
        {
            if (status_ != AIActionExecutionStatus::Running || runtime_ == nullptr)
            {
                return status_;
            }
            
            status_ = MapRuntimeResultToExecutionStatus(
                runtime_->Tick(context_, deltaSeconds));
            return status_;
        }
        
        void Cancel() noexcept
        {
            if (status_ != AIActionExecutionStatus::Running || runtime_ == nullptr)
            {
                return;
            }
            
            runtime_->Cancel(context_);
            status_ = AIActionExecutionStatus::Cancelled;
        }
        
    private:
        
        [[nodiscard]] static constexpr  AIActionExecutionStatus MapRuntimeResultToExecutionStatus(
            AIActionRuntimeResult result) noexcept
        {
            switch (result)
            {
            case AIActionRuntimeResult::Running:
                return AIActionExecutionStatus::Running;
            case AIActionRuntimeResult::Succeeded:
                return AIActionExecutionStatus::Succeeded;
            case AIActionRuntimeResult::Failed:
                return AIActionExecutionStatus::Failed;
            default:
                return AIActionExecutionStatus::Failed;
            }
        }
        
        [[nodiscard]] static constexpr bool IsTerminalStatus(
            AIActionExecutionStatus status) noexcept
        {
            switch (status)
            {
                case AIActionExecutionStatus::NotStarted: [[fallthrough]];
                case AIActionExecutionStatus::Running:
                    return false;
                case AIActionExecutionStatus::Succeeded: [[fallthrough]];
                case AIActionExecutionStatus::Failed: [[fallthrough]];
                case AIActionExecutionStatus::Cancelled:
                    return true;
                default:
                    return true;
            }
        }
        
        AIActionRuntimeContext context_{};
        std::unique_ptr<IAIActionRuntime> runtime_{};
        AIActionExecutionStatus status_{ AIActionExecutionStatus::NotStarted };
    };
}
