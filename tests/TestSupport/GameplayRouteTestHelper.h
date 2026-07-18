#pragma once

import core;

namespace GameplayRouteTestHelper
{
    [[nodiscard]] constexpr mathUtils::Vec3 MakeWorldPosition(
        const float x,
        const float y,
        const float z) noexcept
    {
        return mathUtils::Vec3{ x, y, z };
    }
    
    [[nodiscard]] constexpr rendern::GameplayRoutePoint MakeRoutePoint(
        const float x,
        const float y,
        const float z) noexcept
    {
        return rendern::GameplayRoutePoint{
            .worldPosition = MakeWorldPosition(x, y, z)
        };
    }

    [[nodiscard]] constexpr rendern::GameplayTraversalLinkHandle
        MakeTraversalLink(
            const rendern::GameplayTraversalLinkHandle::ValueType value) noexcept
    {
        return rendern::GameplayTraversalLinkHandle{ value };
    }
}