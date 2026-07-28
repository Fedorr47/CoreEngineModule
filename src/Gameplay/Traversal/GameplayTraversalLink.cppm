module;

#include <cstddef>
#include <cstdint>

export module core:gameplay_traversal_link;

import :gameplay;
import :gameplay_route;

export namespace rendern
{
    struct GameplayTraversalTypeId
    {
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue{0u};

        ValueType value{InvalidValue};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        [[nodiscard]] constexpr bool operator==(const GameplayTraversalTypeId&) const noexcept = default;
    };

    struct GameplayTraversalTypeIdHasher
    {
        [[nodiscard]] constexpr std::size_t operator()(const GameplayTraversalTypeId typeId) const noexcept
        {
            return static_cast<std::size_t>(typeId.value);
        }
    };

    inline constexpr GameplayTraversalTypeId kDoorTraversalTypeId{1u};
    inline constexpr GameplayTraversalTypeId kJumpTraversalTypeId{2u};

    struct GameplayTraversalLink
    {
        GameplayTraversalLinkHandle handle{};
        GameplayTraversalTypeId traversalTypeId{};
        EntityHandle targetEntity{kNullEntity};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return handle.IsValid() && traversalTypeId.IsValid() && targetEntity != kNullEntity;
        }
    };
}