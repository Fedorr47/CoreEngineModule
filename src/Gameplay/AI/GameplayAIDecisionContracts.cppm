module;

export module core:gameplay_ai_decision_contracts;

import :ai_plan_execution;
import :ai_system;
import :gameplay;

export namespace rendern
{
    class GameplayAIDecisionInstance
    {
    public:
        virtual ~GameplayAIDecisionInstance() = default;
        virtual void Update(AISystem& aiSystem, const GameplayWorld& world) = 0;
        virtual void Cancel(AISystem& aiSystem) noexcept = 0;
        [[nodiscard]] virtual AIPlanExecutionStatus GetStatus() const noexcept = 0;
        [[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;
    };
}
