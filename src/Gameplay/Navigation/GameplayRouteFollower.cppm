module;

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <utility>

export module core:gameplay_route_follower;

import :math_utils;
import :gameplay_route;
import :gameplay_steering;

export namespace rendern
{
    enum class GameplayRouteFollowerStatus : std::uint8_t
    {
        NotStarted,
        Following,
        TraversalRequired,
        Succeeded,
        InvalidRoute
    };
    
    struct GameplayRouteFollowerOutput
    {
        GameplayMovementIntent movement;
        GameplayRouteFollowerStatus status{GameplayRouteFollowerStatus::NotStarted};
        std::optional<GameplayTraversalLinkHandle> requiredTraversalLink{};
    };
    
    class GameplayRouteFollower
    {
    public:
        // The supplied route is owned by value. points[0] is the route start
        // anchor; the follower never prepends a segment from currentPosition.
        [[nodiscard]] GameplayRouteFollowerStatus Start(
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {})
        {
            if (status_ != GameplayRouteFollowerStatus::NotStarted)
            {
                return status_;
            }
            
            route_ = std::move(route);
            steeringSettings_ = steeringSettings;
            currentSegmentIndex_ = 0u;
            
            if (!route_.IsValid())
            {
                status_ = GameplayRouteFollowerStatus::InvalidRoute;
                return status_;
            }
            
            if (route_.segmentAnnotations.empty())
            {
                status_ = GameplayRouteFollowerStatus::Succeeded;
                return status_;
            }
            
            status_ = GameplayRouteFollowerStatus::Following;
            return status_;
        }
        
        [[nodiscard]] GameplayRouteFollowerOutput Tick(
            const mathUtils::Vec3& currentPosition)
        {
            GameplayRouteFollowerOutput output{};
            output.status = status_;
            
            if (status_ == GameplayRouteFollowerStatus::TraversalRequired)
            {
                output.requiredTraversalLink = GetCurrentTraversalLink();
                return output;
            }
            
            if (status_ != GameplayRouteFollowerStatus::Following)
            {
                return output;
            }
            
            while (currentSegmentIndex_ < route_.segmentAnnotations.size())
            {
                const GameplayRouteSegmentAnnotation& annotation = 
                    route_.segmentAnnotations[currentSegmentIndex_];
                
                if (annotation.traversalLink.has_value())
                {
                    status_ = GameplayRouteFollowerStatus::TraversalRequired;
                    output.status = status_;
                    output.requiredTraversalLink = annotation.traversalLink;
                    return output;
                }
                
                const bool bIsFinalSegment = currentSegmentIndex_ + 1u == route_.segmentAnnotations.size();

                GameplayArrivalSteeringSettings segmentSteeringSettings = steeringSettings_;
                segmentSteeringSettings.slowingRadius = bIsFinalSegment
                    ? steeringSettings_.slowingRadius
                    : steeringSettings_.acceptanceRadius;
                
                const mathUtils::Vec3& targetPosition = route_.points[currentSegmentIndex_ + 1u].worldPosition;
                const GameplaySteeringOutput steering = BuildGameplayArrivalSteering(
                    currentPosition,
                    targetPosition,
                    segmentSteeringSettings);
                
                if (steering.status == GameplaySteeringStatus::Moving)
                {
                    const bool bHasMovementIntent = steering.movement.IsMoving();
                    assert(bHasMovementIntent && "Moving steering status must provide a non-zero movement intent.");
                    output.status = GameplayRouteFollowerStatus::Following;
                    output.movement = steering.movement;
                    return output;
                }
                
                ++currentSegmentIndex_;
            }
            
            status_ = GameplayRouteFollowerStatus::Succeeded;
            output.status = status_;
            return output;
        }
        
        // A successful completion means the external traversal system reached
        // points[currentSegmentIndex + 1] for the annotated segment.
        [[nodiscard]] bool CompleteTraversal(
            GameplayTraversalLinkHandle completedTraversalLink) noexcept
        {
            const std::optional<GameplayTraversalLinkHandle> pendingTraversalLink =
                GetCurrentTraversalLink();
            const bool bCnaCompleteTraversal = 
                status_ == GameplayRouteFollowerStatus::TraversalRequired &&
                pendingTraversalLink.has_value() &&
                completedTraversalLink.IsValid() &&
                completedTraversalLink == *pendingTraversalLink;
            
            if (!bCnaCompleteTraversal)
            {
                return false;
            }
            
            ++currentSegmentIndex_;
            status_ = currentSegmentIndex_ >= route_.segmentAnnotations.size() 
                ? GameplayRouteFollowerStatus::Succeeded
                : GameplayRouteFollowerStatus::Following;
            
            return true;
        }
        
        void Reset() noexcept
        {
            route_ = GameplayRoute{};
            steeringSettings_ = GameplayArrivalSteeringSettings{};
            currentSegmentIndex_ = 0u;
            status_ = GameplayRouteFollowerStatus::NotStarted;
        }
        
        [[nodiscard]] GameplayRouteFollowerStatus GetStatus() const noexcept
        {
            return status_;
        }
    
    private:
        [[nodiscard]] std::optional<GameplayTraversalLinkHandle> GetCurrentTraversalLink() const noexcept
        {
            if (currentSegmentIndex_ >= route_.segmentAnnotations.size())
            {
                return std::nullopt;
            }
            
            return route_.segmentAnnotations[currentSegmentIndex_].traversalLink;
        }
        
        GameplayArrivalSteeringSettings steeringSettings_{};
        GameplayRouteFollowerStatus status_{GameplayRouteFollowerStatus::NotStarted};
        GameplayRoute route_{};
        std::size_t currentSegmentIndex_{0u};
    };
}