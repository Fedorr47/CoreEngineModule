#include <gtest/gtest.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

import core;
import std;

#include "App/AppLifecycle.h"
#include "App/GameplayPhysicsCharacterIntegration.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/LevelPhysicsRuntime.h"

namespace
{
    constexpr std::string_view SmokeMap = "levels/character_physics_smoke.level.json";
    constexpr std::string_view PlayerNodeName = "CharacterPhysicsSmoke_Player";
    constexpr std::string_view FloorNodeName = "CharacterPhysicsSmoke_Floor";
    constexpr std::string_view WallNodeName = "CharacterPhysicsSmoke_Wall";
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;

    struct ArgvBuffer
    {
        std::vector<std::string> storage;
        std::vector<char*> argv;

        explicit ArgvBuffer(std::vector<std::string> values)
            : storage(std::move(values))
        {
            for (std::string& value : storage)
            {
                argv.push_back(value.data());
            }
        }
    };

    [[nodiscard]] int FindNodeIndex(
        const rendern::LevelAsset& level,
        const std::string_view name)
    {
        const auto iterator = std::ranges::find(level.nodes, name, &rendern::LevelNode::name);
        return iterator == level.nodes.end()
            ? -1
            : static_cast<int>(std::distance(level.nodes.begin(), iterator));
    }

    class CharacterPhysicsSmokeTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            arguments = std::make_unique<ArgvBuffer>(std::vector<std::string>{
                "CoreEngineCharacterPhysicsSmokeTests",
                "--null",
                "--map=levels/character_physics_smoke.level.json"
            });
            ASSERT_NO_THROW(appLifecycle::InitializeApp(
                app,
                static_cast<int>(arguments->argv.size()),
                arguments->argv.data()));

            std::string error;
            ASSERT_TRUE(appLifecycle::SetGameplayMode(
                app, rendern::GameplayRuntimeMode::Game, error)) << error;
        }

        void TearDown() override
        {
            if (!app.initialized)
            {
                return;
            }
            if (app.runtimeState.gameplayMode == rendern::GameplayRuntimeMode::Game)
            {
                std::string ignoredError;
                static_cast<void>(appLifecycle::SetGameplayMode(
                    app, rendern::GameplayRuntimeMode::Editor, ignoredError));
            }
            EXPECT_NO_THROW(appLifecycle::ShutdownApp(app));
        }

        void StepFrame()
        {
            auto& gameplay = *app.runtimeState.gameplayRuntime;
            auto& physicsWorld = *app.physicsState.joltPhysicsWorld;
            auto& levelPhysicsRuntime = *app.physicsState.levelPhysicsRuntime;
            auto& levelAsset = *app.contentState.levelAsset;
            auto& levelInstance = *app.runtimeState.levelInstance;

            rendern::GameplayUpdateContext context{};
            context.deltaSeconds = FixedDeltaSeconds;
            context.mode = rendern::GameplayRuntimeMode::Game;
            context.levelAsset = &levelAsset;
            context.levelInstance = &levelInstance;
            context.scene = &app.runtimeState.scene;

            gameplay.BeginFrame();

            ASSERT_TRUE(
                appRuntime::EnsureGameplayPhysicsCharacters(
                    gameplay,
                    physicsWorld,
                    levelAsset));

            gameplay.PrePhysicsUpdate(context);

            ASSERT_TRUE(
                appRuntime::SubmitGameplayPhysicsCharacterVelocities(
                    gameplay,
                    physicsWorld));

            std::string errorMessage;

            ASSERT_TRUE(
                levelPhysicsRuntime.SynchronizeBeforePhysics(
                    levelAsset,
                    errorMessage))
                << errorMessage;

            ASSERT_EQ(
                physicsWorld.Update(FixedDeltaSeconds),
                1u);

            ASSERT_TRUE(
                levelPhysicsRuntime.SynchronizeAfterPhysics(
                    levelAsset,
                    levelInstance,
                    app.runtimeState.scene,
                    errorMessage))
                << errorMessage;

            ASSERT_TRUE(
                appRuntime::ApplyGameplayPhysicsCharacterFeedback(
                    gameplay,
                    physicsWorld));

            gameplay.PostPhysicsUpdate(context);
        }

        appLifecycle::AppState app{};
        std::unique_ptr<ArgvBuffer> arguments;
    };
}

TEST_F(CharacterPhysicsSmokeTest, SerializedLevelDrivesControlledCharacterThroughFloorAndWall)
{
    ASSERT_EQ(app.launchState.currentLevelName, SmokeMap);
    const rendern::LevelAsset& level = *app.contentState.levelAsset;
    const int playerNodeIndex = FindNodeIndex(level, PlayerNodeName);
    const int floorNodeIndex = FindNodeIndex(level, FloorNodeName);
    const int wallNodeIndex = FindNodeIndex(level, WallNodeName);
    ASSERT_GE(playerNodeIndex, 0);
    ASSERT_GE(floorNodeIndex, 0);
    ASSERT_GE(wallNodeIndex, 0);
    EXPECT_FALSE(level.nodes[static_cast<std::size_t>(playerNodeIndex)].skinnedMesh.empty());
    ASSERT_TRUE(level.nodes[static_cast<std::size_t>(floorNodeIndex)].physicsBody.has_value());
    ASSERT_TRUE(level.nodes[static_cast<std::size_t>(wallNodeIndex)].physicsBody.has_value());
    EXPECT_TRUE(app.physicsState.levelPhysicsRuntime->IsActive());

    StepFrame();
    rendern::GameplayRuntime& gameplay = *app.runtimeState.gameplayRuntime;
    const rendern::EntityHandle player = gameplay.GetControlledEntity();
    ASSERT_NE(player, rendern::kNullEntity);
    const rendern::GameplayWorld& initialWorld = gameplay.GetWorld();
    const auto* initialBinding = initialWorld.TryGetPhysicsCharacter(player);
    const auto* initialTransform = initialWorld.TryGetTransform(player);
    ASSERT_NE(initialBinding, nullptr);
    ASSERT_NE(initialTransform, nullptr);
    ASSERT_TRUE(initialBinding->character.IsValid());
    const auto initialCenter = app.physicsState.joltPhysicsWorld->GetCharacterPosition(
        initialBinding->character);
    ASSERT_TRUE(initialCenter.has_value());
    EXPECT_NEAR(
        initialTransform->position.y - initialCenter->y,
        initialBinding->visualRootOffset.y,
        0.01f);

    for (int frame = 0; frame < 120; ++frame)
    {
        StepFrame();
    }
    const rendern::GameplayWorld& settledWorld = gameplay.GetWorld();
    const auto* settledTransform = settledWorld.TryGetTransform(player);
    const auto* movementState = settledWorld.TryGetCharacterMovementState(player);
    ASSERT_NE(settledTransform, nullptr);
    ASSERT_NE(movementState, nullptr);
    EXPECT_TRUE(movementState->grounded);
    EXPECT_FALSE(movementState->falling);
    EXPECT_NEAR(settledTransform->position.y, 0.0f, 0.08f);
    const float spawnPositionZ = settledTransform->position.z;

    gameplay.BindIntentSource(
        player,
        [](rendern::EntityHandle,
            const rendern::GameplayUpdateContext&,
            rendern::GameplayWorld&,
            rendern::GameplayInputIntentComponent& intent,
            rendern::GameplayActionComponent*)
        {
            intent.moveY = 1.0f;
        });
    for (int frame = 0; frame < 240; ++frame)
    {
        StepFrame();
    }
    const auto* blockedTransform = gameplay.GetWorld().TryGetTransform(player);
    ASSERT_NE(blockedTransform, nullptr);
    const float blockedPositionZ = blockedTransform->position.z;
    EXPECT_GT(blockedPositionZ, spawnPositionZ + 3.0f);
    EXPECT_LT(blockedPositionZ, 5.75f);

    for (int frame = 0; frame < 120; ++frame)
    {
        StepFrame();
    }
    const rendern::GameplayWorld& finalWorld = gameplay.GetWorld();
    const auto* finalTransform = finalWorld.TryGetTransform(player);
    const auto* motor = finalWorld.TryGetCharacterMotor(player);
    const auto* followCamera = finalWorld.TryGetFollowCamera(player);
    ASSERT_NE(finalTransform, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(followCamera, nullptr);
    EXPECT_NEAR(finalTransform->position.z, blockedPositionZ, 0.03f);
    EXPECT_GT(motor->desiredVelocity.z, 1.5f);
    EXPECT_NEAR(
        app.runtimeState.scene.camera.target.z,
        finalTransform->position.z + followCamera->focusOffset.z,
        0.01f);
}