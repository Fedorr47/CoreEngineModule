module;

#include <memory>
#include <utility>

export module core:ai_plan_execution_bridge;

import :gameplay;
import :ai_system;
export import :ai_action_binding;
export import :ai_plan_execution;

export namespace rendern
{
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
            AIPlanExecution& execution,
            const AISystem& aiSystem,
            const EntityHandle agentEntity) noexcept
        {
            if (!execution.IsRunningStep())
            {
                return execution.GetStatus();
            }
            
            const AIPlanStep* step = execution.GetCurrentStep();
            if (step == nullptr)
            {
                return execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Failed);
            }
            
            AIActionExecutionStatus status = aiSystem.GetActionStatus(agentEntity, step->actionId);
            if (status == AIActionExecutionStatus::NotStarted)
            {
                status = AIActionExecutionStatus::Failed;
            }
            
            return execution.ApplyCurrentStepStatus(status);
        }
        
        static void CancelPlanExecution(
            AIPlanExecution& execution,
            AISystem& aiSystem,
            const EntityHandle agentEntity) noexcept
        {
            if (execution.IsRunningStep())
            {
                const AIPlanStep* step = execution.GetCurrentStep();
                if (step != nullptr && 
                    aiSystem.GetActionStatus(agentEntity, step->actionId) != AIActionExecutionStatus::NotStarted)
                {
                    aiSystem.CancelAction(agentEntity);
                }
            }
            execution.Cancel();
        }
    };
}