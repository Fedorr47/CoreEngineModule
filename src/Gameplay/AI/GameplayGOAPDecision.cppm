module;

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>

export module core:gameplay_goap_decision;

export import :ai_debug_view_model;

import :ai_action_contracts;
import :ai_action_binding;
import :ai_system;
import :gameplay;

export namespace rendern
{
    struct GameplayGOAPDecisionDefinition
    {
        std::vector<AIGoalSelectionCandidate> goals{};
        std::vector<AIActionDefinition> actions{};
        AIDefinitionMetadata metadata{};
    };

    class GameplayGOAPDecision
    {
    public:
        GameplayGOAPDecision(const EntityHandle agent,
            GameplayGOAPDecisionDefinition definition)
            : decision_(agent), definition_(std::move(definition))
        {
        }

        [[nodiscard]] bool InstallActionBinding(const AIActionId actionId,
            std::unique_ptr<IAIActionBinding> binding)
        {
            if (!binding || !bindings_.Register(actionId, *binding))
            {
                return false;
            }
            ownedBindings_.push_back(std::move(binding));
            return true;
        }
        
        [[nodiscard]] bool HasCompleteActionBindings() const noexcept
        {
            for (const AIActionDefinition& action : definition_.actions)
            {
                if (action.actionId.IsValid() && !bindings_.Contains(action.actionId))
                {
                    return false;
                }
            }
            return true;
        }

        void Update(AISystem& aiSystem, const GameplayWorld& world)
        {
            (void)decision_.Update(facts_, definition_.goals, definition_.actions,
                bindings_, aiSystem, world);
        }

        void Cancel(AISystem& aiSystem) noexcept
        {
            decision_.Cancel(aiSystem);
        }

        [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept
        {
            return decision_.GetStatus();
        }

        [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept
        {
            return facts_;
        }

        [[nodiscard]] AIAgentWorldState& GetObservedState() noexcept
        {
            return facts_;
        }
        
        [[nodiscard]] AIDebugViewModel BuildDebugViewModel(
            const std::span<const AIWorldFactId> booleanFactIds,
            const std::span<const AIWorldIntegerFactId> integerFactIds) const
        {
            return BuildAIDebugViewModel(facts_, booleanFactIds, integerFactIds,
                definition_.actions, decision_);
        }
        
        [[nodiscard]] AIDebugViewModel BuildDebugViewModel() const
        {
            std::vector<AIWorldFactId> booleanIds;
            std::vector<AIWorldIntegerFactId> integerIds;
            for (const auto& fact : definition_.metadata.booleanFacts)
            {
                booleanIds.push_back(fact.id);
            }
            for (const auto& fact : definition_.metadata.integerFacts)
            {
                integerIds.push_back(fact.id);
            }
            AIDebugViewModel result = BuildDebugViewModel(booleanIds, integerIds);
            const auto& metadata = definition_.metadata;
            for (std::size_t index = 0; index < result.booleanFacts.size(); ++index)
            {
                result.booleanFacts[index].name = metadata.booleanFacts[index].name;
            }
            for (std::size_t index = 0; index < result.integerFacts.size(); ++index)
            {
                result.integerFacts[index].name = metadata.integerFacts[index].name;
            }
            if (result.selectedGoalId)
            {
                const auto goal = std::ranges::find_if(metadata.goals,
                    [&](const auto& value) { return value.id == *result.selectedGoalId; });
                if (goal != metadata.goals.end())
                {
                    result.selectedGoalName = goal->name;
                }
            }
            const auto nameAction = [&](auto& value)
            {
                const auto action = std::ranges::find_if(metadata.actions, [&](const auto& named)
                {
                    return named.actionId == value.actionId && named.contextId == value.contextId;
                });
                if (action != metadata.actions.end())
                {
                    value.actionName = action->actionName;
                    value.contextName = action->contextName;
                }
            };
            for (auto& step : result.selectedPlan)
            {
                nameAction(step);
            }
            for (auto& action : result.actionApplicability)
            {
                nameAction(action);
                for (auto& failure : action.failedBooleanConditions)
                {
                    const auto fact = std::ranges::find_if(metadata.booleanFacts,
                        [&](const auto& named) { return named.id == failure.factId; });
                    if (fact != metadata.booleanFacts.end())
                    {
                        failure.factName = fact->name;
                    }
                }
                for (auto& failure : action.failedNumericConditions)
                {
                    const auto fact = std::ranges::find_if(metadata.integerFacts,
                        [&](const auto& named) { return named.id == failure.factId; });
                    if (fact != metadata.integerFacts.end())
                    {
                        failure.factName = fact->name;
                    }
                }
            }
            return result;
        }

    private:
        AIDecisionRuntime decision_;
        const GameplayGOAPDecisionDefinition definition_;
        AIAgentWorldState facts_{};
        AIActionBindingRegistry bindings_{};
        std::vector<std::unique_ptr<IAIActionBinding>> ownedBindings_{};
    };
}