module;

#include <cstdint>

export module core:gameplay_world_event;

import :EnTTHelpers;

export namespace rendern
{
    using namespace EnTT_helpers;

    enum class GameplayWorldEventType : std::uint8_t
    {
        PickupCollected,
        ResourcePurchased
    };

    struct GameplayWorldEvent
    {
        GameplayWorldEventType type{};
        EntityHandle instigator{ kNullEntity };
        EntityHandle subject{ kNullEntity };
    };
}