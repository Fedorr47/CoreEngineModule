module;

#include <span>
#include <vector>

export module core:gameplay_ai_decision_contracts;

import :gameplay_world_event;

export namespace rendern
{
    class AISystem;
    class GameplayWorld;

    enum class GameplayAIDecisionStatus
    {
        NotStarted,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
    
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
        [[nodiscard]] virtual GameplayAIDecisionStatus GetStatus() const noexcept = 0;
    };
}
