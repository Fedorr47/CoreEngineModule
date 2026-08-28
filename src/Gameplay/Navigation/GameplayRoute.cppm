module;

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

export module core:gameplay_route;
import :math_utils;

export namespace rendern
{
    struct GameplayTraversalLinkHandle
    {
        using ValueType = std::uint64_t;
        
        static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();
        
        ValueType value{InvalidValue};
        
        constexpr GameplayTraversalLinkHandle() noexcept = default;
        
        explicit constexpr GameplayTraversalLinkHandle(
            const ValueType inValue) noexcept : value(inValue) {}
        
        [[nodiscard]] constexpr bool IsValid() const noexcept 
        { return value != InvalidValue; }
        
        friend constexpr bool operator==(
            const GameplayTraversalLinkHandle&,
            const GameplayTraversalLinkHandle&) noexcept = default;
    };
    
    struct GameplayRoutePoint
    {
        // World-space point in the ordered route. Adjacent points define
        // the endpoints of the corresponding route segment.
        mathUtils::Vec3 worldPosition{};
    };
    
    struct GameplayRouteSegmentAnnotation
    {
        // Empty means the segment is traversed through ordinary steering.
        // A valid link delegates special traversal to an external system.
        std::optional<GameplayTraversalLinkHandle> traversalLink{};
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            const bool bHasTraversalLink = traversalLink.has_value();
            
            return !bHasTraversalLink || traversalLink->IsValid();
        }
    };
    
    struct GameplayRoute
    {
        // Ordered world-space points. Segment annotation i describes travel
        // from points[i] to points[i + 1].
        std::vector<GameplayRoutePoint> points{};
        
       // Contains exactly one annotation for every pair of adjacent points.
       // An annotation without a traversal link represents ordinary movement.
        std::vector<GameplayRouteSegmentAnnotation> segmentAnnotations{};
        
        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return points.empty() && segmentAnnotations.empty();
        }
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            if (points.empty())
            {
                return segmentAnnotations.empty();
            }
            
            const std::size_t expectedSegmentCount = points.size() - 1u;
            const bool bHasExpectedSegmentCount = segmentAnnotations.size() == expectedSegmentCount;
            
            if (!bHasExpectedSegmentCount)
            {
                return false;
            }
            
            for (const GameplayRoutePoint& point : points)
            {
                const bool bHasFinitePosition =
                    std::isfinite(point.worldPosition.x) &&
                    std::isfinite(point.worldPosition.y) &&
                    std::isfinite(point.worldPosition.z);
                
                if (!bHasFinitePosition)
                {
                    return false;
                }
            }
            
            for (const GameplayRouteSegmentAnnotation& annotation : segmentAnnotations)
            {
                if (!annotation.IsValid())
                {
                    return false;
                }
            }
            
            return true;
        }
    };
}