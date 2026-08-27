module;

#include <memory>
#include <utility>
#include <vector>

export module core:gameplay_goap_decision;

export import :ai_debug_view_model;
import :ai_action_binding;
import :ai_system;
import :gameplay;

export namespace rendern
{
    struct GameplayGOAPDecisionDefinition
    {
        std::vector<AIGoalSelectionCandidate> goals{};
        std::vector<AIActionDefinition> actions{};
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

    private:
        AIDecisionRuntime decision_;
        const GameplayGOAPDecisionDefinition definition_;
        AIAgentWorldState facts_{};
        AIActionBindingRegistry bindings_{};
        std::vector<std::unique_ptr<IAIActionBinding>> ownedBindings_{};
    };
}