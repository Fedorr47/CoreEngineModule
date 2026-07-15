module;

#include <algorithm>
#include <cmath>
#include <cstdint>

export module core:gameplay_steering;

import :math_utils;
import :gameplay;

export namespace rendern
{
    struct GameplayMovementIntent
    {
        mathUtils::Vec3 moveWorld{0.0f, 0.0f, 0.0f};
        float moveMagnitude{0.0f};
        bool wantsRun{false};
        
        [[nodiscard]] bool IsMoving() const noexcept
        {
            return moveMagnitude > mathUtils::kMoveEpsilon &&
                mathUtils::Dot(moveWorld, moveWorld) > mathUtils::kLengthEpsilon;
        }
    };
    
    enum class GameplaySteeringStatus : uint8_t
    {
        Moving,
        Arrived
    };
    
    struct GameplayArrivalSteeringSettings
    {
        float acceptanceRadius{0.25f};
        float slowingRadius{1.0f};
        bool wantsRun{false};
    };
    
    struct GameplaySteeringOutput
    {
        GameplayMovementIntent movement{};
        GameplaySteeringStatus status{GameplaySteeringStatus::Arrived};
        float remainingDistance{0.0f};
    };
    
    [[nodiscard]] GameplaySteeringOutput BuildGameplayArrivalSteering(
        const mathUtils::Vec3& currentPosition,
        const mathUtils::Vec3& targetPosition,
        const GameplayArrivalSteeringSettings& settings = {}) noexcept
    {
        mathUtils::Vec3 planarDelta = targetPosition - currentPosition;
        planarDelta.y = 0.0f;
        
        const float planarDistance = mathUtils::Length(planarDelta);
        const float acceptanceRadius = std::max(settings.acceptanceRadius, 0.0f);
        const float slowingRadius = std::max(settings.slowingRadius, acceptanceRadius);
        const bool isDegenerateDelta = planarDistance <= mathUtils::kLengthEpsilon;
        const bool isInsideAcceptanceRadius = planarDistance <= acceptanceRadius;
        
        GameplaySteeringOutput output{};
        output.remainingDistance = std::max(planarDistance, 0.0f);
        
        if (isInsideAcceptanceRadius || isDegenerateDelta)
        {
            return output;
        }
        
        output.status = GameplaySteeringStatus::Moving;
        output.movement.moveWorld = planarDelta / planarDistance;
        output.movement.wantsRun = settings.wantsRun;
        
        const bool hasSlowingInterval = slowingRadius >= acceptanceRadius;
        if (!hasSlowingInterval || planarDistance >= slowingRadius)
        {
            output.movement.moveMagnitude = 1.0f;
            return output;
        }
        
        output.movement.moveMagnitude = std::clamp(
        (planarDistance - acceptanceRadius) / (slowingRadius - acceptanceRadius), 
        0.0f, 1.0f);
        return output;
    }
    
    void ApplyGaeplayMovementIntent(
        const GameplayMovementIntent& intent,
        GameplayCharacterCommandComponent& command) noexcept
    {
        mathUtils::Vec3 planarDirection = intent.moveWorld;
        planarDirection.y = 0.0f;
        
        const float directionLengthSquare = mathUtils::Dot(planarDirection, planarDirection);
        const float magnitude = std::clamp(intent.moveMagnitude, 0.0f, 1.0f);
        const bool hasDirection = directionLengthSquare > mathUtils::kLengthEpsilonSq;
        const bool hasMagnitude = magnitude > mathUtils::kMoveEpsilon;
        
        command.moveInputX = 0.0f;
        command.moveInputY = 0.0f;
        
        if (!hasDirection || !hasMagnitude)
        {
            command.moveWorld = mathUtils::Vec3{0.0f, 0.0f, 0.0f};
            command.moveMagnitude = 0.0f;
            command.wantsRun = false;
            return;
        }
        
        command.moveWorld = planarDirection / std::sqrt(directionLengthSquare);
        command.moveMagnitude = magnitude;
        command.wantsRun = intent.wantsRun;
    }
}
