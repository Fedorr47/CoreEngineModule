#include "GameplayPhysicsCharacterIntegration.h"

#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

import core;

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

bool appRuntime::EnsureControlledGameplayPhysicsCharacter(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld,
    const rendern::LevelAsset& levelAsset)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const rendern::EntityHandle controlledEntity = gameplayRuntime.GetControlledEntity();
    if (controlledEntity == rendern::kNullEntity || !world.IsEntityValid(controlledEntity))
    {
        return true;
    }

    const rendern::GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(controlledEntity);
    const bool bHasInvalidNodeOwner = nodeLink != nullptr &&
        (nodeLink->nodeIndex < 0 ||
            static_cast<std::size_t>(nodeLink->nodeIndex) >= levelAsset.nodes.size() ||
            !levelAsset.nodes[static_cast<std::size_t>(nodeLink->nodeIndex)].alive);
    if (bHasInvalidNodeOwner)
    {
        return DestroyControlledGameplayPhysicsCharacter(gameplayRuntime, physicsWorld);
    }

    if (const rendern::GameplayPhysicsCharacterComponent* binding =
            world.TryGetPhysicsCharacter(controlledEntity))
    {
        if (physicsWorld.IsCharacterValid(binding->character))
        {
            return true;
        }
        world.RemovePhysicsCharacter(controlledEntity);
    }

    const rendern::GameplayTransformComponent* transform = world.TryGetTransform(controlledEntity);
    const rendern::GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(controlledEntity);
    if (transform == nullptr || motor == nullptr)
    {
        return false;
    }

    const physics::PhysicsCharacterDescriptor descriptor{
        .collider = PlayerCharacterCollider,
        .position = transform->position - PlayerVisualRootOffset,
        .maximumSlopeAngleDegrees = PlayerMaximumSlopeAngleDegrees,
        .maximumStepHeight = PlayerMaximumStepHeight,
        .mass = PlayerMass,
        .maximumSpeed = motor->maxRunSpeed
    };
    const physics::PhysicsCharacterHandle character = physicsWorld.CreateCharacter(descriptor);
    if (!character.IsValid())
    {
        return false;
    }

    world.AddPhysicsCharacter(controlledEntity, rendern::GameplayPhysicsCharacterComponent{
        .character = character,
        .visualRootOffset = PlayerVisualRootOffset
    });
    return true;
}

bool appRuntime::SubmitControlledGameplayPhysicsCharacterVelocity(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const rendern::EntityHandle controlledEntity = gameplayRuntime.GetControlledEntity();
    const bool bHasControlledEntity = controlledEntity != rendern::kNullEntity &&
        world.IsEntityValid(controlledEntity);
    if (!bHasControlledEntity)
    {
        return true;
    }

    const rendern::GameplayPhysicsCharacterComponent* binding =
        world.TryGetPhysicsCharacter(controlledEntity);
    const rendern::GameplayCharacterMotorComponent* motor =
        world.TryGetCharacterMotor(controlledEntity);
    if (binding == nullptr || motor == nullptr)
    {
        return false;
    }
    if (!physicsWorld.IsCharacterValid(binding->character))
    {
        world.RemovePhysicsCharacter(controlledEntity);
        return true;
    }
    return physicsWorld.SetCharacterDesiredVelocity(binding->character, motor->desiredVelocity);
}

bool appRuntime::ApplyControlledGameplayPhysicsCharacterFeedback(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const rendern::EntityHandle controlledEntity = gameplayRuntime.GetControlledEntity();
    const bool bHasControlledEntity = controlledEntity != rendern::kNullEntity &&
        world.IsEntityValid(controlledEntity);
    if (!bHasControlledEntity)
    {
        return true;
    }

    rendern::GameplayPhysicsCharacterComponent* binding =
        world.TryGetPhysicsCharacter(controlledEntity);
    rendern::GameplayTransformComponent* transform = world.TryGetTransform(controlledEntity);
    rendern::GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(controlledEntity);
    rendern::GameplayCharacterMovementStateComponent* movementState =
        world.TryGetCharacterMovementState(controlledEntity);
    if (binding == nullptr || transform == nullptr || motor == nullptr || movementState == nullptr)
    {
        return false;
    }
    if (!physicsWorld.IsCharacterValid(binding->character))
    {
        world.RemovePhysicsCharacter(controlledEntity);
        return true;
    }

    const auto position = physicsWorld.GetCharacterPosition(binding->character);
    const auto velocity = physicsWorld.GetCharacterVelocity(binding->character);
    const auto ground = physicsWorld.GetCharacterGroundState(binding->character);
    if (!position.has_value() || !velocity.has_value() || !ground.has_value())
    {
        return false;
    }

    transform->position = *position + binding->visualRootOffset;
    motor->velocity = *velocity;
    // Grounded means usable walkable support; steep support remains distinct.
    movementState->grounded = ground->bIsWalkable;
    movementState->falling = !ground->bIsSupported && velocity->y < 0.0f;
    movementState->jumping = movementState->jumpPhase == rendern::GameplayJumpPhase::Airborne;
    return true;
}

bool appRuntime::DestroyControlledGameplayPhysicsCharacter(
    rendern::GameplayRuntime& gameplayRuntime,
    physics::JoltPhysicsWorld& physicsWorld)
{
    CORE_ASSERT_RUNTIME_THREAD();
    CORE_ASSERT_PHYSICS_THREAD();

    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const rendern::EntityHandle controlledEntity = gameplayRuntime.GetControlledEntity();
    const bool bHasControlledEntity = controlledEntity != rendern::kNullEntity &&
        world.IsEntityValid(controlledEntity);
    if (!bHasControlledEntity)
    {
        return true;
    }

    const rendern::GameplayPhysicsCharacterComponent* binding =
        world.TryGetPhysicsCharacter(controlledEntity);
    if (binding == nullptr)
    {
        return true;
    }

    const physics::PhysicsCharacterHandle character = binding->character;
    if (physicsWorld.IsCharacterValid(character) && !physicsWorld.DestroyCharacter(character))
    {
        return false;
    }
    world.RemovePhysicsCharacter(controlledEntity);
    return true;
}