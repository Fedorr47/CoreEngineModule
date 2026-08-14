module;

#include <cstddef>
#include <cstdint>
#include <cmath>

export module core:gameplay_traversal_link;

import :gameplay;
import :gameplay_route;
import :math_utils;

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

    struct GameplayJumpTraversalData
    {
        mathUtils::Vec3 takeoffPosition{};
        mathUtils::Vec3 landingPosition{};
        float verticalSpeed{0.0f};
        float takeoffTolerance{0.0f};
        float landingHorizontalTolerance{0.0f};
        float landingVerticalTolerance{0.0f};

        [[nodiscard]] bool IsValid() const noexcept
        {
            return mathUtils::IsFinite(takeoffPosition) && mathUtils::IsFinite(landingPosition) &&
                std::isfinite(verticalSpeed) && verticalSpeed > 0.0f &&
                std::isfinite(takeoffTolerance) && takeoffTolerance > 0.0f &&
                std::isfinite(landingHorizontalTolerance) && landingHorizontalTolerance > 0.0f &&
                std::isfinite(landingVerticalTolerance) && landingVerticalTolerance > 0.0f;
        }
    };

    struct GameplayTraversalLink
    {
        GameplayTraversalLinkHandle handle{};
        GameplayTraversalTypeId traversalTypeId{};
        EntityHandle targetEntity{kNullEntity};
        GameplayJumpTraversalData jump{};

        [[nodiscard]] bool IsValid() const noexcept
        {
            return handle.IsValid() && traversalTypeId.IsValid() && targetEntity != kNullEntity &&
                (traversalTypeId != kJumpTraversalTypeId || jump.IsValid());
        }
    };
}
