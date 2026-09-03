module;

#include <functional>
#include <memory>
#include <span>
#include <vector>

export module core:gameplay_goap_decision_setup;

export import :gameplay_goap_definition_contracts;
import :ai_action_binding;
import :gameplay_ai_decision_contracts;

export namespace rendern
{
    // Owns domain observation and the resources referenced by action bindings.
    // It has no planner, executor, decision lifecycle or definition discovery.
    class IGameplayGOAPContext
    {
    public:
        virtual ~IGameplayGOAPContext() = default;

        virtual void Observe(const GameplayWorld& world,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) = 0;

        // Domains with synchronous action events may confirm them in this update.
        virtual void ObserveActionEvents(const GameplayWorld&,
            std::span<const GameplayWorldEvent>, AIAgentWorldState&)
        {
        }
    };

    struct GameplayGOAPActionBindingSetup
    {
        AIActionId actionId{};
        // Called once after context, facts and the event buffer have stable addresses.
        // Captures may borrow from setup.context, but not from the temporary setup.
        std::function<std::unique_ptr<IAIActionBinding>(
            AIAgentWorldState&, std::vector<GameplayWorldEvent>&)> create{};
    };

    struct GameplayGOAPDecisionSetup
    {
        GameplayGOAPDecisionDefinition definition{};
        std::unique_ptr<IGameplayGOAPContext> context{};
        std::vector<GameplayGOAPActionBindingSetup> actionBindings{};
    };
}
