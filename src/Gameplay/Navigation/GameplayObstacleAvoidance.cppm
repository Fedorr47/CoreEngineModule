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
        float clearanceRadius{0.0f};
    };

    struct GameplayObstacleProbeHit
    {
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };
    
    struct GameplaySupportProbeRequest
    {
        mathUtils::Vec3 origin{};
        float maximumDistance{0.0f};
    };

    struct GameplaySupportProbeHit
    {
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };

    enum class GameplayObstacleAvoidanceSide { None, Left, Right };
    
    struct GameplayObstacleAvoidanceState
    {
        GameplayObstacleAvoidanceSide committedSide{GameplayObstacleAvoidanceSide::None};
    };

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
        GameplayObstacleAvoidanceSide preferredSide{GameplayObstacleAvoidanceSide::None};
        GameplayObstacleAvoidanceSide chosenSide{GameplayObstacleAvoidanceSide::None};
        bool sideHeldByHysteresis{false};
        bool leftSupported{false};
        bool rightSupported{false};
        bool safetyOverride{false};
        mathUtils::Vec3 leftEscapeCandidate{};
        mathUtils::Vec3 rightEscapeCandidate{};
    };

    class IGameplayObstacleQuery
    {
    public:
        virtual ~IGameplayObstacleQuery() = default;

        [[nodiscard]] virtual bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept = 0;
        
        // Queries may retain the legacy, support-unaware behavior by not overriding this.
        [[nodiscard]] virtual bool ProbeSupport(
            const GameplaySupportProbeRequest&,
            GameplaySupportProbeHit&) const noexcept
        {
            return true;
        }
    };

    struct GameplayObstacleAvoidanceSettings
    {
        float forwardProbeDistance{3.0f};
        float sideProbeDistance{2.0f};
        float sideProbeAngleDegrees{30.0f};
        float sideSwitchClearanceAdvantage{0.15f};
        float clearanceMargin{0.02f};
        float supportProbeForwardDistance{0.75f};
        float supportProbeUpOffset{0.25f};
        float maximumSupportDropDistance{0.35f};
    };

    struct GameplayObstacleAvoidanceInput
    {
        GameplayMovementIntent baseMovement{};
        mathUtils::Vec3 probeOrigin{};
        float characterRadius{0.0f};
        float supportOriginVerticalOffset{0.0f};
    };
    
    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
        const GameplayObstacleAvoidanceInput& input,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings = {},
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept;

    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
        const GameplayObstacleAvoidanceInput& input,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings,
        GameplayObstacleAvoidanceState& state,
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept;

    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
        const GameplayMovementIntent& baseMovement,
        const mathUtils::Vec3& probeOrigin,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings = {},
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept;
    
    [[nodiscard]] GameplayMovementIntent ApplyGameplayObstacleAvoidance(
        const GameplayMovementIntent& baseMovement,
        const mathUtils::Vec3& probeOrigin,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings,
        GameplayObstacleAvoidanceState& state,
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept;
}

namespace
{
    // Keep strong lateral escape without cancelling meaningful forward progress.
    constexpr float SelectedFeelerWeight = 0.60f;
    constexpr float SurfaceTangentWeight = 0.40f;
    constexpr float ObstacleSeparationBias = 0.30f;
    
    struct SanitizedSettings
    {
        float forwardDistance{0.0f};
        float sideDistance{0.0f};
        float sideAngleRadians{0.0f};
        float sideSwitchClearanceAdvantage{0.0f};
        float effectiveClearanceRadius{0.0f};
        float supportForwardDistance{0.0f};
        float supportUpOffset{0.0f};
        float maximumSupportDropDistance{0.0f};
        float supportOriginVerticalOffset{0.0f};
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
        const rendern::GameplayObstacleAvoidanceInput& input,
        const rendern::GameplayObstacleAvoidanceSettings& settings) noexcept
    {
        const float angleDegrees = std::isfinite(settings.sideProbeAngleDegrees)
            ? std::clamp(settings.sideProbeAngleDegrees, 0.0f, 90.0f)
            : 0.0f;
        const float characterRadius = FiniteOrZero(input.characterRadius);
        const float clearanceMargin = FiniteOrZero(settings.clearanceMargin);
        const float effectiveClearanceRadius = characterRadius > 0.0f &&
            std::isfinite(characterRadius + clearanceMargin)
            ? characterRadius + clearanceMargin
            : characterRadius;
        return {
            .forwardDistance = FiniteOrZero(settings.forwardProbeDistance),
            .sideDistance = FiniteOrZero(settings.sideProbeDistance),
            .sideAngleRadians = mathUtils::DegToRad(angleDegrees),
            .sideSwitchClearanceAdvantage =
                FiniteOrZero(settings.sideSwitchClearanceAdvantage),
            .effectiveClearanceRadius = effectiveClearanceRadius,
            .supportForwardDistance = FiniteOrZero(settings.supportProbeForwardDistance),
            .supportUpOffset = FiniteOrZero(settings.supportProbeUpOffset),
            .maximumSupportDropDistance = FiniteOrZero(settings.maximumSupportDropDistance),
            .supportOriginVerticalOffset = FiniteOrZero(input.supportOriginVerticalOffset)
        };
    }

    [[nodiscard]] ProbeResult RunProbe(
        const rendern::IGameplayObstacleQuery& query,
        const mathUtils::Vec3& origin,
        const mathUtils::Vec3& direction,
        const float maximumDistance,
        const float clearanceRadius,
        rendern::GameplayObstacleProbeDebugState* debugOut) noexcept
    {
        const rendern::GameplayObstacleProbeRequest request{
            .origin = origin,
            .direction = direction,
            .maximumDistance = maximumDistance,
            .clearanceRadius = clearanceRadius
        };
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
        if (debugOut != nullptr)
        {
            debugOut->queried = true;
        }
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
    
    [[nodiscard]] bool HasSupport(
        const rendern::IGameplayObstacleQuery& query,
        const mathUtils::Vec3& probeOrigin,
        const mathUtils::Vec3& direction,
        const SanitizedSettings& settings) noexcept
    {
        const float maximumDistance = settings.supportUpOffset +
            settings.maximumSupportDropDistance;
        const mathUtils::Vec3 supportBase = probeOrigin -
            mathUtils::Vec3{0.0f, settings.supportOriginVerticalOffset, 0.0f};
        const mathUtils::Vec3 origin = supportBase +
            direction * settings.supportForwardDistance +
            mathUtils::Vec3{0.0f, settings.supportUpOffset, 0.0f};
        if (!mathUtils::IsFinite(origin) || maximumDistance <= 0.0f ||
            !std::isfinite(maximumDistance))
        {
            return false;
        }
        rendern::GameplaySupportProbeHit hit{};
        if (!query.ProbeSupport({.origin = origin, .maximumDistance = maximumDistance}, hit))
        {
            return false;
        }
        return std::isfinite(hit.distance) && hit.distance >= 0.0f &&
            hit.distance <= maximumDistance && mathUtils::IsFinite(hit.position) &&
            mathUtils::IsFinite(hit.normal);
    }

    [[nodiscard]] mathUtils::Vec3 BuildEscapeCandidate(
        const mathUtils::Vec3& selectedFeeler,
        const mathUtils::Vec3& forward,
        const rendern::GameplayObstacleAvoidanceSide side,
        const mathUtils::Vec3& forwardHitNormal) noexcept
    {
        mathUtils::Vec3 planarNormal{
            forwardHitNormal.x, 0.0f, forwardHitNormal.z};
        const float normalLengthSquared = mathUtils::Dot(planarNormal, planarNormal);
        if (!mathUtils::IsFinite(planarNormal) ||
            normalLengthSquared <= mathUtils::kLengthEpsilonSq)
        {
            return selectedFeeler;
        }

        planarNormal = planarNormal / std::sqrt(normalLengthSquared);
        
        // Orient the surface tangent against the stable steering right axis,
        // not against the rotating side feeler. Using the feeler here creates
        // a discontinuity when Dot(tangent, feeler) crosses zero and can make
        // a committed side suddenly reverse its actual escape direction.
        const mathUtils::Vec3 rightAxis{
            -forward.z,
            0.0f,
            forward.x
        };
        mathUtils::Vec3 rightTangent{
            -planarNormal.z,
            0.0f,
            planarNormal.x
        };
        if (mathUtils::Dot(rightTangent, rightAxis) < 0.0f)
        {
            rightTangent = rightTangent * -1.0f;
        }
        
        const mathUtils::Vec3 sideTangent =
            side == rendern::GameplayObstacleAvoidanceSide::Left
            ? rightTangent * -1.0f
            : rightTangent;
        
        const mathUtils::Vec3 escape = selectedFeeler * SelectedFeelerWeight +
           sideTangent * SurfaceTangentWeight +
           planarNormal * ObstacleSeparationBias;
        
        const float escapeLengthSquared = mathUtils::Dot(escape, escape);
        if (!mathUtils::IsFinite(escape) ||
            escapeLengthSquared <= mathUtils::kLengthEpsilonSq)
        {
            return selectedFeeler;
        }
        return escape / std::sqrt(escapeLengthSquared);
    }
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
const GameplayObstacleAvoidanceInput& input,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings,
    GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
{
    GameplayObstacleAvoidanceState state{};
    return ApplyGameplayObstacleAvoidance(input, query, settings, state, debugOut);
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
    const GameplayObstacleAvoidanceInput& input,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings,
    GameplayObstacleAvoidanceState& state,
    GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
{
    const GameplayMovementIntent& baseMovement = input.baseMovement;
    const mathUtils::Vec3& probeOrigin = input.probeOrigin;
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
        state.committedSide = GameplayObstacleAvoidanceSide::None;
        return baseMovement;
    }

    mathUtils::Vec3 forward{baseMovement.moveWorld.x, 0.0f, baseMovement.moveWorld.z};
    const float forwardLengthSquared = mathUtils::Dot(forward, forward);
    if (!mathUtils::IsFinite(forward) || forwardLengthSquared <= mathUtils::kLengthEpsilonSq)
    {
        state.committedSide = GameplayObstacleAvoidanceSide::None;
        return baseMovement;
    }
    forward = forward / std::sqrt(forwardLengthSquared);

    const SanitizedSettings sanitized = Sanitize(input, settings);
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
        sanitized.effectiveClearanceRadius,
        debugOut != nullptr ? &debugOut->forward : nullptr);
    const ProbeResult leftResult = RunProbe(
        query, probeOrigin, left, sanitized.sideDistance,
        sanitized.effectiveClearanceRadius,
        debugOut != nullptr ? &debugOut->left : nullptr);
    const ProbeResult rightResult = RunProbe(
        query, probeOrigin, right, sanitized.sideDistance,
        sanitized.effectiveClearanceRadius,
        debugOut != nullptr ? &debugOut->right : nullptr);

    if (!forwardResult.hit)
    {
        state.committedSide = GameplayObstacleAvoidanceSide::None;
        return baseMovement;
    }
    
    GameplayMovementIntent corrected = baseMovement;
    const mathUtils::Vec3 leftEscape = BuildEscapeCandidate(
        left,
        forward,
        GameplayObstacleAvoidanceSide::Left,
        forwardResult.hitNormal);
    const mathUtils::Vec3 rightEscape = BuildEscapeCandidate(
        right,
        forward,
        GameplayObstacleAvoidanceSide::Right,
        forwardResult.hitNormal);
    const bool leftSupported = HasSupport(query, probeOrigin, leftEscape, sanitized);
    const bool rightSupported = HasSupport(query, probeOrigin, rightEscape, sanitized);
    if (debugOut != nullptr)
    {
        debugOut->leftSupported = leftSupported;
        debugOut->rightSupported = rightSupported;
        debugOut->leftEscapeCandidate = leftEscape;
        debugOut->rightEscapeCandidate = rightEscape;
    }
    if (!leftSupported && !rightSupported)
    {
        state.committedSide = GameplayObstacleAvoidanceSide::None;
        corrected.moveWorld = {};
        corrected.moveMagnitude = 0.0f;
        if (debugOut != nullptr)
        {
            debugOut->active = true;
            debugOut->finalMovement = corrected;
        }
        return corrected;
    }

    // Equal clearance always selects left, keeping symmetric situations deterministic.
    const GameplayObstacleAvoidanceSide preferredSide =
        leftResult.clearance >= rightResult.clearance
        ? GameplayObstacleAvoidanceSide::Left
        : GameplayObstacleAvoidanceSide::Right;
    GameplayObstacleAvoidanceSide chosenSide = preferredSide;
    const GameplayObstacleAvoidanceSide previousSide = state.committedSide;
    if (leftSupported != rightSupported)
    {
        chosenSide = leftSupported
            ? GameplayObstacleAvoidanceSide::Left
            : GameplayObstacleAvoidanceSide::Right;
    }
    else if (state.committedSide == GameplayObstacleAvoidanceSide::Left)
    {
        chosenSide = rightResult.clearance > leftResult.clearance +
                sanitized.sideSwitchClearanceAdvantage
            ? GameplayObstacleAvoidanceSide::Right
            : GameplayObstacleAvoidanceSide::Left;
    }
    else if (state.committedSide == GameplayObstacleAvoidanceSide::Right)
    {
        chosenSide = leftResult.clearance > rightResult.clearance +
                sanitized.sideSwitchClearanceAdvantage
            ? GameplayObstacleAvoidanceSide::Left
            : GameplayObstacleAvoidanceSide::Right;
    }
    state.committedSide = chosenSide;
    const bool preferredUnsupported =
        (preferredSide == GameplayObstacleAvoidanceSide::Left && !leftSupported) ||
        (preferredSide == GameplayObstacleAvoidanceSide::Right && !rightSupported);
    const bool committedUnsupported =
        (previousSide == GameplayObstacleAvoidanceSide::Left && !leftSupported) ||
        (previousSide == GameplayObstacleAvoidanceSide::Right && !rightSupported);
    const bool safetyOverride = leftSupported != rightSupported &&
        (preferredUnsupported || committedUnsupported);
    const bool choseLeft = chosenSide == GameplayObstacleAvoidanceSide::Left;
    corrected.moveWorld = choseLeft ? leftEscape : rightEscape;
    if (debugOut != nullptr)
    {
        const float directionDot = mathUtils::Dot(forward, corrected.moveWorld);
        const bool changedDirection = std::isfinite(directionDot) &&
            directionDot < 1.0f - mathUtils::kLengthEpsilonSq;
        debugOut->active = changedDirection;
        debugOut->preferredSide = changedDirection
            ? preferredSide
            : GameplayObstacleAvoidanceSide::None;
        debugOut->chosenSide = changedDirection
            ? chosenSide
            : GameplayObstacleAvoidanceSide::None;
        debugOut->sideHeldByHysteresis = changedDirection && !safetyOverride &&
            leftSupported && rightSupported &&
            previousSide != GameplayObstacleAvoidanceSide::None &&
            chosenSide == previousSide && chosenSide != preferredSide;
        debugOut->safetyOverride = safetyOverride;
        debugOut->finalMovement = corrected;
    }
    return corrected;
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
    const GameplayMovementIntent& baseMovement,
    const mathUtils::Vec3& probeOrigin,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings,
    GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
{
    return ApplyGameplayObstacleAvoidance(
        {.baseMovement = baseMovement, .probeOrigin = probeOrigin},
        query, settings, debugOut);
}

rendern::GameplayMovementIntent rendern::ApplyGameplayObstacleAvoidance(
    const GameplayMovementIntent& baseMovement,
    const mathUtils::Vec3& probeOrigin,
    const IGameplayObstacleQuery& query,
    const GameplayObstacleAvoidanceSettings& settings,
    GameplayObstacleAvoidanceState& state,
    GameplayObstacleAvoidanceDebugSnapshot* debugOut) noexcept
{
    return ApplyGameplayObstacleAvoidance(
        {.baseMovement = baseMovement, .probeOrigin = probeOrigin},
        query, settings, state, debugOut);
}