#include <gtest/gtest.h>
#include <array>
#include <cstdint>

import core;

using namespace rendern;

TEST(InputCore, PressReleaseEdges)
{
    InputCore core;

    std::array<std::uint8_t, 256> keys{};
    MouseInput mouse{};

    // Frame 1: key 'W' down
    keys[static_cast<std::uint8_t>('W')] = 1;
    core.NewFrame({}, true, keys, mouse, false, 0);

    const auto& s1 = core.State();
    EXPECT_TRUE(s1.KeyDown('W'));
    EXPECT_TRUE(s1.KeyPressed('W'));
    EXPECT_FALSE(s1.KeyReleased('W'));

    // Frame 2: still down
    core.NewFrame({}, true, keys, mouse, false, 0);
    const auto& s2 = core.State();
    EXPECT_TRUE(s2.KeyDown('W'));
    EXPECT_FALSE(s2.KeyPressed('W'));
    EXPECT_FALSE(s2.KeyReleased('W'));

    // Frame 3: released
    keys[static_cast<std::uint8_t>('W')] = 0;
    core.NewFrame({}, true, keys, mouse, false, 0);
    const auto& s3 = core.State();
    EXPECT_FALSE(s3.KeyDown('W'));
    EXPECT_FALSE(s3.KeyPressed('W'));
    EXPECT_TRUE(s3.KeyReleased('W'));
}

TEST(InputCore, WheelRemainderIsPreserved)
{
    InputCore core;

    std::array<std::uint8_t, 256> keys{};
    MouseInput mouse{};

    // +60 units -> 0 steps
    core.NewFrame({}, true, keys, mouse, false, +60);
    EXPECT_EQ(core.State().mouse.wheelSteps, 0);

    // +60 again -> now +1 step
    core.NewFrame({}, true, keys, mouse, false, +60);
    EXPECT_EQ(core.State().mouse.wheelSteps, 1);

    // -240 -> -2 steps
    core.NewFrame({}, true, keys, mouse, false, -240);
    EXPECT_EQ(core.State().mouse.wheelSteps, -2);
}

TEST(InputCore, CaptureAndShiftAreStored)
{
    InputCore core;

    std::array<std::uint8_t, 256> keys{};
    MouseInput mouse{};

    InputCapture cap{};
    cap.captureKeyboard = true;
    cap.captureMouse = false;

    core.NewFrame(cap, false, keys, mouse, true, 0);

    const auto& s = core.State();
    EXPECT_FALSE(s.hasFocus);
    EXPECT_TRUE(s.shiftDown);
    EXPECT_TRUE(s.capture.captureKeyboard);
    EXPECT_FALSE(s.capture.captureMouse);
}
