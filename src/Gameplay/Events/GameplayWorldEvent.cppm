module;

#include <cstdint>
#include <string>

export module core:gameplay_world_event;

import :EnTTHelpers;

export namespace rendern
{
    using namespace EnTT_helpers;

    enum class GameplayWorldEventType : std::uint8_t
    {
        PickupCollected,
        ResourcePurchased,
        HideEntityRequested
    };

    struct GameplayWorldEvent
    {
        GameplayWorldEventType type{};
        EntityHandle instigator{ kNullEntity };
        EntityHandle subject{ kNullEntity };
        // Owned semantic identity, required for ResourcePurchased. Other events leave it empty.
        std::string receiptId{};
    };
}