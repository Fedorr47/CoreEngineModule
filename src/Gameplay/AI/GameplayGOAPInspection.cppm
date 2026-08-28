module;

export module core:gameplay_goap_inspection;

export import :ai_agent_world_state;
export import :ai_debug_view_model;
export import :ai_plan_execution;

export namespace rendern
{
    class IGameplayGOAPInspection
    {
    public:
        virtual ~IGameplayGOAPInspection() = default;

        [[nodiscard]] virtual AIPlanExecutionStatus GetGOAPStatus() const noexcept = 0;
        [[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;
        [[nodiscard]] virtual AIDebugViewModel BuildDebugViewModel() const = 0;
    };
}