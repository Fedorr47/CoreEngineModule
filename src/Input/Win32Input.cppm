module;

#if defined(_WIN32)
  // Prevent Windows headers from defining the `min`/`max` macros which break std::min/std::max.
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <windowsx.h> // GET_WHEEL_DELTA_WPARAM
#endif

#include <array>
#include <cstdint>

export module core:win32_input;

import :input;

export namespace rendern
{
    class Win32Input
    {
    public:
        Win32Input() = default;

        void SetCaptureMode(InputCapture cap) noexcept
        {
            state_.capture = cap;
        }

        const InputState& State() const noexcept { return state_; }

#if defined(_WIN32)
        void OnWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            (void)hwnd; (void)lParam;
            switch (msg)
            {
            case WM_MOUSEWHEEL:
                wheelAccum_ += GET_WHEEL_DELTA_WPARAM(wParam);
                break;
            case WM_KILLFOCUS:
            case WM_ACTIVATEAPP:
                ReleaseLook(hwnd);
                break;
            default:
                break;
            }
        }

        // Poll keyboard/mouse and update internal InputState for this frame.
        void NewFrame(HWND hwnd)
        {
            // Per-frame values
            state_.mouse.lookDx = 0;
            state_.mouse.lookDy = 0;

            // Focus check (cheap and robust)
            state_.hasFocus = (GetForegroundWindow() == hwnd);

            // Keyboard: poll all virtual keys.
            for (int i = 0; i < 256; ++i)
            {
                const bool down = (GetAsyncKeyState(i) & 0x8000) != 0;
                const bool wasDown = (prevKeyDown_[static_cast<std::uint8_t>(i)] != 0);

                state_.keyDown[static_cast<std::uint8_t>(i)] = static_cast<std::uint8_t>(down);
                state_.keyPressed[static_cast<std::uint8_t>(i)] = static_cast<std::uint8_t>(down && !wasDown);
                state_.keyReleased[static_cast<std::uint8_t>(i)] = static_cast<std::uint8_t>(!down && wasDown);

                prevKeyDown_[static_cast<std::uint8_t>(i)] = static_cast<std::uint8_t>(down);
            }

            state_.shiftDown = state_.KeyDown(VK_SHIFT);

            // Mouse buttons
            state_.mouse.rmbDown = state_.KeyDown(VK_RBUTTON);

            // Mouse wheel: convert to whole steps for this frame.
            // Windows uses WHEEL_DELTA = 120 units per notch.
            int steps = 0;
            if (wheelAccum_ != 0)
            {
                steps = wheelAccum_ / WHEEL_DELTA;
                wheelAccum_ = 0; // keep it simple (no fractional remainder)
            }
            state_.mouse.wheelSteps = steps;

            // Relative mouse look (hold RMB): managed here (platform layer), not in camera controller.
            const bool allowMouse = state_.hasFocus && !state_.capture.captureMouse;
            if (!allowMouse)
            {
                ReleaseLook(hwnd);
                return;
            }

            const bool rmbDown = state_.mouse.rmbDown;
            if (rmbDown && !lookActive_)
            {
                BeginLook(hwnd);
            }
            else if (!rmbDown && lookActive_)
            {
                ReleaseLook(hwnd);
            }

            if (lookActive_)
            {
                UpdateLook(hwnd);
            }
        }

#else
        // Stubs for non-Windows builds.
        void OnWndProc(void*, unsigned, std::uintptr_t, std::intptr_t) {}
        void NewFrame(void*) {}
#endif

    private:
#if defined(_WIN32)
        void BeginLook(HWND hwnd)
        {
            lookActive_ = true;

            // Save cursor pos to restore later.
            GetCursorPos(&savedCursorPos_);

            ::SetCapture(hwnd);

            // Hide cursor (ShowCursor uses an internal counter).
            while (ShowCursor(FALSE) >= 0) {}

            CenterCursor(hwnd);
            lastCenterValid_ = true;
        }

        void ReleaseLook(HWND hwnd)
        {
            if (!lookActive_)
                return;

            lookActive_ = false;
            lastCenterValid_ = false;

            ReleaseCapture();

            // Show cursor back.
            while (ShowCursor(TRUE) < 0) {}

            SetCursorPos(savedCursorPos_.x, savedCursorPos_.y);
            (void)hwnd;
        }

        void CenterCursor(HWND hwnd)
        {
            RECT rc{};
            GetClientRect(hwnd, &rc);

            POINT pt{};
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;

            ClientToScreen(hwnd, &pt);
            centerScreen_ = pt;

            SetCursorPos(centerScreen_.x, centerScreen_.y);
        }

        void UpdateLook(HWND hwnd)
        {
            if (!lastCenterValid_)
            {
                CenterCursor(hwnd);
                lastCenterValid_ = true;
                return;
            }

            POINT cur{};
            GetCursorPos(&cur);

            const int dx = cur.x - centerScreen_.x;
            const int dy = cur.y - centerScreen_.y;

            state_.mouse.lookDx = dx;
            state_.mouse.lookDy = dy;

            if (dx != 0 || dy != 0)
            {
                // Re-center.
                SetCursorPos(centerScreen_.x, centerScreen_.y);
            }
        }
#endif

        InputState state_{};

#if defined(_WIN32)
        std::array<std::uint8_t, 256> prevKeyDown_{};

        int wheelAccum_{ 0 };

        bool lookActive_{ false };
        bool lastCenterValid_{ false };
        POINT savedCursorPos_{};
        POINT centerScreen_{};
#endif
    };
}
