#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

#include <cmath>

import core;

#include "GameplayPhysicsCharacterIntegration.h"

namespace
{
    constexpr physics::CharacterColliderDescriptor PlayerCharacterCollider{
        .radius = 0.35f,
        .cylinderHeight = 1.1f
    };

    constexpr mathUtils::Vec3 PlayerVisualRootOffset{
        0.0f,
        -PlayerCharacterCollider.GetTotalHeight() * 0.5f,
        0.0f
    };
    constexpr float PlayerMaximumSlopeAngleDegrees = 45.0f;
    constexpr float PlayerMaximumStepHeight = 0.35f;
    constexpr float PlayerMass = 80.0f;
}

bool appRuntime::EnsureGameplayPhysicsCharacters(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld,
    const rendern::LevelAsset& levelAsset)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    for (const rendern::EntityHandle entity : gameplayRuntime.GetNodeBoundEntities())
    {
        if (!world.IsEntityValid(entity) ||
             (!world.HasPlayerControlled(entity) && !world.HasAI(entity)))
        {
            continue;
        }

        const rendern::GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);
        const bool bHasInvalidNodeOwner = nodeLink != nullptr &&
            (nodeLink->nodeIndex < 0 ||
                static_cast<std::size_t>(nodeLink->nodeIndex) >= levelAsset.nodes.size() ||
                !levelAsset.nodes[static_cast<std::size_t>(nodeLink->nodeIndex)].alive);
        if (bHasInvalidNodeOwner)
        {
            if (const auto* binding = world.TryGetPhysicsCharacter(entity))
            {
                if (physicsWorld.IsCharacterValid(binding->character) &&
                    !physicsWorld.DestroyCharacter(binding->character))
                {
                    return false;
                }
                world.RemovePhysicsCharacter(entity);
            }
            continue;
        }

        if (const auto* binding = world.TryGetPhysicsCharacter(entity))
        {
            if (physicsWorld.IsCharacterValid(binding->character))
            {
                continue;
            }
            world.RemovePhysicsCharacter(entity);
        }
        
        const auto* transform = world.TryGetTransform(entity);
        const auto* motor = world.TryGetCharacterMotor(entity);
        if (transform == nullptr || motor == nullptr)
        {
            return false;
        }

        const bool bIsPlayer = world.HasPlayerControlled(entity);
        const rendern::GameplayCharacterPhysicalSettingsComponent* characterSettings =
            world.TryGetCharacterPhysicalSettings(entity);
        if (!bIsPlayer && characterSettings == nullptr)
        {
            return false;
        }
       
        const mathUtils::Vec3 visualRootOffset = bIsPlayer
            ? PlayerVisualRootOffset
            : mathUtils::Vec3{ 0.0f, -characterSettings->GetTotalHeight() * 0.5f, 0.0f };
        const physics::PhysicsCharacterDescriptor descriptor = bIsPlayer
            ? physics::PhysicsCharacterDescriptor{
                .collider = PlayerCharacterCollider,
                .position = transform->position - visualRootOffset,
                .maximumSlopeAngleDegrees = PlayerMaximumSlopeAngleDegrees,
                .maximumStepHeight = PlayerMaximumStepHeight,
                .mass = PlayerMass,
                .maximumSpeed = motor->maxRunSpeed
            }
        : characterSettings->BuildPhysicsCharacterDescriptor(
            transform->position - visualRootOffset, motor->maxRunSpeed);
        const physics::PhysicsCharacterHandle character = physicsWorld.CreateCharacter(descriptor);
        if (!character.IsValid())
        {
            return false;
        }
        world.AddPhysicsCharacter(entity, rendern::GameplayPhysicsCharacterComponent{
            .character = character,
            .visualRootOffset = visualRootOffset
        });
    }
    
    return true;
}

bool appRuntime::SubmitGameplayPhysicsCharacterVelocities(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    for (const rendern::EntityHandle entity : gameplayRuntime.GetNodeBoundEntities())
    {
        const auto* binding = world.TryGetPhysicsCharacter(entity);
        const auto* motor = world.TryGetCharacterMotor(entity);
        auto* movementState = world.TryGetCharacterMovementState(entity);
        auto* action = world.TryGetAction(entity);
        if (binding == nullptr)
        {
            continue;
        }
        if (motor == nullptr)
        {
            return false;
        }
        if (!physicsWorld.IsCharacterValid(binding->character))
        {
            world.RemovePhysicsCharacter(entity);
            continue;
        }
        if (!physicsWorld.SetCharacterDesiredVelocity(binding->character, motor->desiredVelocity))
        {
            return false;
        }
        if (movementState != nullptr &&
            movementState->jumpPhase == rendern::GameplayJumpPhase::Preparing)
        {
            const bool accepted = physicsWorld.RequestCharacterJump(
                binding->character, motor->jumpVerticalSpeed);
            movementState->jumpRequestResult = accepted
                ? rendern::GameplayJumpRequestResult::Accepted
                : rendern::GameplayJumpRequestResult::Rejected;
            movementState->jumpAirbornePhysicallyObserved = false;
            if (accepted)
            {
                movementState->jumpPhase = rendern::GameplayJumpPhase::Airborne;
                movementState->grounded = false;
                movementState->jumping = true;
            }
            else
            {
                movementState->jumpPhase = rendern::GameplayJumpPhase::None;
                movementState->jumpLockedVelocity = {};
                movementState->jumping = false;
                if (action != nullptr &&
                    rendern::GetGameplayRequestedActionKind(*action) ==
                        rendern::GameplayActionKind::Jump)
                {
                    rendern::ClearGameplayActionRequest(action->pending);
                    action->pendingDispatched = false;
                }
                if (action != nullptr &&
                    action->current == rendern::GameplayActionKind::Jump)
                {
                    rendern::FinishGameplayActionState(*action);
                }
            }
        }
    }
    
    return true;
}

bool appRuntime::ApplyGameplayPhysicsCharacterFeedback(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();
    
    constexpr float kBlockedIntentSpeed = 0.1f;
    constexpr float kBlockedProgressRatio = 0.1f;
    constexpr float kBlockedPersistenceSeconds = 0.25f;
    constexpr float kFixedDeltaSeconds = 1.0f / 60.0f;
    
    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();

    for (const rendern::EntityHandle entity : gameplayRuntime.GetNodeBoundEntities())
    {
        auto* binding = world.TryGetPhysicsCharacter(entity);
        if (binding == nullptr)
        {
            continue;
        }
        auto* transform = world.TryGetTransform(entity);
        auto* motor = world.TryGetCharacterMotor(entity);
        auto* movementState = world.TryGetCharacterMovementState(entity);
        if (transform == nullptr || motor == nullptr || movementState == nullptr)
        {
            return false;
        }
        if (!physicsWorld.IsCharacterValid(binding->character))
        {
            world.RemovePhysicsCharacter(entity);
            continue;
        }

        const auto position = physicsWorld.GetCharacterPosition(binding->character);
        const auto velocity = physicsWorld.GetCharacterVelocity(binding->character);
        const auto ground = physicsWorld.GetCharacterGroundState(binding->character);
        if (!position || !velocity || !ground)
        {
            return false;
        }
        const auto motionObservation =
            physicsWorld.ConsumeCharacterMotionObservation(binding->character);
        if (!motionObservation)
        {
            return false;
        }
        
        transform->position = *position + binding->visualRootOffset;
        motor->velocity = *velocity;
        movementState->grounded = ground->bIsWalkable;
        movementState->falling = !ground->bIsSupported && velocity->y < 0.0f;
        movementState->jumping = movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne;
        if (movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne &&
            !ground->bIsSupported)
        {
            movementState->jumpAirbornePhysicallyObserved = true;
        }
        if (ground->bIsSupported &&
            movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne &&
            movementState->jumpAirbornePhysicallyObserved)
        {
            movementState->jumpPhase = rendern::GameplayJumpPhase::None;
            movementState->jumpAirbornePhysicallyObserved = false;
            movementState->jumpLockedVelocity = {};
            movementState->jumping = false;
        }

        const float desiredPlanarSpeed = std::hypot(motor->desiredVelocity.x, motor->desiredVelocity.z);
        for (std::uint32_t stepIndex = 0u;
            stepIndex < motionObservation->stepCount; ++stepIndex)
        {
            const physics::CharacterMotionStepObservation& step =
                motionObservation->steps[stepIndex];
            const float actualPlanarSpeed = std::hypot(
                step.displacement.x, step.displacement.z) / kFixedDeltaSeconds;
            const bool bInsufficientProgress = step.bIsSupported &&
                desiredPlanarSpeed > kBlockedIntentSpeed &&
                actualPlanarSpeed < desiredPlanarSpeed * kBlockedProgressRatio;
            movementState->physicalBlockedSeconds = bInsufficientProgress
                ? movementState->physicalBlockedSeconds + kFixedDeltaSeconds
                : 0.0f;
        }
        movementState->physicallyBlocked =
            movementState->physicalBlockedSeconds >= kBlockedPersistenceSeconds;
    }
    
    return true;
}

bool appRuntime::DestroyGameplayPhysicsCharacters(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    for (const rendern::EntityHandle entity : gameplayRuntime.GetNodeBoundEntities())
    {
        const auto* binding = world.TryGetPhysicsCharacter(entity);
        if (binding == nullptr)
        {
            continue;
        }
        const physics::PhysicsCharacterHandle character = binding->character;
        if (physicsWorld.IsCharacterValid(character) && !physicsWorld.DestroyCharacter(character))
        {
            return false;
        }
        world.RemovePhysicsCharacter(entity);
    }

    return true;
}

bool appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld,
    const rendern::EntityHandle entity)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* transform = world.TryGetTransform(entity);
    const auto* binding = world.TryGetPhysicsCharacter(entity);
    if (transform == nullptr || binding == nullptr ||
        !physicsWorld.IsCharacterValid(binding->character))
    {
        return false;
    }
    return physicsWorld.TeleportCharacter(
        binding->character,
        transform->position - binding->visualRootOffset);
}