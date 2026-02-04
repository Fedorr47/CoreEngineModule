export module core:input;

import std;

export namespace input
{
    // Backend-agnostic input snapshot for one frame.
    // Platform backends (Win32/GLFW/...) fill this, controllers consume it.

    enum class Key : std::uint8_t
    {
        W,
        A,
        S,
        D,
        Q,
        E,
        Shift,
        Ctrl,
        Space,
        Count
    };

    enum class MouseButton : std::uint8_t
    {
        Left,
        Right,
        Middle,
        Count
    };

    struct UICapture
    {
        bool keyboard{ false };
        bool mouse{ false };
    };

    struct InputFrame
    {
        std::array<std::uint8_t, static_cast<std::size_t>(Key::Count)> keys{};
        std::array<std::uint8_t, static_cast<std::size_t>(MouseButton::Count)> mouseButtons{};

        // Mouse deltas in pixels since last frame (only meaningful when lookActive == true).
        float mouseDeltaX{ 0.0f };
        float mouseDeltaY{ 0.0f };

        // Mouse wheel notches for this frame (e.g. +1 / -1). 0 if none.
        float wheelSteps{ 0.0f };

        bool hasFocus{ true };
        bool allowGameInput{ true }; // already includes focus + UI capture filtering
        bool lookActive{ false };    // e.g. RMB-held look mode

        [[nodiscard]] bool KeyDown(Key k) const noexcept
        {
            return keys[static_cast<std::size_t>(k)] != 0;
        }

        [[nodiscard]] bool MouseDown(MouseButton b) const noexcept
        {
            return mouseButtons[static_cast<std::size_t>(b)] != 0;
        }
    };
}
