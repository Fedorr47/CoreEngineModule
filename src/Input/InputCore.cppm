module;

#include <array>
#include <cstdint>
#include <algorithm>

export module core:input_core;

import :input;

export namespace rendern
{
    // Pure, testable input state builder.
    // Platform layer provides "current" key states + per-frame mouse deltas and wheel delta units.
    class InputCore
    {
    public:
        InputCore() = default;

        void Reset() noexcept
        {
            state_ = {};
            prevKeyDown_ = {};
            wheelRemainderUnits_ = 0;
        }

        const InputState& State() const noexcept { return state_; }

        // wheelDeltaUnits: in legacy Windows units (usually 120 per notch). Can be any integer.
        // We intentionally keep remainder so high-resolution wheels are not lost.
        void NewFrame(
            InputCapture capture,
            bool hasFocus,
            const std::array<std::uint8_t, 256>& curKeyDown,
            const MouseInput& mouse,
            bool shiftDown,
            int wheelDeltaUnits)
        {
            state_.capture = capture;
            state_.hasFocus = hasFocus;
            state_.shiftDown = shiftDown;

            // Keys: compute pressed/released edges.
            for (int i = 0; i < 256; ++i)
            {
                const auto idx = static_cast<std::uint8_t>(i);
                const bool down = (curKeyDown[idx] != 0);
                const bool wasDown = (prevKeyDown_[idx] != 0);

                state_.keyDown[idx] = static_cast<std::uint8_t>(down);
                state_.keyPressed[idx] = static_cast<std::uint8_t>(down && !wasDown);
                state_.keyReleased[idx] = static_cast<std::uint8_t>(!down && wasDown);

                prevKeyDown_[idx] = state_.keyDown[idx];
            }

            // Mouse: carry deltas as-is (platform decides when look is active).
            state_.mouse = mouse;

            // Wheel: convert delta units to integer steps (positive/negative), keep remainder.
            constexpr int kWheelDelta = 120;
            wheelRemainderUnits_ += wheelDeltaUnits;

            int steps = 0;
            if (wheelRemainderUnits_ != 0)
            {
                steps = wheelRemainderUnits_ / kWheelDelta;
                wheelRemainderUnits_ -= steps * kWheelDelta;
            }

            state_.mouse.wheelSteps = steps;
        }

    private:
        InputState state_{};
        std::array<std::uint8_t, 256> prevKeyDown_{};
        int wheelRemainderUnits_{ 0 };
    };
}
