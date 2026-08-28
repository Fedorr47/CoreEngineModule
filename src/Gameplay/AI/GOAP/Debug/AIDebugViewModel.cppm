module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

export module core:ai_debug_view_model;

import :ai_action_contracts;
export import :ai_decision_runtime;

export namespace rendern
{
    struct AIDebugBooleanFactView
    {
        AIWorldFactId factId{};
        bool value{};
		std::string name{};
    };

    struct AIDebugIntegerFactView
    {
        AIWorldIntegerFactId factId{};
        std::int32_t value{};
		std::string name{};
    };

    struct AIDebugPlanStepView
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        float cost{};
        bool bCostResolved{};
		std::string actionName{};
        std::string contextName{};
    };

    struct AIDebugFailedBooleanConditionView
    {
        AIWorldFactId factId{};
        bool expected{};
        bool actual{};
		std::string factName{};
    };

    struct AIDebugFailedNumericConditionView
    {
        AIWorldIntegerFactId factId{};
        AINumericConditionOperator comparison{AINumericConditionOperator::Equal};
        std::int32_t expected{};
        std::int32_t actual{};
		std::string factName{};
    };

    struct AIDebugActionApplicabilityView
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        bool applicable{};
        std::vector<AIDebugFailedBooleanConditionView> failedBooleanConditions{};
        std::vector<AIDebugFailedNumericConditionView> failedNumericConditions{};
		std::string actionName{};
        std::string contextName{};
    };

    struct AIDebugViewModel
    {
        std::vector<AIDebugBooleanFactView> booleanFacts{};
        std::vector<AIDebugIntegerFactView> integerFacts{};
        std::optional<AIGoalId> selectedGoalId{};
		std::string selectedGoalName{};
        std::vector<AIDebugPlanStepView> selectedPlan{};
        float totalPlanCost{};
        bool bPlanCostComplete{true};
		AIPlanExecutionStatus decisionStatus{AIPlanExecutionStatus::NotStarted};
        std::optional<AIPlanExecutionStatus> executionStatus{};
        std::optional<std::size_t> currentStepIndex{};
        std::vector<AIDebugActionApplicabilityView> actionApplicability{};
    };

    [[nodiscard]] AIDebugViewModel BuildAIDebugViewModel(
        const AIAgentWorldState& observedState,
        const std::span<const AIWorldFactId> booleanFactIds,
        const std::span<const AIWorldIntegerFactId> integerFactIds,
        const std::span<const AIActionDefinition> actions,
        const AIPlanExecution* execution)
    {
        AIDebugViewModel result{};
        result.booleanFacts.reserve(booleanFactIds.size());
        for (const AIWorldFactId factId : booleanFactIds)
        {
            result.booleanFacts.push_back({factId, observedState.IsFactSet(factId)});
        }

        result.integerFacts.reserve(integerFactIds.size());
        for (const AIWorldIntegerFactId factId : integerFactIds)
        {
            result.integerFacts.push_back({factId, observedState.GetIntegerFact(factId)});
        }

        if (execution != nullptr)
        {
            const AIPlan& plan = execution->GetPlan();
            result.selectedGoalId = plan.goalId;
            result.executionStatus = execution->GetStatus();
            result.selectedPlan.reserve(plan.steps.size());
            for (const AIPlanStep& step : plan.steps)
            {
                const auto definition = std::ranges::find_if(actions,
                    [&](const AIActionDefinition& action)
                    {
                        return action.actionId == step.actionId
                            && action.contextId == step.contextId;
                    });
                const bool bCostResolved = definition != actions.end();
                const float cost = bCostResolved ? definition->baseCost : 0.0f;
                result.selectedPlan.push_back(
                    {step.actionId, step.contextId, cost, bCostResolved});
                result.totalPlanCost += cost;
                result.bPlanCostComplete &= bCostResolved;
            }
            if (execution->HasCurrentStep())
            {
                result.currentStepIndex = execution->GetCurrentStepIndex();
            }
        }

        result.actionApplicability.reserve(actions.size());
        for (const AIActionDefinition& action : actions)
        {
            AIDebugActionApplicabilityView applicability{
                .actionId = action.actionId,
                .contextId = action.contextId,
                .applicable = true};
            for (const AIFactCondition& condition : action.preconditions)
            {
                const bool actual = observedState.IsFactSet(condition.factId);
                if (!EvaluateFactCondition(observedState, condition))
                {
                    applicability.failedBooleanConditions.push_back(
                        {condition.factId, condition.bExpectedValue, actual});
                }
            }
            for (const AINumericCondition& condition : action.numericPreconditions)
            {
                const std::int32_t actual = observedState.GetIntegerFact(condition.factId);
                if (!EvaluateNumericCondition(actual, condition.comparison, condition.value))
                {
                    applicability.failedNumericConditions.push_back(
                        {condition.factId, condition.comparison, condition.value, actual});
                }
            }
            applicability.applicable = applicability.failedBooleanConditions.empty()
                && applicability.failedNumericConditions.empty();
            result.actionApplicability.push_back(std::move(applicability));
        }

        return result;
    }

    [[nodiscard]] AIDebugViewModel BuildAIDebugViewModel(
        const AIAgentWorldState& observedState,
        const std::span<const AIWorldFactId> booleanFactIds,
        const std::span<const AIWorldIntegerFactId> integerFactIds,
        const std::span<const AIActionDefinition> actions,
        const AIDecisionRuntime& runtime)
    {
        AIDebugViewModel result = BuildAIDebugViewModel(
            observedState, booleanFactIds, integerFactIds, actions,
            runtime.GetPlanExecution());
        result.selectedGoalId = runtime.GetSelectedGoalId();
        result.decisionStatus = runtime.GetStatus();
        return result;
    }
}