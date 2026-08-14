module;

#include <algorithm>
#include <cmath>
#include <vector>

export module core:character_movement;

import :gameplay;
import :math_utils;

export namespace rendern
{
    struct CharacterMovementUpdateAccess
    {
        GameplayTransformComponent* transform{};
        GameplayCharacterMotorComponent* motor{};
        GameplayCharacterCommandComponent* command{};
        GameplayCharacterMovementStateComponent* movementState{};
        GameplayActionComponent* action{};
    };
    
    struct CharacterLocomotionAccess
    {
        const GameplayTransformComponent* transform{};
        const GameplayCharacterMotorComponent* motor{};
        const GameplayCharacterCommandComponent* command{};
        GameplayCharacterMovementStateComponent* movementState{};
        GameplayLocomotionComponent* locomotion{};
    };

    [[nodiscard]] inline CharacterMovementUpdateAccess TryGetCharacterMovementUpdateAccess(
        GameplayWorld& world,
        const EntityHandle entity) noexcept
    {
        return CharacterMovementUpdateAccess{
            .transform = world.TryGetTransform(entity),
            .motor = world.TryGetCharacterMotor(entity),
            .command = world.TryGetCharacterCommand(entity),
            .movementState = world.TryGetCharacterMovementState(entity),
            .action = world.TryGetAction(entity)
        };
    }
    
    [[nodiscard]] inline CharacterLocomotionAccess TryGetCharacterLocomotionAccess(
        GameplayWorld& world,
        const EntityHandle entity) noexcept
    {
        return CharacterLocomotionAccess{
            .transform = world.TryGetTransform(entity),
            .motor = world.TryGetCharacterMotor(entity),
            .command = world.TryGetCharacterCommand(entity),
            .movementState = world.TryGetCharacterMovementState(entity),
            .locomotion = world.TryGetLocomotion(entity)
        };
    }
    
    [[nodiscard]] inline float NormalizeGameplayYawDeltaDegrees_(float degrees) noexcept
    {
        while (degrees > 180.0f)
        {
            degrees -= 360.0f;
        }
        while (degrees < -180.0f)
        {
            degrees += 360.0f;
        }
        return degrees;
    }

    inline mathUtils::Vec3 MoveVelocityTowards_(
        const mathUtils::Vec3& current,
        const mathUtils::Vec3& target,
        const float maxDelta) noexcept
    {
        const mathUtils::Vec3 delta = target - current;
        if (maxDelta <= mathUtils::kLengthEpsilon)
        {
            return target;
        }

        const float deltaLenSq = mathUtils::Dot(delta, delta);
        const float maxDeltaSq = maxDelta * maxDelta;
        if (deltaLenSq <= maxDeltaSq)
        {
            return target;
        }

        const float deltaLen = std::sqrt(deltaLenSq);
        return current + (delta * (maxDelta / deltaLen));
    }

    void UpdateGameplayCharacterMovement(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const float deltaSeconds)
    {
        constexpr float kTurnInPlaceActivateDegrees = 40.0f;
        constexpr float kTurnInPlaceStopDegrees = 10.0f;
        constexpr float kTurnInPlaceSpeedDegreesPerSecond = 360.0f;

        const float dt = std::max(deltaSeconds, 0.0f);
        for (const EntityHandle entity : entities)
        {
            const CharacterMovementUpdateAccess access = TryGetCharacterMovementUpdateAccess(world, entity);
            GameplayTransformComponent* transform = access.transform;
            GameplayCharacterCommandComponent* command = access.command;
            GameplayCharacterMotorComponent* motor = access.motor;
            GameplayCharacterMovementStateComponent*  movementState = access.movementState;
            GameplayActionComponent* action = access.action;
            const bool bUsesPhysicsTranslation =
                world.HasPhysicsCharacter(entity) || world.HasPlayerControlled(entity);
            
            if (transform == nullptr || motor == nullptr || command == nullptr)
            {
                continue;
            }

            const bool hasPendingJumpRequest = action != nullptr &&
                GetGameplayRequestedActionKind(*action) == GameplayActionKind::Jump;
            if (movementState != nullptr && !hasPendingJumpRequest)
            {
                movementState->jumpRequestConsumed = false;
            }
            const bool jumpRequested = hasPendingJumpRequest &&
                movementState != nullptr && !movementState->jumpRequestConsumed;

            float speedScale = 1.0f;
            if (command->moveInputY < -0.1f)
            {
                speedScale = motor->backwardSpeedScale;
            }

            const float baseTargetSpeed = command->wantsRun ? motor->maxRunSpeed : motor->maxWalkSpeed;
            const float targetSpeed = baseTargetSpeed * speedScale;
            const mathUtils::Vec3 targetVelocity = command->moveWorld * (targetSpeed * command->moveMagnitude);

            if (movementState != nullptr &&
                jumpRequested &&
                movementState->jumpPhase == GameplayJumpPhase::None)
            {
                // Capture the actual planar momentum at jump preparation start.
                // Ground movement continues until the explicit physical attempt.
                movementState->jumpLockedVelocity = motor->velocity;
                movementState->jumpPhase = GameplayJumpPhase::Preparing;
                movementState->jumpRequestConsumed = true;
            }

            const bool bIsAirborne = movementState != nullptr &&
                movementState->jumpPhase == GameplayJumpPhase::Airborne;
            const bool bIsPreparing = movementState != nullptr &&
                movementState->jumpPhase == GameplayJumpPhase::Preparing;

            if (bIsAirborne)
            {
                const float airDeceleration =
                    std::max(motor->airDeceleration, 0.0f) * dt;

                const mathUtils::Vec3 currentVelocity = bUsesPhysicsTranslation
                     ? motor->desiredVelocity
                     : motor->velocity;
                motor->desiredVelocity = MoveVelocityTowards_(
                    currentVelocity,
                    mathUtils::Vec3(0.0f, 0.0f, 0.0f),
                    airDeceleration);
            }
            else
            {
                const mathUtils::Vec3 currentVelocity = bUsesPhysicsTranslation
                    ? motor->desiredVelocity
                    : motor->velocity;
                const float currentSpeedSq = mathUtils::Dot(currentVelocity, currentVelocity);
                const float desiredSpeedSq = mathUtils::Dot(targetVelocity, targetVelocity);

                const float rate =
                    desiredSpeedSq > currentSpeedSq
                        ? motor->acceleration
                        : motor->deceleration;

                const float maxDelta = std::max(rate, 0.0f) * dt;

                motor->desiredVelocity = MoveVelocityTowards_(
                     currentVelocity,
                    targetVelocity,
                    maxDelta);
            }

            if (!bUsesPhysicsTranslation)
            {
                motor->velocity = motor->desiredVelocity;
                transform->position = transform->position + motor->velocity * dt;
            }

            if (movementState != nullptr)
            {
                const bool bHasMoveMagnitude = command->moveMagnitude > mathUtils::kLengthEpsilon;
                const bool bHasMoveDirection = mathUtils::Length(command->moveWorld) > mathUtils::kLengthEpsilon;
                const bool bHasMoveInput = bHasMoveMagnitude && bHasMoveDirection;
                const bool bIsPlayerControlled = world.HasPlayerControlled(entity);

                const bool bCanUseCameraTurnInPlace =
                    bIsPlayerControlled 
                && !bIsAirborne 
                && !bIsPreparing 
                && !bHasMoveInput;

                if (bCanUseCameraTurnInPlace)
                {
                    const float aimYawDeltaDegrees =
                        NormalizeGameplayYawDeltaDegrees_(
                            movementState->cameraFacingYawDegrees -
                            movementState->facingYawDegrees);

                    const bool bShouldStartTurningInPlace =
                        !movementState->turningInPlace &&
                        std::abs(aimYawDeltaDegrees) >=
                            kTurnInPlaceActivateDegrees;

                    if (bShouldStartTurningInPlace)
                    {
                        movementState->turningInPlace = true;
                    }

                    if (movementState->turningInPlace)
                    {
                        const bool bHasReachedCameraFacing =
                            std::abs(aimYawDeltaDegrees) <=
                            kTurnInPlaceStopDegrees;

                        if (bHasReachedCameraFacing)
                        {
                            movementState->turningInPlace = false;

                            movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                        }
                        else
                        {
                            const float maxTurnStep = kTurnInPlaceSpeedDegreesPerSecond * dt;

                            movementState->desiredFacingYawDegrees =
                                movementState->facingYawDegrees +
                                std::clamp(
                                    aimYawDeltaDegrees,
                                    -maxTurnStep,
                                    maxTurnStep);
                        }
                    }
                }
                else
                {
                    // AI and other non-player characters never turn in place in
                    // response to the player or debug camera.
                    movementState->turningInPlace = false;
                }

                transform->rotationDegrees.y = movementState->desiredFacingYawDegrees;
                movementState->facingYawDegrees = transform->rotationDegrees.y;

                if (!bUsesPhysicsTranslation)
                {
                    movementState->jumping = bIsAirborne;
                    movementState->grounded = !bIsAirborne;
                    movementState->falling = false;
                }
            }
        }
    }

    inline void UpdateGameplayCharacterLocomotion(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities)
    {
        for (const EntityHandle entity : entities)
        {
            const CharacterLocomotionAccess access = TryGetCharacterLocomotionAccess(world, entity);
            const GameplayCharacterCommandComponent* command = access.command;
            const GameplayCharacterMotorComponent* motor = access.motor;
            GameplayCharacterMovementStateComponent* movementState = access.movementState;
            const GameplayTransformComponent* transform = access.transform;
            GameplayLocomotionComponent* locomotion = access.locomotion;

            if (transform == nullptr || motor == nullptr || command == nullptr || locomotion == nullptr)
            {
                continue;
            }

            const mathUtils::Vec3 planarVelocity(motor->velocity.x, 0.0f, motor->velocity.z);
            const float planarSpeedSq = mathUtils::Dot(planarVelocity, planarVelocity);
            const float planarSpeed = std::sqrt(planarSpeedSq);
            const bool isMoving = planarSpeedSq > mathUtils::kMoveEpsilonSq;
            const float yawRadians = mathUtils::DegToRad(transform->rotationDegrees.y);
            const mathUtils::Vec3 actorForward(std::sin(yawRadians), 0.0f, std::cos(yawRadians));
            const mathUtils::Vec3 actorRight(-actorForward.z, 0.0f, actorForward.x);

            locomotion->forwardSpeed = mathUtils::Dot(planarVelocity, actorForward);
            locomotion->rightSpeed = mathUtils::Dot(planarVelocity, actorRight);
            locomotion->planarSpeed = planarSpeed;
            locomotion->isMoving = isMoving;
            locomotion->isRunning = isMoving && command->wantsRun;

            const float normalizationSpeed = std::max(command->wantsRun ? motor->maxRunSpeed : motor->maxWalkSpeed, 1.0f);
            const bool hasMoveIntent = command->moveMagnitude > mathUtils::kMoveEpsilon &&
                mathUtils::Dot(command->moveWorld, command->moveWorld) > mathUtils::kLengthEpsilonSq;
            if (hasMoveIntent)
            {
                const mathUtils::Vec3 moveIntent = command->moveWorld * command->moveMagnitude;
                locomotion->moveX = std::clamp(mathUtils::Dot(moveIntent, actorRight), -1.0f, 1.0f);
                locomotion->moveY = std::clamp(mathUtils::Dot(moveIntent, actorForward), -1.0f, 1.0f);
            }
            else
            {
                locomotion->moveX = std::clamp(locomotion->rightSpeed / normalizationSpeed, -1.0f, 1.0f);
                locomotion->moveY = std::clamp(locomotion->forwardSpeed / normalizationSpeed, -1.0f, 1.0f);
            }

            float turnDeltaYawDegrees = 0.0f;
            if (movementState != nullptr)
            {
                const float actualTurnDeltaYawDegrees = NormalizeGameplayYawDeltaDegrees_(
                    transform->rotationDegrees.y - movementState->previousFacingYawDegrees);
                const float aimYawDeltaDegrees = NormalizeGameplayYawDeltaDegrees_(
                    movementState->cameraFacingYawDegrees - transform->rotationDegrees.y);

                locomotion->wantsTurnInPlaceLeft = !isMoving && !movementState->jumping &&
                    movementState->turningInPlace && aimYawDeltaDegrees < -10.0f;
                locomotion->wantsTurnInPlaceRight = !isMoving && !movementState->jumping &&
                    movementState->turningInPlace && aimYawDeltaDegrees > 10.0f;

                turnDeltaYawDegrees = movementState->turningInPlace ? aimYawDeltaDegrees : actualTurnDeltaYawDegrees;
                movementState->previousFacingYawDegrees = transform->rotationDegrees.y;
            }
            else
            {
                locomotion->wantsTurnInPlaceLeft = false;
                locomotion->wantsTurnInPlaceRight = false;
            }

            locomotion->turnDeltaYawDegrees = turnDeltaYawDegrees;
        }
    }
}
