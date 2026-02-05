module;

#include <array>
#include <cstdint>

export module core:input;

export namespace rendern
{
    struct InputCapture
    {
        bool captureKeyboard{ false };
        bool captureMouse{ false };
    };

    struct MouseInput
    {
        // Relative look delta in pixels (usually only non-zero while RMB-look is active).
        int lookDx{ 0 };
        int lookDy{ 0 };

        // Mouse wheel steps since last frame (positive = wheel up, negative = wheel down).
        int wheelSteps{ 0 };

        // Useful for gameplay-style "hold RMB to look" logic.
        bool rmbDown{ false };
    };

    struct InputState
    {
        InputCapture capture{};
        bool hasFocus{ true };
        bool shiftDown{ false };

        std::array<std::uint8_t, 256> keyDown{};
        std::array<std::uint8_t, 256> keyPressed{};
        std::array<std::uint8_t, 256> keyReleased{};

        MouseInput mouse{};

        bool KeyDown(int vk) const noexcept
        {
            return (vk >= 0 && vk < 256) ? (keyDown[static_cast<std::uint8_t>(vk)] != 0) : false;
        }

        bool KeyPressed(int vk) const noexcept
        {
            return (vk >= 0 && vk < 256) ? (keyPressed[static_cast<std::uint8_t>(vk)] != 0) : false;
        }

        bool KeyReleased(int vk) const noexcept
        {
            return (vk >= 0 && vk < 256) ? (keyReleased[static_cast<std::uint8_t>(vk)] != 0) : false;
        }
    };
}
