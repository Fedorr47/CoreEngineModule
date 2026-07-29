module;

export module core:gameplay_ai_components;

export import :ai_agent_world_state;

export namespace rendern
{
    struct AIComponent
    {
        AIAgentWorldState worldState{};
    };
}