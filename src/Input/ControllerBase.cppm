export module core:controller_base;

import :input;

export namespace rendern
{
    // Shared gating logic for any controller driven by InputState.
    class ControllerBase
    {
    public:
        virtual ~ControllerBase() = default;

        void SetEnabled(bool v) noexcept { enabled_ = v; }
        bool Enabled() const noexcept { return enabled_; }

        void SetRequireFocus(bool v) noexcept { requireFocus_ = v; }
        bool RequireFocus() const noexcept { return requireFocus_; }

    protected:
        bool CanUpdate(const InputState& input) const noexcept
        {
            if (!enabled_)
                return false;
            if (requireFocus_ && !input.hasFocus)
                return false;
            return true;
        }

        bool AllowKeyboard(const InputState& input) const noexcept
        {
            return CanUpdate(input) && !input.capture.captureKeyboard;
        }

        bool AllowMouse(const InputState& input) const noexcept
        {
            return CanUpdate(input) && !input.capture.captureMouse;
        }

    private:
        bool enabled_{ true };
        bool requireFocus_{ true };
    };
}
