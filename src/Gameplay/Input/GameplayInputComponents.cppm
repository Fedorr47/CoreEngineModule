module;

#include <cstdint>
#include <vector>

export module core:gameplay_input_components;

import :gameplay_action_components;

export namespace rendern
{
    struct GameplayPlayerControlledComponent
    {
        bool isPrimary{ true };
    };

    struct GameplayInputIntentComponent
    {
        float moveX{ 0.0f };
        float moveY{ 0.0f };
        bool runHeld{ false };
        std::vector<GameplayActionId> actionIntents{};
    };
}
