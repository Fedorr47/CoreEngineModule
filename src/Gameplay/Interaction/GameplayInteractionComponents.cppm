module;

export module core:gameplay_interaction_components;

import :math_utils;

export namespace rendern
{
    struct GameplayInteractionPointComponent
    {
        mathUtils::Vec3 localPosition{};
        float localFacingYawDegrees{ 0.0f };
    };

    struct GameplayDoorComponent
    {
        bool isOpen{ false };
    };
}
