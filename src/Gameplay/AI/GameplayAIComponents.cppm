module;

export module core:gameplay_ai_components;

export import :ai_decision_contracts;

export namespace rendern
{
    struct AIComponent
    {
        AIAgentWorldState worldState{};
        AIGoalId selectedGoalId{};
    };
}