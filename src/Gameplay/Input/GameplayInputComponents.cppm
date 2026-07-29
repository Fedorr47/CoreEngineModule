module;

#include <cstdint>

export module core:gameplay_input_components;

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
        std::uint32_t actionIntentMask{ 0u };
    };
}
