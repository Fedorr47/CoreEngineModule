module;

#include <algorithm>
#include <cmath>
#include <vector>

export module core:character_movement;

import :gameplay;
import :math_utils;

export namespace rendern
{
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

    inline void UpdateGameplayCharacterMovement(
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
            GameplayTransformComponent* transform = world.TryGetTransform(entity);
            GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity);
            GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (transform == nullptr || motor == nullptr || command == nullptr)
            {
                continue;
            }

            const bool jumping = action != nullptr &&
                (action->current == GameplayActionKind::Jump || GetGameplayRequestedActionKind(*action) == GameplayActionKind::Jump);

            float speedScale = 1.0f;
            if (command->moveInputY < -0.1f)
            {
                speedScale = motor->backwardSpeedScale;
            }

            const float baseTargetSpeed = command->wantsRun ? motor->maxRunSpeed : motor->maxWalkSpeed;
            const float targetSpeed = baseTargetSpeed * speedScale;
            const mathUtils::Vec3 targetVelocity = command->moveWorld * (targetSpeed * command->moveMagnitude);

            if (jumping)
            {
                if (movementState != nullptr && !movementState->jumping)
                {
                    mathUtils::Vec3 launchVelocity = motor->velocity;
                    if (mathUtils::Dot(targetVelocity, targetVelocity) > mathUtils::kMoveEpsilonSq)
                    {
                        launchVelocity = targetVelocity;
                    }

                    movementState->jumpLockedVelocity = launchVelocity;
                    movementState->jumpMovementLocked = true;
                    motor->velocity = launchVelocity;
                }

                const float airDeceleration = std::max(motor->airDeceleration, 0.0f) * dt;
                motor->velocity = MoveVelocityTowards_(motor->velocity, mathUtils::Vec3(0.0f, 0.0f, 0.0f), airDeceleration);
            }
            else
            {
                const float currentSpeedSq = mathUtils::Dot(motor->velocity, motor->velocity);
                const float desiredSpeedSq = mathUtils::Dot(targetVelocity, targetVelocity);
                const float rate = desiredSpeedSq > currentSpeedSq ? motor->acceleration : motor->deceleration;
                const float maxDelta = std::max(rate, 0.0f) * dt;
                motor->velocity = MoveVelocityTowards_(motor->velocity, targetVelocity, maxDelta);

                if (movementState != nullptr)
                {
                    movementState->jumpMovementLocked = false;
                    movementState->jumpLockedVelocity = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
                }
            }

            transform->position = transform->position + motor->velocity * dt;

            if (movementState != nullptr)
            {
                movementState->previousFacingYawDegrees = movementState->facingYawDegrees;

                const bool hasMoveInput = command->moveMagnitude > 0.1f;
                if (!jumping && !hasMoveInput)
                {
                    const float aimYawDeltaDegrees = NormalizeGameplayYawDeltaDegrees_(
                        movementState->cameraFacingYawDegrees - movementState->facingYawDegrees);

                    if (!movementState->turningInPlace &&
                        std::abs(aimYawDeltaDegrees) >= kTurnInPlaceActivateDegrees)
                    {
                        movementState->turningInPlace = true;
                    }

                    if (movementState->turningInPlace)
                    {
                        if (std::abs(aimYawDeltaDegrees) <= kTurnInPlaceStopDegrees)
                        {
                            movementState->turningInPlace = false;
                            movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                        }
                        else
                        {
                            const float maxStep = kTurnInPlaceSpeedDegreesPerSecond * dt;
                            movementState->desiredFacingYawDegrees = movementState->facingYawDegrees +
                                std::clamp(aimYawDeltaDegrees, -maxStep, maxStep);
                        }
                    }
                }
                else
                {
                    movementState->turningInPlace = false;
                }

                transform->rotationDegrees.y = movementState->desiredFacingYawDegrees;
                movementState->facingYawDegrees = transform->rotationDegrees.y;
                movementState->jumping = jumping;
                movementState->grounded = !jumping;
                movementState->falling = false;
            }
        }
    }

    inline void UpdateGameplayCharacterLocomotion(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities)
    {
        for (const EntityHandle entity : entities)
        {
            const GameplayTransformComponent* transform = world.TryGetTransform(entity);
            const GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity);
            const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
            GameplayLocomotionComponent* locomotion = world.TryGetLocomotion(entity);
            if (transform == nullptr || motor == nullptr || command == nullptr || locomotion == nullptr)
            {
                continue;
            }

            const float planarSpeedSq = mathUtils::Dot(motor->velocity, motor->velocity);
            const float planarSpeed = std::sqrt(planarSpeedSq);
            const bool isMoving = planarSpeedSq > mathUtils::kMoveEpsilonSq;
            const float yawRadians = mathUtils::DegToRad(transform->rotationDegrees.y);
            const mathUtils::Vec3 actorForward(std::sin(yawRadians), 0.0f, std::cos(yawRadians));
            const mathUtils::Vec3 actorRight(-actorForward.z, 0.0f, actorForward.x);

            locomotion->forwardSpeed = mathUtils::Dot(motor->velocity, actorForward);
            locomotion->rightSpeed = mathUtils::Dot(motor->velocity, actorRight);
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
