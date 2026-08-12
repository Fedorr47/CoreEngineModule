#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <optional>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

import core;
import std;

#include "App/GameplayPhysicsCharacterIntegration.h"
#include "App/AppLifecycle.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"
#include "TestSupport/TestThreadAffinity.h"

namespace
{
    constexpr float StepSeconds = 1.0f / 60.0f;

    physics::PhysicsBodyDescriptor FloorDescriptor()
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 10.0f, 0.5f, 10.0f } },
            .transform = { .position = { 0.0f, -0.5f, 0.0f } },
            .motionType = physics::PhysicsMotionType::Static
        };
    }

    class GameplayPhysicsCharacterIntegrationTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            threadGuard.emplace();
            threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
            ASSERT_TRUE(joltRuntime.Initialize());
            ASSERT_TRUE(physicsWorld.Initialize());

            levelAsset.name = "CR443GameplayPhysicsIntegration";
            rendern::LevelNode playerNode{};
            playerNode.name = "Player";
            playerNode.alive = true;
            playerNode.visible = false;
            levelAsset.nodes.push_back(playerNode);
            scene.camera.position = {0.0f, 2.0f, -5.0f};
            scene.camera.target = {0.0f, 0.0f, 0.0f};
            gameplayRuntime.Initialize(levelAsset, levelInstance, scene);

            editorContext.mode = rendern::GameplayRuntimeMode::Editor;
            editorContext.deltaSeconds = StepSeconds;
            editorContext.levelAsset = &levelAsset;
            editorContext.levelInstance = &levelInstance;
            editorContext.scene = &scene;
            gameContext = editorContext;
            gameContext.mode = rendern::GameplayRuntimeMode::Game;

            player = gameplayRuntime.SpawnNodeBoundEntity(editorContext, 0, true);
            ASSERT_NE(player, rendern::kNullEntity);
        }

        void TearDown() override
        {
            EXPECT_TRUE(appRuntime::DestroyControlledGameplayPhysicsCharacter(
                gameplayRuntime, physicsWorld));
            gameplayRuntime.Shutdown();
            physicsWorld.Shutdown();
            joltRuntime.Shutdown();
            threadGuard.reset();
        }

        void SetMoveIntent(const float moveY)
        {
            gameplayRuntime.BindIntentSource(
                player,
                [moveY](rendern::EntityHandle,
                    const rendern::GameplayUpdateContext&,
                    rendern::GameplayWorld&,
                    rendern::GameplayInputIntentComponent& intent,
                    rendern::GameplayActionComponent*)
                {
                    intent.moveY = moveY;
                });
        }

        void StepFrame()
        {
            // Mirrors the app's CR-443 gameplay/physics phase order.
            gameplayRuntime.BeginFrame();
            ASSERT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
                gameplayRuntime, physicsWorld, levelAsset));
            gameplayRuntime.PrePhysicsUpdate(gameContext);
            ASSERT_TRUE(appRuntime::SubmitControlledGameplayPhysicsCharacterVelocity(
                gameplayRuntime, physicsWorld));
            ASSERT_EQ(physicsWorld.Update(StepSeconds), 1u);
            ASSERT_TRUE(appRuntime::ApplyControlledGameplayPhysicsCharacterFeedback(
                gameplayRuntime, physicsWorld));
            gameplayRuntime.PostPhysicsUpdate(gameContext);
        }

        std::optional<InlineThreadOwnerRolesGuard> threadGuard{};
        physics::JoltRuntime joltRuntime{};
        physics::JoltPhysicsWorld physicsWorld{joltRuntime};
        rendern::GameplayRuntime gameplayRuntime{};
        rendern::LevelAsset levelAsset{};
        rendern::LevelInstance levelInstance{};
        rendern::Scene scene{};
        rendern::GameplayUpdateContext editorContext{};
        rendern::GameplayUpdateContext gameContext{};
        rendern::EntityHandle player{rendern::kNullEntity};
    };
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ForwardMovementFeedsBackActualStateAndPreservesVisualOffset)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    SetMoveIntent(1.0f);
    for (int frame = 0; frame < 90; ++frame)
    {
        StepFrame();
    }

    const rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(player);
    const auto* transform = world.TryGetTransform(player);
    const auto* motor = world.TryGetCharacterMotor(player);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    const auto center = physicsWorld.GetCharacterPosition(binding->character);
    const auto actualVelocity = physicsWorld.GetCharacterVelocity(binding->character);
    ASSERT_TRUE(center.has_value());
    ASSERT_TRUE(actualVelocity.has_value());
    EXPECT_GT(transform->position.z, 1.0f);
    EXPECT_NEAR(transform->position.y - center->y, binding->visualRootOffset.y, 0.001f);
    EXPECT_NEAR(motor->velocity.z, actualVelocity->z, 0.001f);
    EXPECT_GT(motor->desiredVelocity.z, 0.0f);
    const auto* followCamera = world.TryGetFollowCamera(player);
    ASSERT_NE(followCamera, nullptr);
    EXPECT_NEAR(scene.camera.target.z, transform->position.z + followCamera->focusOffset.z, 0.001f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, CreationConvertsVisualRootToCapsuleCenterExplicitly)
{
    ASSERT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, levelAsset));
    const rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(player);
    const auto* transform = world.TryGetTransform(player);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(transform, nullptr);
    const auto center = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(center.has_value());
    EXPECT_NEAR(center->y, transform->position.y - binding->visualRootOffset.y, 0.001f);
    EXPECT_NEAR(binding->visualRootOffset.y, -0.9f, 0.001f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, StaticWallConstrainsActualMovementButNotDesiredVelocity)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const physics::PhysicsBodyDescriptor wall{
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 2.0f } },
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(wall).IsValid());
    SetMoveIntent(1.0f);
    for (int frame = 0; frame < 180; ++frame)
    {
        StepFrame();
    }

    const rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* transform = world.TryGetTransform(player);
    const auto* motor = world.TryGetCharacterMotor(player);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    EXPECT_GT(motor->desiredVelocity.z, 1.5f);
    EXPECT_LT(transform->position.z, 1.5f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, GravityFeedsFallingThenLandingAndKeepsVisualOffset)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    gameplayRuntime.GetWorld().TryGetTransform(player)->position.y = 3.0f;
    bool bObservedFalling = false;
    bool bObservedVerticalOnlyLocomotion = false;
    for (int frame = 0; frame < 240; ++frame)
    {
        StepFrame();
        const auto* movementState = gameplayRuntime.GetWorld().TryGetCharacterMovementState(player);
        ASSERT_NE(movementState, nullptr);
        bObservedFalling = bObservedFalling || movementState->falling;
        const auto* locomotion = gameplayRuntime.GetWorld().TryGetLocomotion(player);
        ASSERT_NE(locomotion, nullptr);
        if (movementState->falling)
        {
            bObservedVerticalOnlyLocomotion = bObservedVerticalOnlyLocomotion ||
                (!locomotion->isMoving && locomotion->planarSpeed < 0.01f);
        }
    }

    const rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(player);
    const auto* transform = world.TryGetTransform(player);
    const auto* movementState = world.TryGetCharacterMovementState(player);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(movementState, nullptr);
    const auto center = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(center.has_value());
    EXPECT_TRUE(bObservedFalling);
    EXPECT_TRUE(bObservedVerticalOnlyLocomotion);
    EXPECT_TRUE(movementState->grounded);
    EXPECT_FALSE(movementState->falling);
    EXPECT_NEAR(transform->position.y, 0.0f, 0.08f);
    EXPECT_NEAR(transform->position.y - center->y, binding->visualRootOffset.y, 0.001f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ModeLifecycleRecreatesOneFreshCharacter)
{
    StepFrame();
    auto* binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(player);
    ASSERT_NE(binding, nullptr);
    const physics::PhysicsCharacterHandle first = binding->character;
    ASSERT_TRUE(physicsWorld.IsCharacterValid(first));

    ASSERT_TRUE(appRuntime::DestroyControlledGameplayPhysicsCharacter(gameplayRuntime, physicsWorld));
    EXPECT_FALSE(physicsWorld.IsCharacterValid(first));
    EXPECT_FALSE(gameplayRuntime.GetWorld().HasPhysicsCharacter(player));

    gameplayRuntime.BeginFrame();
    gameplayRuntime.PrePhysicsUpdate(editorContext);
    gameplayRuntime.PostPhysicsUpdate(editorContext);
    StepFrame();
    binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(player);
    ASSERT_NE(binding, nullptr);
    EXPECT_TRUE(physicsWorld.IsCharacterValid(binding->character));
    EXPECT_NE(binding->character, first);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, StaleBindingIsRemovedWithoutOverwritingTransformAndRecovers)
{
    StepFrame();
    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(player);
    ASSERT_NE(binding, nullptr);
    ASSERT_TRUE(physicsWorld.DestroyCharacter(binding->character));
    const mathUtils::Vec3 preservedPosition{7.0f, 8.0f, 9.0f};
    world.TryGetTransform(player)->position = preservedPosition;

    EXPECT_TRUE(appRuntime::ApplyControlledGameplayPhysicsCharacterFeedback(
        gameplayRuntime, physicsWorld));
    EXPECT_EQ(world.TryGetTransform(player)->position, preservedPosition);
    EXPECT_FALSE(world.HasPhysicsCharacter(player));
    EXPECT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, levelAsset));
    EXPECT_TRUE(world.HasPhysicsCharacter(player));
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, NodeOwnerDestructionReleasesPhysicsCharacterFirst)
{
    StepFrame();
    const auto* binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(player);
    ASSERT_NE(binding, nullptr);
    const physics::PhysicsCharacterHandle character = binding->character;
    levelAsset.nodes[0].alive = false;

    gameplayRuntime.BeginFrame();
    ASSERT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, levelAsset));
    EXPECT_FALSE(physicsWorld.IsCharacterValid(character));
    // The normal pre-physics phase removes gameplay state only after the App
    // integration phase has released its physical representation.
    gameplayRuntime.PrePhysicsUpdate(gameContext);
    EXPECT_FALSE(gameplayRuntime.GetWorld().IsEntityValid(player));
    EXPECT_EQ(gameplayRuntime.GetControlledEntity(), rendern::kNullEntity);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, NoControlledEntityIsSuccessfulNoOp)
{
    ASSERT_TRUE(appRuntime::DestroyControlledGameplayPhysicsCharacter(gameplayRuntime, physicsWorld));
    gameplayRuntime.GetWorld().DestroyEntity(player);

    EXPECT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, levelAsset));
    EXPECT_TRUE(appRuntime::SubmitControlledGameplayPhysicsCharacterVelocity(
        gameplayRuntime, physicsWorld));
    EXPECT_TRUE(appRuntime::ApplyControlledGameplayPhysicsCharacterFeedback(
        gameplayRuntime, physicsWorld));
    EXPECT_TRUE(appRuntime::DestroyControlledGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld));
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ReleasedInputSubmitsDecelerationToZero)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    SetMoveIntent(1.0f);
    for (int frame = 0; frame < 60; ++frame)
    {
        StepFrame();
    }
    const float movingDesiredSpeed =
        gameplayRuntime.GetWorld().TryGetCharacterMotor(player)->desiredVelocity.z;
    ASSERT_GT(movingDesiredSpeed, 1.0f);

    SetMoveIntent(0.0f);
    for (int frame = 0; frame < 60; ++frame)
    {
        StepFrame();
    }
    const auto* motor = gameplayRuntime.GetWorld().TryGetCharacterMotor(player);
    ASSERT_NE(motor, nullptr);
    EXPECT_LT(std::abs(motor->desiredVelocity.z), 0.01f);
    EXPECT_LT(std::abs(motor->velocity.z), 0.1f);
}

TEST(GameplayPhysicsCharacterAppLifecycle, SetGameplayModeDestroysAndRecreatesControlledCharacter)
{
    InlineThreadOwnerRolesGuard threadGuard{};
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
    appLifecycle::AppState app{};
    app.physicsState.Initialize();
    app.contentState.levelAsset = std::make_unique<rendern::LevelAsset>();
    rendern::LevelNode playerNode{};
    playerNode.name = "Player";
    playerNode.alive = true;
    playerNode.visible = false;
    app.contentState.levelAsset->nodes.push_back(playerNode);
    app.runtimeState.levelInstance = std::make_unique<rendern::LevelInstance>();
    app.runtimeState.gameplayRuntime = std::make_unique<rendern::GameplayRuntime>();
    app.runtimeState.gameplayRuntime->Initialize(
        *app.contentState.levelAsset,
        *app.runtimeState.levelInstance,
        app.runtimeState.scene);
    rendern::GameplayUpdateContext editorContext{};
    editorContext.mode = rendern::GameplayRuntimeMode::Editor;
    editorContext.levelAsset = app.contentState.levelAsset.get();
    editorContext.levelInstance = app.runtimeState.levelInstance.get();
    editorContext.scene = &app.runtimeState.scene;
    const rendern::EntityHandle player = app.runtimeState.gameplayRuntime->SpawnNodeBoundEntity(
        editorContext, 0, true);
    ASSERT_NE(player, rendern::kNullEntity);

    std::string error;
    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app, rendern::GameplayRuntimeMode::Game, error)) << error;
    ASSERT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        *app.runtimeState.gameplayRuntime,
        *app.physicsState.joltPhysicsWorld,
        *app.contentState.levelAsset));
    const auto* firstBinding =
        app.runtimeState.gameplayRuntime->GetWorld().TryGetPhysicsCharacter(player);
    ASSERT_NE(firstBinding, nullptr);
    const physics::PhysicsCharacterHandle firstCharacter = firstBinding->character;
    ASSERT_TRUE(app.physicsState.joltPhysicsWorld->IsCharacterValid(firstCharacter));

    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app, rendern::GameplayRuntimeMode::Editor, error)) << error;
    EXPECT_FALSE(app.physicsState.joltPhysicsWorld->IsCharacterValid(firstCharacter));
    EXPECT_FALSE(app.runtimeState.gameplayRuntime->GetWorld().HasPhysicsCharacter(player));

    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app, rendern::GameplayRuntimeMode::Game, error)) << error;
    ASSERT_TRUE(appRuntime::EnsureControlledGameplayPhysicsCharacter(
        *app.runtimeState.gameplayRuntime,
        *app.physicsState.joltPhysicsWorld,
        *app.contentState.levelAsset));
    const auto* secondBinding =
        app.runtimeState.gameplayRuntime->GetWorld().TryGetPhysicsCharacter(player);
    ASSERT_NE(secondBinding, nullptr);
    EXPECT_TRUE(app.physicsState.joltPhysicsWorld->IsCharacterValid(secondBinding->character));
    EXPECT_NE(secondBinding->character, firstCharacter);

    ASSERT_TRUE(appLifecycle::SetGameplayMode(
        app, rendern::GameplayRuntimeMode::Editor, error)) << error;
    app.runtimeState.gameplayRuntime->Shutdown();
    app.runtimeState.gameplayRuntime.reset();
    app.physicsState.Shutdown();
}