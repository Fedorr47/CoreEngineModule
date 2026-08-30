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
        const GameplayObstacleAvoidanceSettings& settings = {}) noexcept;
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
        const float maximumDistance) noexcept
    {
        if (maximumDistance <= 0.0f)
        {
            return {};
        }

        rendern::GameplayObstacleProbeHit hit{};
        const bool didHit = query.Probe({origin, direction, maximumDistance}, hit);
        if (!didHit)
        {
            return {.hit = false, .clearance = maximumDistance};
        }

        const float distance = std::isfinite(hit.distance) ? hit.distance : 0.0f;
        return {.hit = true, .clearance = std::clamp(distance, 0.0f, maximumDistance)};
    }
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
    const GameplayMovementIntent& baseMovement,
    const mathUtils::Vec3& probeOrigin,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings) noexcept
{
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
        query, probeOrigin, forward, sanitized.forwardDistance);
    const ProbeResult leftResult = RunProbe(
        query, probeOrigin, left, sanitized.sideDistance);
    const ProbeResult rightResult = RunProbe(
        query, probeOrigin, right, sanitized.sideDistance);

    if (!forwardResult.hit)
    {
        return baseMovement;
    }

    // Equal clearance always selects left, keeping symmetric situations deterministic.
    GameplayMovementIntent corrected = baseMovement;
    corrected.moveWorld = leftResult.clearance >= rightResult.clearance ? left : right;
    return corrected;
}