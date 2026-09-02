module;

#include <cassert>
#include <algorithm>
#include <cmath>
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
    
    struct GameplayRouteFollowerSettings
    {
        // Zero explicitly preserves waypoint-directed route following.
        float cornerLookAheadDistance{0.75f};
    };
    
    class GameplayRouteFollower
    {
    public:
        // The supplied route is owned by value. points[0] is the route start
        // anchor; the follower never prepends a segment from currentPosition.
        [[nodiscard]] GameplayRouteFollowerStatus Start(
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const GameplayRouteFollowerSettings& followerSettings = {})
        {
            if (status_ != GameplayRouteFollowerStatus::NotStarted)
            {
                return status_;
            }
            
            route_ = std::move(route);
            steeringSettings_ = steeringSettings;
            followerSettings_ = followerSettings;
            if (!std::isfinite(followerSettings_.cornerLookAheadDistance) ||
                followerSettings_.cornerLookAheadDistance < 0.0f)
            {
                followerSettings_.cornerLookAheadDistance = 0.0f;
            }
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
            
            bool bAdvancedByLookAhead = false;
            
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
                
                const mathUtils::Vec3& cornerPosition = route_.points[currentSegmentIndex_ + 1u].worldPosition;
                const GameplaySteeringOutput cornerSteering = BuildGameplayArrivalSteering(
                    currentPosition,
                    cornerPosition,
                    segmentSteeringSettings);
                
                if (cornerSteering.status == GameplaySteeringStatus::Arrived)
                {
                    ++currentSegmentIndex_;
                    continue;
                }

                mathUtils::Vec3 steeringTarget = cornerPosition;
                const bool bCanLookAhead = TryBuildLookAheadTarget_(
                    currentPosition,
                    steeringTarget);

                if (!bAdvancedByLookAhead &&
                    bCanLookAhead &&
                    HasPassedCurrentCorner_(currentPosition))
                {
                    bAdvancedByLookAhead = true;
                    ++currentSegmentIndex_;
                    continue;
                }

                const GameplaySteeringOutput steering = bCanLookAhead
                    ? BuildGameplaySeekSteering(
                        currentPosition,
                        steeringTarget,
                        GameplaySeekSteeringSettings{
                            .acceptanceRadius = 0.0f,
                            .wantsRun = steeringSettings_.wantsRun })
                    : cornerSteering;
                
                if (steering.status == GameplaySteeringStatus::Moving)
                {
                    const bool bHasMovementIntent = steering.movement.IsMoving();
                    assert(bHasMovementIntent && "Moving steering status must provide a non-zero movement intent.");
                    output.status = GameplayRouteFollowerStatus::Following;
                    output.movement = steering.movement;
                    return output;
                }
                
                // A degenerate virtual target cannot provide a direction. Keep
                // the authoritative segment and fall back to its corner intent.
                output.status = GameplayRouteFollowerStatus::Following;
                output.movement = cornerSteering.movement;
                return output;
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
            followerSettings_ = GameplayRouteFollowerSettings{};
            currentSegmentIndex_ = 0u;
            status_ = GameplayRouteFollowerStatus::NotStarted;
        }
        
        [[nodiscard]] GameplayRouteFollowerStatus GetStatus() const noexcept
        {
            return status_;
        }
    
    private:
         [[nodiscard]] bool TryBuildLookAheadTarget_(
            const mathUtils::Vec3& currentPosition,
            mathUtils::Vec3& steeringTarget) const noexcept
        {
            const float lookAheadDistance = followerSettings_.cornerLookAheadDistance;
            const std::size_t nextSegmentIndex = currentSegmentIndex_ + 1u;
            if (lookAheadDistance <= 0.0f ||
                nextSegmentIndex >= route_.segmentAnnotations.size() ||
                route_.segmentAnnotations[nextSegmentIndex].traversalLink.has_value())
            {
                return false;
            }

            const mathUtils::Vec3& corner = route_.points[currentSegmentIndex_ + 1u].worldPosition;
            const mathUtils::Vec3& next = route_.points[currentSegmentIndex_ + 2u].worldPosition;
            mathUtils::Vec3 incoming = corner - route_.points[currentSegmentIndex_].worldPosition;
            incoming.y = 0.0f;
            mathUtils::Vec3 outgoing = next - corner;
            outgoing.y = 0.0f;
            const float incomingLength = mathUtils::Length(incoming);
            const float outgoingLength = mathUtils::Length(outgoing);
            if (!std::isfinite(incomingLength) || incomingLength <= mathUtils::kLengthEpsilon ||
                !std::isfinite(outgoingLength) || outgoingLength <= mathUtils::kLengthEpsilon)
            {
                return false;
            }

            mathUtils::Vec3 toCorner = corner - currentPosition;
            toCorner.y = 0.0f;
            const float cornerDistance = mathUtils::Length(toCorner);
            if (!std::isfinite(cornerDistance) || cornerDistance >= lookAheadDistance)
            {
                return false;
            }

            const float distanceAlongNext = std::min(
                lookAheadDistance - cornerDistance,
                outgoingLength);
            steeringTarget = corner + outgoing * (distanceAlongNext / outgoingLength);
            steeringTarget.y = corner.y;
            return true;
        }

        [[nodiscard]] bool HasPassedCurrentCorner_(
            const mathUtils::Vec3& currentPosition) const noexcept
         {
             const std::size_t nextSegmentIndex = currentSegmentIndex_ + 1u;
             if (nextSegmentIndex >= route_.segmentAnnotations.size())
             {
                 return false;
             }

             const mathUtils::Vec3& start = route_.points[currentSegmentIndex_].worldPosition;
             const mathUtils::Vec3& corner = route_.points[currentSegmentIndex_ + 1u].worldPosition;
             const mathUtils::Vec3& next = route_.points[currentSegmentIndex_ + 2u].worldPosition;
             
             mathUtils::Vec3 incoming = corner - start;
             incoming.y = 0.0f;
             mathUtils::Vec3 outgoing = next - corner;
             outgoing.y = 0.0f;

             
             const float incomingLength = mathUtils::Length(incoming);
             const float outgoingLength = mathUtils::Length(outgoing);
             if (!std::isfinite(incomingLength) ||
                 incomingLength <= mathUtils::kLengthEpsilon ||
                 !std::isfinite(outgoingLength) ||
                 outgoingLength <= mathUtils::kLengthEpsilon)

             {
                 return false;
             }

             mathUtils::Vec3 fromCorner = currentPosition - corner;
             fromCorner.y = 0.0f;
             const float cornerDistance = mathUtils::Length(fromCorner);
             const bool bInsideEnvelope = std::isfinite(cornerDistance) &&
                 cornerDistance <= followerSettings_.cornerLookAheadDistance;
            
             const mathUtils::Vec3 incomingDirection = incoming / incomingLength;
             const mathUtils::Vec3 outgoingDirection = outgoing / outgoingLength;
             mathUtils::Vec3 transitionNormal = incomingDirection + outgoingDirection;
             const float transitionNormalLengthSquared =
                 mathUtils::Dot(transitionNormal, transitionNormal);

             if (!mathUtils::IsFinite(transitionNormal) ||
                 transitionNormalLengthSquared <= mathUtils::kLengthEpsilonSq)
             {
                 // Near-180-degree reversals have no stable angle bisector.
                 // Preserve the previous incoming end-plane behavior.
                 transitionNormal = incomingDirection;
             }
             else
             {
                 transitionNormal =
                     transitionNormal / std::sqrt(transitionNormalLengthSquared);
             }

             const bool bCrossedTransitionPlane =
                 mathUtils::Dot(fromCorner, transitionNormal) >= 0.0f;
             return bInsideEnvelope && bCrossedTransitionPlane;
         }
        
        [[nodiscard]] std::optional<GameplayTraversalLinkHandle> GetCurrentTraversalLink() const noexcept
        {
            if (currentSegmentIndex_ >= route_.segmentAnnotations.size())
            {
                return std::nullopt;
            }
            
            return route_.segmentAnnotations[currentSegmentIndex_].traversalLink;
        }
        
        GameplayArrivalSteeringSettings steeringSettings_{};
        GameplayRouteFollowerSettings followerSettings_{};
        GameplayRouteFollowerStatus status_{GameplayRouteFollowerStatus::NotStarted};
        GameplayRoute route_{};
        std::size_t currentSegmentIndex_{0u};
    };
}