module;

#include <algorithm>
#include <cmath>
#include <vector>

export module core:character_controller;

import :gameplay;
import :gameplay_runtime_common;
import :math_utils;
import :scene;

export namespace rendern
{
    inline void BuildGameplayPlanarMovementBasis(const Camera& camera, mathUtils::Vec3& outRight, mathUtils::Vec3& outForward) noexcept
    {
        mathUtils::Vec3 forward = camera.target - camera.position;
        forward.y = 0.0f;
        if (mathUtils::Length(forward) <= 1e-6f)
        {
            forward = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            forward = mathUtils::Normalize(forward);
        }

        const mathUtils::Vec3 worldUp(0.0f, 1.0f, 0.0f);
        mathUtils::Vec3 right = mathUtils::Cross(worldUp, forward);
        if (mathUtils::Length(right) <= 1e-6f)
        {
            right = mathUtils::Vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            right = mathUtils::Normalize(right);
        }

        outRight = right;
        outForward = forward;
    }

    [[nodiscard]] inline float ExtractGameplayCameraYawDegrees(const Camera& camera) noexcept
    {
        mathUtils::Vec3 forward = camera.target - camera.position;
        forward.y = 0.0f;
        if (mathUtils::Length(forward) <= 1e-6f)
        {
            forward = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            forward = mathUtils::Normalize(forward);
        }

        return mathUtils::RadToDeg(std::atan2(forward.x, forward.z));
    }


    [[nodiscard]] inline float ExtractGameplayYawDegreesFromDirection(const mathUtils::Vec3& direction) noexcept
    {
        mathUtils::Vec3 planar = direction;
        planar.y = 0.0f;
        if (mathUtils::Length(planar) <= 1e-6f)
        {
            return 0.0f;
        }

        planar = mathUtils::Normalize(planar);
        return mathUtils::RadToDeg(std::atan2(planar.x, planar.z));
    }

    inline void BuildGameplayCharacterCommands(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayUpdateContext& ctx)
    {
        mathUtils::Vec3 moveRight(1.0f, 0.0f, 0.0f);
        mathUtils::Vec3 moveForward(0.0f, 0.0f, 1.0f);
        float cameraYawDegrees = 0.0f;
        if (ctx.scene != nullptr)
        {
            BuildGameplayPlanarMovementBasis(ctx.scene->camera, moveRight, moveForward);
            cameraYawDegrees = ExtractGameplayCameraYawDegrees(ctx.scene->camera);
        }

        for (const EntityHandle entity : entities)
        {
            GameplayInputIntentComponent* intent = world.TryGetInputIntent(entity);
            GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity);
            GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
            if (intent == nullptr || command == nullptr || motor == nullptr)
            {
                continue;
            }

            *command = {};
            command->moveInputX = intent->moveX;
            command->moveInputY = intent->moveY;
            command->wantsRun = intent->runHeld;
            command->actionIntentMask = intent->actionIntentMask;

            mathUtils::Vec3 desiredMove = moveRight * intent->moveX + moveForward * intent->moveY;
            desiredMove.y = 0.0f;
            const float desiredLen = mathUtils::Length(desiredMove);
            if (desiredLen > 1e-6f)
            {
                command->moveWorld = desiredMove / desiredLen;
                command->moveMagnitude = std::clamp(desiredLen, 0.0f, 1.0f);
            }

            motor->desiredMoveWorld = command->moveWorld;

            if (movementState != nullptr)
            {
                movementState->cameraFacingYawDegrees = cameraYawDegrees;

                if (ctx.mode == GameplayRuntimeMode::Game && ctx.scene != nullptr)
                {
                    movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                    if (movementState->jumping && movementState->jumpMovementLocked)
                    {
                        if (mathUtils::Length(movementState->jumpLockedVelocity) > 1e-6f)
                        {
                            movementState->desiredFacingYawDegrees =
                                ExtractGameplayYawDegreesFromDirection(movementState->jumpLockedVelocity);
                        }
                    }
                    else if (command->moveMagnitude > 0.1f)
                    {
                        movementState->desiredFacingYawDegrees = cameraYawDegrees;
                    }
                }
                else
                {
                    movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                    if (command->moveInputY > 0.1f && mathUtils::Length(command->moveWorld) > 1e-6f)
                    {
                        movementState->desiredFacingYawDegrees =
                            ExtractGameplayYawDegreesFromDirection(command->moveWorld);
                    }
                }
            }
        }
    }
}
