module;

#include <cstdint>
#include <vector>

export module core:gameplay_input_bindings;

import :gameplay_action_components;

export namespace rendern
{
    inline constexpr int kGameplayMouseLeft = 0x01;
    inline constexpr int kGameplayMouseRight = 0x02;
    inline constexpr int kGameplayMouseMiddle = 0x04;
    inline constexpr int kGameplayMouseX1 = 0x05;
    inline constexpr int kGameplayMouseX2 = 0x06;

    struct GameplayAxisKeyBinding { int negativeKey{ 0 }; int positiveKey{ 0 }; };
    struct GameplayButtonKeyBinding { int key{ 0 }; };
    struct GameplayActionKeyBinding { int key{ 0 }; GameplayActionId action{}; };

    [[nodiscard]] constexpr bool IsGameplayMouseButton(const int key) noexcept
    {
        return key == kGameplayMouseLeft || key == kGameplayMouseRight ||
            key == kGameplayMouseMiddle || key == kGameplayMouseX1 || key == kGameplayMouseX2;
    }

    [[nodiscard]] constexpr bool IsSupportedGameplayKeyboardKey(const int key) noexcept
    {
        return (key >= 'A' && key <= 'Z') ||
            (key >= '0' && key <= '9') || (key >= 0x70 && key <= 0x7B) ||
            key == 0x20 || key == 0x10 || key == 0x11 || key == 0x0D ||
            key == 0x1B || key == 0x09 || key == 0x2E;
    }

    [[nodiscard]] constexpr bool IsSupportedGameplayActionInput(const int key) noexcept
    {
        return IsSupportedGameplayKeyboardKey(key) || IsGameplayMouseButton(key);
    }

    [[nodiscard]] constexpr bool IsGameplayActionBindingKeyReserved(const int key) noexcept
    {
        // F5-F7 are application hotkeys; RMB owns relative camera look.
        return key == 0x74 || key == 0x75 || key == 0x76 || key == kGameplayMouseRight;
    }

    struct GameplayKeyboardMouseBindings
    {
        GameplayAxisKeyBinding moveX{ 'D', 'A' };
        GameplayAxisKeyBinding moveY{ 'S', 'W' };
        GameplayButtonKeyBinding run{ 0x10 };
        std::vector<GameplayActionKeyBinding> actions{
            { 0x20, kGameplayActionJump },
            { 'E', kGameplayActionInteract }
        };
    };
}