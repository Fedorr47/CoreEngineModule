module;

export module core:gameplay_entity_components;

import :math_utils;
import :EnTTHelpers;

export namespace rendern
{
    using namespace EnTT_helpers;
    struct GameplayTransformComponent
    {
        mathUtils::Vec3 position{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 rotationDegrees{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 scale{ 1.0f, 1.0f, 1.0f };
    };

    struct GameplayNodeLinkComponent
    {
        int nodeIndex{ -1 };
        EntityHandle levelEntity{ kNullEntity };
    };
    
    struct GameplayPickupComponent
    {
        float collectionRadius{ 0.6f };
        bool collected{ false };
    };
}
