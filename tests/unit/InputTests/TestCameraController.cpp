#include <gtest/gtest.h>

import core;

using namespace rendern;

static void ExpectVec3Near(const mathUtils::Vec3& a, const mathUtils::Vec3& b, float eps = 1e-4f)
{
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
    EXPECT_NEAR(a.z, b.z, eps);
}

TEST(CameraController, ResetFromCamera_ForwardZ)
{
    Camera cam{};
    cam.position = { 0.0f, 0.0f, 0.0f };
    cam.target   = { 0.0f, 0.0f, 1.0f };
    cam.up       = { 0.0f, 1.0f, 0.0f };

    CameraController ctl;
    ctl.ResetFromCamera(cam);

    EXPECT_NEAR(ctl.YawRad(), 0.0f, 1e-4f);
    EXPECT_NEAR(ctl.PitchRad(), 0.0f, 1e-4f);

    ExpectVec3Near(ctl.Forward(), { 0.0f, 0.0f, 1.0f });
}

TEST(CameraController, MoveForward_W)
{
    Camera cam{};
    cam.position = { 0.0f, 0.0f, 0.0f };
    cam.target   = { 0.0f, 0.0f, 1.0f };

    CameraController ctl;
    ctl.ResetFromCamera(cam);
    ctl.Settings().moveSpeed = 10.0f;

    InputState in{};
    in.hasFocus = true;
    in.keyDown[static_cast<std::uint8_t>('W')] = 1;

    ctl.Update(1.0f, in, cam);

    ExpectVec3Near(cam.position, { 0.0f, 0.0f, 10.0f });
}

TEST(CameraController, SprintUsesShiftDown)
{
    Camera cam{};
    cam.position = { 0.0f, 0.0f, 0.0f };
    cam.target   = { 0.0f, 0.0f, 1.0f };

    CameraController ctl;
    ctl.ResetFromCamera(cam);
    ctl.Settings().moveSpeed = 2.0f;
    ctl.Settings().sprintMultiplier = 5.0f;

    InputState in{};
    in.hasFocus = true;
    in.shiftDown = true;
    in.keyDown[static_cast<std::uint8_t>('W')] = 1;

    ctl.Update(1.0f, in, cam);

    ExpectVec3Near(cam.position, { 0.0f, 0.0f, 10.0f });
}

TEST(CameraController, MouseLookIgnoredWhenMouseCaptured)
{
    Camera cam{};
    cam.position = { 0.0f, 0.0f, 0.0f };
    cam.target   = { 0.0f, 0.0f, 1.0f };

    CameraController ctl;
    ctl.ResetFromCamera(cam);

    const float yaw0 = ctl.YawRad();
    const float pitch0 = ctl.PitchRad();

    InputState in{};
    in.hasFocus = true;
    in.capture.captureMouse = true;
    in.mouse.lookDx = 100;
    in.mouse.lookDy = 50;

    ctl.Update(1.0f, in, cam);

    EXPECT_NEAR(ctl.YawRad(), yaw0, 1e-6f);
    EXPECT_NEAR(ctl.PitchRad(), pitch0, 1e-6f);
}

TEST(CameraController, MovementIgnoredWhenKeyboardCaptured)
{
    Camera cam{};
    cam.position = { 0.0f, 0.0f, 0.0f };
    cam.target   = { 0.0f, 0.0f, 1.0f };

    CameraController ctl;
    ctl.ResetFromCamera(cam);
    ctl.Settings().moveSpeed = 10.0f;

    InputState in{};
    in.hasFocus = true;
    in.capture.captureKeyboard = true;
    in.keyDown[static_cast<std::uint8_t>('W')] = 1;

    ctl.Update(1.0f, in, cam);

    ExpectVec3Near(cam.position, { 0.0f, 0.0f, 0.0f });
}
