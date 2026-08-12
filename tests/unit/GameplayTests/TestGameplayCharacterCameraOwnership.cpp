#include <gtest/gtest.h>

#include <vector>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    [[nodiscard]] EntityHandle CreateCommandCharacter(
        GameplayWorld& world,
        const bool playerControlled,
        const GameplayInputIntentComponent& inputIntent,
        const GameplayCharacterMovementStateComponent& movementState)
    {
        const EntityHandle entity =
            world.CreateEntity();

        world.AddTransform(
            entity,
            GameplayTransformComponent{
                .rotationDegrees = {
                    0.0f,
                    movementState.facingYawDegrees,
                    0.0f
                }
            });

        world.AddInputIntent(entity, inputIntent);

        world.AddCharacterCommand(
            entity,
            GameplayCharacterCommandComponent{});

        world.AddCharacterMotor(
            entity,
            GameplayCharacterMotorComponent{});

        world.AddCharacterMovementState(
            entity,
            movementState);

        if (playerControlled)
        {
            world.AddPlayerControlled(
                entity,
                GameplayPlayerControlledComponent{});
        }

        return entity;
    }

    [[nodiscard]] GameplayUpdateContext MakeCameraContext(
        Scene& scene)
    {
        GameplayUpdateContext context{};
        context.mode = GameplayRuntimeMode::Game;
        context.deltaSeconds = 1.0f / 60.0f;
        context.scene = &scene;
        return context;
    }
}

// Protects camera ownership so command construction updates view-facing yaw
// only for the player-controlled character and leaves ordinary NPC state intact.
TEST(
    GameplayCharacterCameraOwnership,
    CommandBuildWritesCameraFacingOnlyForPlayer)
{
    GameplayWorld world{};

    GameplayCharacterMovementStateComponent playerMovementState{};
    playerMovementState.facingYawDegrees = 0.0f;
    playerMovementState.desiredFacingYawDegrees = 0.0f;
    playerMovementState.cameraFacingYawDegrees = 15.0f;

    GameplayCharacterMovementStateComponent npcMovementState{};
    npcMovementState.facingYawDegrees = 25.0f;
    npcMovementState.desiredFacingYawDegrees = 25.0f;
    npcMovementState.cameraFacingYawDegrees = 137.0f;

    const EntityHandle player =
        CreateCommandCharacter(
            world,
            true,
            GameplayInputIntentComponent{},
            playerMovementState);

    const EntityHandle npc =
        CreateCommandCharacter(
            world,
            false,
            GameplayInputIntentComponent{},
            npcMovementState);

    Scene scene{};
    scene.camera.position = {0.0f, 0.0f, 0.0f};
    scene.camera.target = {1.0f, 0.0f, 0.0f};

    const std::vector<EntityHandle> entities{
        player,
        npc
    };

    BuildGameplayCharacterCommands(
        world,
        entities,
        MakeCameraContext(scene));

    const GameplayCharacterMovementStateComponent* updatedPlayerState =
        world.TryGetCharacterMovementState(player);

    const GameplayCharacterMovementStateComponent* updatedNPCState =
        world.TryGetCharacterMovementState(npc);

    ASSERT_NE(updatedPlayerState, nullptr);
    ASSERT_NE(updatedNPCState, nullptr);

    EXPECT_NEAR(
        updatedPlayerState->cameraFacingYawDegrees,
        90.0f,
        0.001f);

    EXPECT_FLOAT_EQ(
        updatedNPCState->cameraFacingYawDegrees,
        137.0f);

    EXPECT_FLOAT_EQ(
        updatedNPCState->desiredFacingYawDegrees,
        25.0f);
}

// Protects non-player facing ownership so NPC command movement determines body
// orientation without copying the player or debug camera yaw into NPC state.
TEST(
    GameplayCharacterCameraOwnership,
    NonPlayerCommandFacesMovementWithoutWritingCameraYaw)
{
    GameplayWorld world{};

    GameplayCharacterMovementStateComponent movementState{};
    movementState.facingYawDegrees = 0.0f;
    movementState.desiredFacingYawDegrees = 0.0f;
    movementState.cameraFacingYawDegrees = 137.0f;

    const EntityHandle npc =
        CreateCommandCharacter(
            world,
            false,
            GameplayInputIntentComponent{
                .moveX = 1.0f
            },
            movementState);

    Scene scene{};
    scene.camera.position = {0.0f, 0.0f, 0.0f};
    scene.camera.target = {1.0f, 0.0f, 0.0f};

    const std::vector<EntityHandle> entities{npc};

    BuildGameplayCharacterCommands(
        world,
        entities,
        MakeCameraContext(scene));

    const GameplayCharacterCommandComponent* command =
        world.TryGetCharacterCommand(npc);

    const GameplayCharacterMovementStateComponent* updatedState =
        world.TryGetCharacterMovementState(npc);

    ASSERT_NE(command, nullptr);
    ASSERT_NE(updatedState, nullptr);

    EXPECT_LT(command->moveWorld.z, -0.9f);

    EXPECT_NEAR(
        std::abs(
            updatedState->desiredFacingYawDegrees),
        180.0f,
        0.001f);

    EXPECT_FLOAT_EQ(
        updatedState->cameraFacingYawDegrees,
        137.0f);
}

// Protects the AI/camera boundary so an idle NPC never performs player-style
// turn-in-place merely because its stored camera yaw differs from body yaw.
TEST(
    GameplayCharacterCameraOwnership,
    IdleNonPlayerDoesNotTurnTowardCameraFacingYaw)
{
    GameplayWorld world{};

    GameplayCharacterMovementStateComponent movementState{};
    movementState.facingYawDegrees = 10.0f;
    movementState.desiredFacingYawDegrees = 10.0f;
    movementState.previousFacingYawDegrees = 10.0f;
    movementState.cameraFacingYawDegrees = 150.0f;

    const EntityHandle npc =
        CreateCommandCharacter(
            world,
            false,
            GameplayInputIntentComponent{},
            movementState);

    const std::vector<EntityHandle> entities{npc};

    UpdateGameplayCharacterMovement(
        world,
        entities,
        1.0f / 60.0f);

    const GameplayTransformComponent* transform =
        world.TryGetTransform(npc);

    const GameplayCharacterMovementStateComponent* updatedState =
        world.TryGetCharacterMovementState(npc);

    ASSERT_NE(transform, nullptr);
    ASSERT_NE(updatedState, nullptr);

    EXPECT_FALSE(updatedState->turningInPlace);

    EXPECT_FLOAT_EQ(
        updatedState->desiredFacingYawDegrees,
        10.0f);

    EXPECT_FLOAT_EQ(
        updatedState->facingYawDegrees,
        10.0f);

    EXPECT_FLOAT_EQ(
        transform->rotationDegrees.y,
        10.0f);
}

// Protects existing player behavior so restricting camera-facing logic to
// player-controlled entities does not remove camera-driven turn-in-place.
TEST(
    GameplayCharacterCameraOwnership,
    IdlePlayerStillTurnsTowardCameraFacingYaw)
{
    GameplayWorld world{};

    GameplayCharacterMovementStateComponent movementState{};
    movementState.facingYawDegrees = 0.0f;
    movementState.desiredFacingYawDegrees = 0.0f;
    movementState.previousFacingYawDegrees = 0.0f;
    movementState.cameraFacingYawDegrees = 90.0f;

    const EntityHandle player =
        CreateCommandCharacter(
            world,
            true,
            GameplayInputIntentComponent{},
            movementState);

    const std::vector<EntityHandle> entities{player};

    UpdateGameplayCharacterMovement(
        world,
        entities,
        1.0f / 60.0f);

    const GameplayTransformComponent* transform =
        world.TryGetTransform(player);

    const GameplayCharacterMovementStateComponent* updatedState =
        world.TryGetCharacterMovementState(player);

    ASSERT_NE(transform, nullptr);
    ASSERT_NE(updatedState, nullptr);

    EXPECT_TRUE(updatedState->turningInPlace);

    EXPECT_NEAR(
        updatedState->desiredFacingYawDegrees,
        6.0f,
        0.001f);

    EXPECT_NEAR(
        transform->rotationDegrees.y,
        6.0f,
        0.001f);
}

TEST(GameplayCharacterMovement, PhysicsBackedPlayerProducesDesiredVelocityWithoutDirectTranslation)
{
    GameplayWorld world{};
    const EntityHandle player = CreateCommandCharacter(
        world,
        true,
        GameplayInputIntentComponent{},
        GameplayCharacterMovementStateComponent{});
    GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(player);
    GameplayTransformComponent* transform = world.TryGetTransform(player);
    ASSERT_NE(command, nullptr);
    ASSERT_NE(transform, nullptr);
    command->moveWorld = {1.0f, 0.0f, 0.0f};
    command->moveMagnitude = 1.0f;
    const mathUtils::Vec3 initialPosition = transform->position;
    world.AddPhysicsCharacter(player, GameplayPhysicsCharacterComponent{});

    UpdateGameplayCharacterMovement(world, std::vector<EntityHandle>{player}, 0.25f);

    const GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(player);
    ASSERT_NE(motor, nullptr);
    EXPECT_GT(motor->desiredVelocity.x, 0.0f);
    EXPECT_FLOAT_EQ(motor->velocity.x, 0.0f);
    EXPECT_EQ(world.TryGetTransform(player)->position, initialPosition);
}

TEST(GameplayCharacterMovement, ControlledPlayerDoesNotFallbackWhenPhysicsBindingIsMissing)
{
    GameplayWorld world{};
    const EntityHandle player = CreateCommandCharacter(
        world,
        true,
        GameplayInputIntentComponent{},
        GameplayCharacterMovementStateComponent{});
    GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(player);
    ASSERT_NE(command, nullptr);
    command->moveWorld = {1.0f, 0.0f, 0.0f};
    command->moveMagnitude = 1.0f;
    const mathUtils::Vec3 initialPosition = world.TryGetTransform(player)->position;

    UpdateGameplayCharacterMovement(world, std::vector<EntityHandle>{player}, 0.25f);

    EXPECT_GT(world.TryGetCharacterMotor(player)->desiredVelocity.x, 0.0f);
    EXPECT_EQ(world.TryGetTransform(player)->position, initialPosition);
}

TEST(GameplayCharacterMovement, LegacyNonPhysicsCharacterStillIntegratesTransform)
{
    GameplayWorld world{};
    const EntityHandle npc = CreateCommandCharacter(
        world,
        false,
        GameplayInputIntentComponent{},
        GameplayCharacterMovementStateComponent{});
    GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(npc);
    ASSERT_NE(command, nullptr);
    command->moveWorld = {1.0f, 0.0f, 0.0f};
    command->moveMagnitude = 1.0f;

    UpdateGameplayCharacterMovement(world, std::vector<EntityHandle>{npc}, 0.25f);

    const GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(npc);
    const GameplayTransformComponent* transform = world.TryGetTransform(npc);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(transform, nullptr);
    EXPECT_GT(motor->velocity.x, 0.0f);
    EXPECT_GT(transform->position.x, 0.0f);
}

// Protects node-bound ownership so only explicitly player-controlled entities
// receive player markers and follow-camera components during spawning.
TEST(
    GameplayCharacterCameraOwnership,
    NodeBoundSpawnAddsFollowCameraOnlyToPlayer)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset{};
    levelAsset.name = "CameraOwnershipSpawnFixture";

    LevelNode playerNode{};
    playerNode.name = "Player";
    playerNode.alive = true;
    playerNode.visible = false;
    levelAsset.nodes.push_back(playerNode);

    LevelNode npcNode{};
    npcNode.name = "NPC";
    npcNode.alive = true;
    npcNode.visible = false;
    levelAsset.nodes.push_back(npcNode);

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(
        levelAsset,
        levelInstance,
        scene);

    GameplayUpdateContext context{};
    context.mode = GameplayRuntimeMode::Editor;
    context.levelAsset = &levelAsset;
    context.levelInstance = &levelInstance;
    context.scene = &scene;

    const EntityHandle player =
        runtime.SpawnNodeBoundEntity(
            context,
            0,
            true);

    const EntityHandle npc =
        runtime.SpawnNodeBoundEntity(
            context,
            1,
            false);

    ASSERT_NE(player, kNullEntity);
    ASSERT_NE(npc, kNullEntity);

    const GameplayWorld& world = runtime.GetWorld();

    EXPECT_TRUE(world.HasPlayerControlled(player));
    EXPECT_TRUE(world.HasFollowCamera(player));

    EXPECT_FALSE(world.HasPlayerControlled(npc));
    EXPECT_FALSE(world.HasFollowCamera(npc));

    EXPECT_TRUE(world.HasTransform(npc));
    EXPECT_TRUE(world.HasCharacterCommand(npc));
    EXPECT_TRUE(world.HasCharacterMotor(npc));
    EXPECT_TRUE(world.HasCharacterMovementState(npc));
    EXPECT_TRUE(world.HasLocomotion(npc));
}