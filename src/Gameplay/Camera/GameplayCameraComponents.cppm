module;

export module core:gameplay_camera_components;

import :math_utils;

export namespace rendern
{
    struct GameplayFollowCameraComponent
    {
        float yawRad{ 0.0f };
        float pitchRad{ mathUtils::DegToRad(-18.0f) };
        float distance{ 3.75f };
        mathUtils::Vec3 focusOffset{ 0.0f, 1.55f, 0.0f };
        float mouseSensitivity{ 0.0025f };
        float maxPitchRad{ mathUtils::DegToRad(80.0f) };
        bool initialized{ false };
        bool consumeMouseLook{ true };
    };
}
