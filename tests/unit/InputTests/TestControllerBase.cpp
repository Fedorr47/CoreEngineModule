#include <gtest/gtest.h>
#include <cstdint>

import core;

using namespace rendern;

namespace
{
    class DummyController final : public ControllerBase
    {
    public:
        bool CanK(const InputState& s) const { return AllowKeyboard(s); }
        bool CanM(const InputState& s) const { return AllowMouse(s); }
        bool CanU(const InputState& s) const { return CanUpdate(s); }
    };
}

TEST(ControllerBase, EnabledAndFocusGating)
{
    DummyController c;
    InputState s{};

    s.hasFocus = true;
    EXPECT_TRUE(c.CanU(s));

    s.hasFocus = false;
    EXPECT_FALSE(c.CanU(s));

    c.SetRequireFocus(false);
    EXPECT_TRUE(c.CanU(s));

    c.SetEnabled(false);
    EXPECT_FALSE(c.CanU(s));
}

TEST(ControllerBase, CaptureGating)
{
    DummyController c;
    InputState s{};
    s.hasFocus = true;

    s.capture.captureKeyboard = true;
    EXPECT_FALSE(c.CanK(s));
    EXPECT_TRUE(c.CanM(s));

    s.capture.captureKeyboard = false;
    s.capture.captureMouse = true;
    EXPECT_TRUE(c.CanK(s));
    EXPECT_FALSE(c.CanM(s));
}
