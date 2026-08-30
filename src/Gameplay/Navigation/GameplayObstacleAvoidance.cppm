module;

#include <algorithm>
#include <cmath>

export module core:gameplay_obstacle_avoidance;

import :math_utils;
import :gameplay_steering;

export namespace rendern
{
    struct GameplayObstacleProbeRequest
    {
        mathUtils::Vec3 origin{};
        mathUtils::Vec3 direction{};
        float maximumDistance{0.0f};
    };

    struct GameplayObstacleProbeHit
    {
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };

    enum class GameplayObstacleAvoidanceSide { None, Left, Right };

    struct GameplayObstacleProbeDebugState
    {
        GameplayObstacleProbeRequest request{};
        bool queried{false};
        bool hit{false};
        float clearance{0.0f};
        mathUtils::Vec3 hitPosition{};
        mathUtils::Vec3 hitNormal{};
    };

    struct GameplayObstacleAvoidanceDebugSnapshot
    {
        bool evaluated{false};
        bool active{false};
        mathUtils::Vec3 probeOrigin{};
        GameplayMovementIntent baseMovement{};
        GameplayMovementIntent finalMovement{};
        GameplayObstacleProbeDebugState forward{};
        GameplayObstacleProbeDebugState left{};
        GameplayObstacleProbeDebugState right{};
        GameplayObstacleAvoidanceSide chosenSide{GameplayObstacleAvoidanceSide::None};
    };

    class IGameplayObstacleQuery
    {
    public:
        virtual ~IGameplayObstacleQuery() = default;

        [[nodiscard]] virtual bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept = 0;
    };

    struct GameplayObstacleAvoidanceSettings
    {
        float forwardProbeDistance{1.5f};
        float sideProbeDistance{1.0f};
        float sideProbeAngleDegrees{30.0f};
    };

    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
        const GameplayMovementIntent& baseMovement,
        const mathUtils::Vec3& probeOrigin,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings = {},
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept;
}

namespace
{
    struct SanitizedSettings
    {
        float forwardDistance{0.0f};
        float sideDistance{0.0f};
        float sideAngleRadians{0.0f};
    };

    struct ProbeResult
    {
        bool hit{false};
        float clearance{0.0f};
        mathUtils::Vec3 hitPosition{};
        mathUtils::Vec3 hitNormal{};
    };

    [[nodiscard]] float FiniteOrZero(const float value) noexcept
    {
        return std::isfinite(value) ? std::max(value, 0.0f) : 0.0f;
    }

    [[nodiscard]] SanitizedSettings Sanitize(
        const rendern::GameplayObstacleAvoidanceSettings& settings) noexcept
    {
        const float angleDegrees = std::isfinite(settings.sideProbeAngleDegrees)
            ? std::clamp(settings.sideProbeAngleDegrees, 0.0f, 90.0f)
            : 0.0f;
        return {
            .forwardDistance = FiniteOrZero(settings.forwardProbeDistance),
            .sideDistance = FiniteOrZero(settings.sideProbeDistance),
            .sideAngleRadians = mathUtils::DegToRad(angleDegrees)
        };
    }

    [[nodiscard]] ProbeResult RunProbe(
        const rendern::IGameplayObstacleQuery& query,
        const mathUtils::Vec3& origin,
        const mathUtils::Vec3& direction,
        const float maximumDistance,
        rendern::GameplayObstacleProbeDebugState* debugOut) noexcept
    {
        const rendern::GameplayObstacleProbeRequest request{origin, direction, maximumDistance};
        if (debugOut != nullptr)
        {
            *debugOut = {};
            debugOut->request = request;
            debugOut->clearance = maximumDistance;
        }
        if (maximumDistance <= 0.0f)
        {
            return {};
        }

        rendern::GameplayObstacleProbeHit hit{};
        if (debugOut != nullptr) debugOut->queried = true;
        const bool didHit = query.Probe(request, hit);
        if (!didHit)
        {
            return {.hit = false, .clearance = maximumDistance};
        }

        const float distance = std::isfinite(hit.distance) ? hit.distance : 0.0f;
        const ProbeResult result{.hit = true,
            .clearance = std::clamp(distance, 0.0f, maximumDistance),
            .hitPosition = hit.position, .hitNormal = hit.normal};
        if (debugOut != nullptr)
        {
            debugOut->hit = true;
            debugOut->clearance = result.clearance;
            debugOut->hitPosition = result.hitPosition;
            debugOut->hitNormal = result.hitNormal;
        }
        return result;
    }
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
    const GameplayMovementIntent& baseMovement,
    const mathUtils::Vec3& probeOrigin,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings,
    GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
{
    if (debugOut != nullptr)
    {
        *debugOut = {};
        debugOut->evaluated = true;
        debugOut->probeOrigin = probeOrigin;
        debugOut->baseMovement = baseMovement;
        debugOut->finalMovement = baseMovement;
    }
    if (!baseMovement.IsMoving())
    {
        return baseMovement;
    }

    mathUtils::Vec3 forward{baseMovement.moveWorld.x, 0.0f, baseMovement.moveWorld.z};
    const float forwardLengthSquared = mathUtils::Dot(forward, forward);
    if (!mathUtils::IsFinite(forward) || forwardLengthSquared <= mathUtils::kLengthEpsilonSq)
    {
        return baseMovement;
    }
    forward = forward / std::sqrt(forwardLengthSquared);

    const SanitizedSettings sanitized = Sanitize(settings);
    const float cosine = std::cos(sanitized.sideAngleRadians);
    const float sine = std::sin(sanitized.sideAngleRadians);
    
    const mathUtils::Vec3 left{
        (forward.x * cosine) + (forward.z * sine),
        0.0f,
        (forward.z * cosine) - (forward.x * sine)
    };
    const mathUtils::Vec3 right{
        (forward.x * cosine) - (forward.z * sine),
        0.0f,
        (forward.z * cosine) + (forward.x * sine)
    };

    const ProbeResult forwardResult = RunProbe(
        query, probeOrigin, forward, sanitized.forwardDistance,
        debugOut != nullptr ? &debugOut->forward : nullptr);
    const ProbeResult leftResult = RunProbe(
        query, probeOrigin, left, sanitized.sideDistance,
        debugOut != nullptr ? &debugOut->left : nullptr);
    const ProbeResult rightResult = RunProbe(
        query, probeOrigin, right, sanitized.sideDistance,
        debugOut != nullptr ? &debugOut->right : nullptr);

    if (!forwardResult.hit)
    {
        return baseMovement;
    }

    // Equal clearance always selects left, keeping symmetric situations deterministic.
    GameplayMovementIntent corrected = baseMovement;
    const bool choseLeft = leftResult.clearance >= rightResult.clearance;
    corrected.moveWorld = choseLeft ? left : right;
    if (debugOut != nullptr)
    {
        const float directionDot = mathUtils::Dot(forward, corrected.moveWorld);
        const bool changedDirection = std::isfinite(directionDot) &&
            directionDot < 1.0f - mathUtils::kLengthEpsilonSq;
        debugOut->active = changedDirection;
        debugOut->chosenSide = changedDirection
            ? (choseLeft ? GameplayObstacleAvoidanceSide::Left
                         : GameplayObstacleAvoidanceSide::Right)
            : GameplayObstacleAvoidanceSide::None;
        debugOut->finalMovement = corrected;
    }
    return corrected;
}