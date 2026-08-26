module;

#include <algorithm>
#include <optional>
#include <span>
#include <utility>

export module core:ai_decision_runtime;

import :gameplay;
import :ai_system;
export import :ai_goal_selection;
export import :ai_planner;
export import :ai_plan_execution_bridge;

export namespace rendern
{
    // Coordinates one agent's semantic GOAP plan. Concrete action runtimes remain
    // owned and ticked by AISystem; observed world facts remain caller-owned.
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

            const bool bGoalChanged = !selectedGoalId_.has_value() || *selectedGoalId_ != selectedGoal->goalId;
            if (bSynchronizedRunningExecution
                && status_ == AIPlanExecutionStatus::Succeeded
                && !bGoalChanged)
            {
                return status_;
            }

            const bool bPlanInvalidated = !bGoalChanged
                && execution_.has_value()
                && execution_->IsReadyToStartStep()
                && !IsRemainingPlanValid_(observedState, *selectedGoal, actions);

            if (!bGoalChanged && !bPlanInvalidated && HasValidExecution_())
            {
                if (bSynchronizedRunningExecution)
                {
                    return status_;
                }
                return Update(bindings, aiSystem, world);
            }

            const bool bNeedsPlan = bGoalChanged
                || bPlanInvalidated
                || !execution_.has_value()
                || execution_->GetStatus() == AIPlanExecutionStatus::Failed
                || execution_->GetStatus() == AIPlanExecutionStatus::Succeeded;
            if (bNeedsPlan)
            {
                std::optional<AIPlan> candidatePlan = FindAIPlan(observedState, *selectedGoal, actions);
                if (!candidatePlan.has_value())
                {
                    // A goal change must not destroy an otherwise valid running plan.
                    if (!bPlanInvalidated && HasValidExecution_())
                    {
                        if (bSynchronizedRunningExecution)
                        {
                            return status_;
                        }

                        return Update(bindings, aiSystem, world);
                    }
                    
                    CancelActiveExecution_(aiSystem);
                    execution_.reset();
                    selectedGoalId_ = selectedGoal->goalId;
                    status_ = AIPlanExecutionStatus::Failed;
                    return status_;
                }

                InstallPlan_(std::move(*candidatePlan), aiSystem);
                selectedGoalId_ = selectedGoal->goalId;
                return status_;
            }

            return Update(bindings, aiSystem, world);
        }

        [[nodiscard]] AIPlanExecutionStatus Update(
            const AIActionBindingRegistry& bindings,
            AISystem& aiSystem,
            const GameplayWorld& world)
        {
            if (!execution_.has_value() || execution_->IsTerminal())
            {
                return status_;
            }

            if (execution_->IsRunningStep())
            {
                status_ = AIPlanExecutionBridge::SynchronizeRunningPlanStep(*execution_, aiSystem, agentEntity_);
                return status_;
            }

            if (execution_->IsReadyToStartStep())
            {
                status_ = AIPlanExecutionBridge::StartReadyPlanStep(
                    *execution_, bindings, aiSystem, world, agentEntity_);
            }
            return status_;
        }

        void Cancel(AISystem& aiSystem) noexcept
        {
            bExplicitlyCancelled_ = true;
            if (execution_.has_value())
            {
                AIPlanExecutionBridge::CancelPlanExecution(*execution_, aiSystem, agentEntity_);
                status_ = execution_->GetStatus();
                return;
            }
            if (status_ != AIPlanExecutionStatus::Succeeded && status_ != AIPlanExecutionStatus::Failed)
            {
                status_ = AIPlanExecutionStatus::Cancelled;
            }
        }

        [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept
        {
            return status_;
        }
        
        [[nodiscard]] EntityHandle GetAgentEntity() const noexcept
        {
            return agentEntity_;
        }
        
        [[nodiscard]] const AIPlanExecution* GetPlanExecution() const noexcept
        {
            return execution_ ? &*execution_ : nullptr;
        }

    private:
        [[nodiscard]] static const AIGoalDefinition* FindGoalDefinition_(
            const AIGoalSelectionResult selection,
            const std::span<const AIGoalSelectionCandidate> candidates) noexcept
        {
            if (!selection.IsValid())
            {
                return nullptr;
            }
            const auto candidate = std::ranges::find(
                candidates, selection.goalId, [](const AIGoalSelectionCandidate& value)
                {
                    return value.goal.goalId;
                });
            return candidate == candidates.end() ? nullptr : &candidate->goal;
        }

        [[nodiscard]] bool HasValidExecution_() const noexcept
        {
            return execution_.has_value() && !execution_->IsTerminal();
        }
        
        [[nodiscard]] bool IsRemainingPlanValid_(
            const AIAgentWorldState& observedState,
            const AIGoalDefinition& goal,
            const std::span<const AIActionDefinition> actions) const
        {
            if (!execution_.has_value())
            {
                return false;
            }

            AIAgentWorldState predicted = observedState;
            const AIPlan& plan = execution_->GetPlan();
            for (std::size_t index = execution_->GetCurrentStepIndex();
                index < plan.steps.size(); ++index)
            {
                const AIPlanStep& step = plan.steps[index];
                const auto definition = std::ranges::find_if(actions,
                    [&](const AIActionDefinition& action)
                    {
                        return action.actionId == step.actionId
                            && action.contextId == step.contextId;
                    });
                if (definition == actions.end()
                    || !AreFactConditionsSatisfied(predicted, definition->preconditions)
                    || !AreNumericConditionsSatisfied(predicted, definition->numericPreconditions))
                {
                    return false;
                }

                ApplyFactEffects(predicted, definition->effects);
                if (!ApplyNumericEffects(predicted, definition->numericEffects))
                {
                    return false;
                }
            }

            return AreFactConditionsSatisfied(predicted, goal.desiredFacts);
        }

        void InstallPlan_(AIPlan plan, AISystem& aiSystem)
        {
            CancelActiveExecution_(aiSystem);
            execution_.emplace(std::move(plan));
            status_ = execution_->Start();
        }

        void CancelActiveExecution_(AISystem& aiSystem) noexcept
        {
            if (execution_.has_value() && !execution_->IsTerminal())
            {
                AIPlanExecutionBridge::CancelPlanExecution(*execution_, aiSystem, agentEntity_);
            }
        }

        EntityHandle agentEntity_{};
        std::optional<AIPlanExecution> execution_{};
        std::optional<AIGoalId> selectedGoalId_{};
        AIPlanExecutionStatus status_{AIPlanExecutionStatus::NotStarted};
        bool bExplicitlyCancelled_{};
    };
}