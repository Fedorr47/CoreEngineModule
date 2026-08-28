module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <queue>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

export module core:ai_planner;

import :ai_action_contracts;
export import :ai_decision_contracts;

namespace rendern
{
    namespace
    {
        [[nodiscard]] bool IsFactIdValid(const AIWorldFactId factId) noexcept
        {
            return factId.index < AIAgentWorldState::FactCapacity;
        }
        
        [[nodiscard]] bool IsIntegerFactIdValid(
           const AIWorldIntegerFactId factId) noexcept
        {
            return factId.index < AIAgentWorldState::IntegerFactCapacity;
        }

        [[nodiscard]] bool IsNumericConditionOperatorValid(
            const AINumericConditionOperator comparison) noexcept
        {
            return comparison >= AINumericConditionOperator::Equal
                && comparison <= AINumericConditionOperator::GreaterOrEqual;
        }

        [[nodiscard]] bool IsNumericEffectOperationValid(
            const AINumericEffectOperation operation) noexcept
        {
            return operation >= AINumericEffectOperation::Set
                && operation <= AINumericEffectOperation::Add;
        }

        [[nodiscard]] bool IsInputValid(
            const AIGoalDefinition& goal,
            const std::span<const AIActionDefinition> actions)
        {
            if (!goal.goalId.IsValid()
                || !std::ranges::all_of(goal.desiredFacts, [](const AIFactCondition& condition)
                {
                    return IsFactIdValid(condition.factId);
                }))
            {
                return false;
            }

            std::vector<std::pair<AIActionId::ValueType, AIActionContextId::ValueType>> actionKeys;
            for (const AIActionDefinition& action : actions)
            {
                if (!action.actionId.IsValid()
                    || !std::isfinite(action.baseCost)
                    || action.baseCost < 0.0f
                    || !std::ranges::all_of(action.preconditions, [](const AIFactCondition& condition)
                    {
                        return IsFactIdValid(condition.factId);
                    })
                    || !std::ranges::all_of(action.effects, [](const AIFactEffect& effect)
                    {
                        return IsFactIdValid(effect.factId);
                    })
                    || !std::ranges::all_of(action.numericPreconditions, [](const AINumericCondition& condition)
                    {
                        return IsIntegerFactIdValid(condition.factId)
                            && IsNumericConditionOperatorValid(condition.comparison);
                    })
                    || !std::ranges::all_of(action.numericEffects, [](const AINumericEffect& effect)
                    {
                        return IsIntegerFactIdValid(effect.factId)
                            && IsNumericEffectOperationValid(effect.operation);
                    }))
                {
                    return false;
                }
                actionKeys.emplace_back(action.actionId.value, action.contextId.value);
            }

            std::ranges::sort(actionKeys);
            return std::ranges::adjacent_find(actionKeys) == actionKeys.end();
        }

        struct SearchNode
        {
            AIAgentWorldState state{};
            float cost{};
            std::uint64_t sequence{};
            std::vector<AIPlanStep> steps{};
        };

        struct SearchNodeGreater
        {
            [[nodiscard]] bool operator()(const SearchNode& left, const SearchNode& right) const noexcept
            {
                if (left.cost != right.cost)
                {
                    return left.cost > right.cost;
                }
                return left.sequence > right.sequence;
            }
        };
    }
}

export namespace rendern
{
    [[nodiscard]] std::optional<AIPlan> FindAIPlan(
        const AIAgentWorldState& initialState,
        const AIGoalDefinition& goal,
        const std::span<const AIActionDefinition> actions)
    {
        if (!IsInputValid(goal, actions))
        {
            return std::nullopt;
        }
        if (AreFactConditionsSatisfied(initialState, goal.desiredFacts))
        {
            return AIPlan{ goal.goalId, {} };
        }

        std::vector<const AIActionDefinition*> orderedActions;
        orderedActions.reserve(actions.size());
        for (const AIActionDefinition& action : actions)
        {
            orderedActions.push_back(&action);
        }
        std::ranges::sort(orderedActions, {}, [](const AIActionDefinition* action)
        {
            return std::pair{action->actionId.value, action->contextId.value};
        });

        std::priority_queue<SearchNode, std::vector<SearchNode>, SearchNodeGreater> frontier;
        std::unordered_map<AIAgentWorldState, float, AIAgentWorldStateHash> bestCosts;
        std::uint64_t nextSequence = 0u;
        bestCosts.emplace(initialState, 0.0f);
        frontier.push(SearchNode{ initialState, 0.0f, nextSequence++, {} });

        while (!frontier.empty())
        {
            SearchNode current = frontier.top();
            frontier.pop();
            const auto currentBest = bestCosts.find(current.state);
            if (currentBest == bestCosts.end() || current.cost != currentBest->second)
            {
                continue;
            }
            if (AreFactConditionsSatisfied(current.state, goal.desiredFacts))
            {
                return AIPlan{ goal.goalId, std::move(current.steps) };
            }

            for (const AIActionDefinition* action : orderedActions)
            {
                if (!AreFactConditionsSatisfied(current.state, action->preconditions))
                {
                    continue;
                }
                if (!AreNumericConditionsSatisfied(current.state, action->numericPreconditions))
                {
                    continue;
                }

                AIAgentWorldState nextState = current.state;
                ApplyFactEffects(nextState, action->effects);
                if (!ApplyNumericEffects(nextState, action->numericEffects))
                {
                    continue;
                }
                const float nextCost = current.cost + action->baseCost;
                if (!std::isfinite(nextCost))
                {
                    continue;
                }

                const auto known = bestCosts.find(nextState);
                if (known != bestCosts.end() && known->second <= nextCost)
                {
                    continue;
                }

                std::vector<AIPlanStep> nextSteps = current.steps;
                nextSteps.push_back(AIPlanStep{action->actionId, action->contextId });
                bestCosts.insert_or_assign(nextState, nextCost);
                frontier.push(SearchNode{
                    std::move(nextState), nextCost, nextSequence++, std::move(nextSteps) });
            }
        }

        return std::nullopt;
    }
}