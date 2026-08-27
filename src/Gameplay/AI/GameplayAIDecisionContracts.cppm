module;

#include <span>
#include <vector>

export module core:gameplay_ai_decision_contracts;

export import :ai_debug_view_model;
import :ai_plan_execution;
import :ai_system;
import :gameplay;
import :gameplay_world_event;

export namespace rendern
{
	struct GameplayAIObservationContext
    {
        const GameplayWorld& world;
        std::span<const GameplayWorldEvent> events;
	    std::vector<GameplayWorldEvent>* eventOutput{};
    };

    class GameplayAIDecisionInstance
    {
    public:
        virtual ~GameplayAIDecisionInstance() = default;
        virtual void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) = 0;
        virtual void Cancel(AISystem& aiSystem) noexcept = 0;
        [[nodiscard]] virtual AIPlanExecutionStatus GetStatus() const noexcept = 0;
        [[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;
        [[nodiscard]] virtual AIDebugViewModel BuildDebugViewModel() const = 0;
    };
}
