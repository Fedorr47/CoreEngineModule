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
    constexpr std::string_view SmokeMap = "levels/ai_physics_smoke.level.json";
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;

    struct ArgvBuffer
    {
        std::vector<std::string> storage;
        std::vector<char*> argv;

        explicit ArgvBuffer(std::vector<std::string> values) : storage(std::move(values))
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

    [[nodiscard]] float PlanarSpeed(const mathUtils::Vec3& value)
    {
        return std::hypot(value.x, value.z);
    }

    class AIPhysicsSmokeTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            arguments = std::make_unique<ArgvBuffer>(std::vector<std::string>{
                "CoreEngineAIPhysicsSmokeTests",
                "--null",
                "--map=levels/ai_physics_smoke.level.json"
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

        [[nodiscard]] rendern::GameplayUpdateContext MakeContext()
        {
            return rendern::GameplayUpdateContext{
                .deltaSeconds = FixedDeltaSeconds,
                .mode = rendern::GameplayRuntimeMode::Game,
                .levelAsset = app.contentState.levelAsset.get(),
                .levelInstance = app.runtimeState.levelInstance.get(),
                .scene = &app.runtimeState.scene
            };
        }

        void StepFrame()
        {
            auto& gameplay = *app.runtimeState.gameplayRuntime;
            auto& physicsWorld = *app.physicsState.joltPhysicsWorld;
            auto& levelPhysicsRuntime = *app.physicsState.levelPhysicsRuntime;
            auto& levelAsset = *app.contentState.levelAsset;
            auto& levelInstance = *app.runtimeState.levelInstance;
            rendern::GameplayUpdateContext context = MakeContext();

            gameplay.BeginFrame();
            ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
                gameplay, physicsWorld, levelAsset));
            gameplay.PrePhysicsUpdate(context);
            ASSERT_TRUE(appRuntime::SubmitGameplayPhysicsCharacterVelocities(
                gameplay, physicsWorld));

            std::string error;
            ASSERT_TRUE(levelPhysicsRuntime.SynchronizeBeforePhysics(levelAsset, error)) << error;
            ASSERT_EQ(physicsWorld.Update(FixedDeltaSeconds), 1u);
            ASSERT_TRUE(levelPhysicsRuntime.SynchronizeAfterPhysics(
                levelAsset, levelInstance, app.runtimeState.scene, error)) << error;
            ASSERT_TRUE(appRuntime::ApplyGameplayPhysicsCharacterFeedback(
                gameplay, physicsWorld));
            gameplay.PostPhysicsUpdate(context);
        }

        [[nodiscard]] rendern::EntityHandle SpawnAgent(const std::string_view nodeName)
        {
            auto& gameplay = *app.runtimeState.gameplayRuntime;
            const int nodeIndex = FindNodeIndex(*app.contentState.levelAsset, nodeName);
            EXPECT_GE(nodeIndex, 0);
            if (nodeIndex < 0)
            {
                return rendern::kNullEntity;
            }
            const rendern::EntityHandle agent = gameplay.SpawnNodeBoundEntity(
                MakeContext(), nodeIndex, false);
            EXPECT_NE(agent, rendern::kNullEntity);
            if (agent != rendern::kNullEntity)
            {
                gameplay.GetWorld().AddAI(agent);
            }
            StepFrame();
            return agent;
        }

        void StartRoute(
            const rendern::EntityHandle agent,
            const mathUtils::Vec3& start,
            const mathUtils::Vec3& target)
        {
            rendern::GameplayRoute route{
                .points = {
                    rendern::GameplayRoutePoint{.worldPosition = start},
                    rendern::GameplayRoutePoint{.worldPosition = target}
                },
                .segmentAnnotations = {rendern::GameplayRouteSegmentAnnotation{}}
            };
            rendern::GameplayArrivalSteeringSettings steering{};
            steering.acceptanceRadius = 0.2f;
            steering.slowingRadius = 0.75f;
            steering.wantsRun = false;
            ASSERT_EQ(
                app.runtimeState.gameplayRuntime->StartAIFollowRoute(
                    agent, std::move(route), steering),
                rendern::AIActionExecutionStatus::Running);
        }

        void AssertValidPhysicsBinding(const rendern::EntityHandle agent)
        {
            const auto* binding = app.runtimeState.gameplayRuntime->GetWorld()
                .TryGetPhysicsCharacter(agent);
            ASSERT_NE(binding, nullptr);
            ASSERT_TRUE(binding->character.IsValid());
            ASSERT_TRUE(app.physicsState.joltPhysicsWorld->IsCharacterValid(binding->character));
        }

        appLifecycle::AppState app{};
        std::unique_ptr<ArgvBuffer> arguments;
    };
}

TEST_F(AIPhysicsSmokeTest, FreeMovementUsesPhysicsCharacter)
{
    ASSERT_EQ(app.launchState.currentLevelName, SmokeMap);
    const rendern::EntityHandle agent = SpawnAgent("NPC_Free_Start");
    ASSERT_NE(agent, rendern::kNullEntity);
    AssertValidPhysicsBinding(agent);
    StartRoute(agent, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f});

    const auto* initialTransform = app.runtimeState.gameplayRuntime->GetWorld().TryGetTransform(agent);
    ASSERT_NE(initialTransform, nullptr);
    const mathUtils::Vec3 initialPosition = initialTransform->position;
    for (int frame = 0; frame < 90; ++frame)
    {
        StepFrame();
    }

    const auto& world = app.runtimeState.gameplayRuntime->GetWorld();
    const auto* transform = world.TryGetTransform(agent);
    const auto* motor = world.TryGetCharacterMotor(agent);
    const auto* movement = world.TryGetCharacterMovementState(agent);
    const auto* binding = world.TryGetPhysicsCharacter(agent);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    ASSERT_NE(binding, nullptr);
    const auto physicalCenter = app.physicsState.joltPhysicsWorld->GetCharacterPosition(binding->character);
    ASSERT_TRUE(physicalCenter.has_value());

    EXPECT_GT(transform->position.z - initialPosition.z, 1.5f);
    EXPECT_GT(PlanarSpeed(motor->desiredVelocity), 1.0f);
    EXPECT_GT(PlanarSpeed(motor->velocity), 1.0f);
    EXPECT_NEAR(transform->position.x, physicalCenter->x + binding->visualRootOffset.x, 0.01f);
    EXPECT_NEAR(transform->position.z, physicalCenter->z + binding->visualRootOffset.z, 0.01f);
    EXPECT_FALSE(movement->physicallyBlocked);
}

TEST_F(AIPhysicsSmokeTest, WallBlockProducesPhysicalBlockedFeedback)
{
    const rendern::EntityHandle agent = SpawnAgent("NPC_Blocked_Start");
    ASSERT_NE(agent, rendern::kNullEntity);
    AssertValidPhysicsBinding(agent);
    StartRoute(agent, {-10.0f, 0.0f, 0.0f}, {-10.0f, 0.0f, 10.0f});

    for (int frame = 0; frame < 180; ++frame)
    {
        StepFrame();
    }
    const auto& world = app.runtimeState.gameplayRuntime->GetWorld();
    const auto* before = world.TryGetTransform(agent);
    ASSERT_NE(before, nullptr);
    const mathUtils::Vec3 positionBeforeObservation = before->position;
    for (int frame = 0; frame < 30; ++frame)
    {
        StepFrame();
    }

    const auto* transform = world.TryGetTransform(agent);
    const auto* motor = world.TryGetCharacterMotor(agent);
    const auto* movement = world.TryGetCharacterMovementState(agent);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    EXPECT_GT(PlanarSpeed(motor->desiredVelocity), 1.0f);
    EXPECT_LT(PlanarSpeed(motor->velocity), 0.08f);
    EXPECT_LT(PlanarSpeed(transform->position - positionBeforeObservation), 0.03f);
    EXPECT_GT(PlanarSpeed(motor->desiredVelocity), PlanarSpeed(motor->velocity) + 1.0f);
    EXPECT_TRUE(movement->physicallyBlocked);
    AssertValidPhysicsBinding(agent);
}

TEST_F(AIPhysicsSmokeTest, WallSlidePreservesTangentialMovement)
{
    const rendern::EntityHandle agent = SpawnAgent("NPC_Slide_Start");
    ASSERT_NE(agent, rendern::kNullEntity);
    AssertValidPhysicsBinding(agent);
    StartRoute(agent, {10.0f, 0.0f, 0.0f}, {16.0f, 0.0f, 12.0f});

    for (int frame = 0; frame < 120; ++frame)
    {
        StepFrame();
    }
    const auto& world = app.runtimeState.gameplayRuntime->GetWorld();
    const auto* before = world.TryGetTransform(agent);
    ASSERT_NE(before, nullptr);
    const mathUtils::Vec3 positionBeforeObservation = before->position;
    for (int frame = 0; frame < 30; ++frame)
    {
        StepFrame();
    }

    const auto* transform = world.TryGetTransform(agent);
    const auto* motor = world.TryGetCharacterMotor(agent);
    const auto* movement = world.TryGetCharacterMovementState(agent);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    EXPECT_GT(motor->desiredVelocity.x, 0.5f);
    EXPECT_GT(motor->desiredVelocity.z, 0.5f);
    EXPECT_LT(transform->position.x, 10.45f);
    EXPECT_LT(std::abs(transform->position.x - positionBeforeObservation.x), 0.03f);
    EXPECT_GT(transform->position.z - positionBeforeObservation.z, 0.4f);
    EXPECT_LT(std::abs(motor->velocity.x), 0.08f);
    EXPECT_GT(motor->velocity.z, 0.5f);
    EXPECT_FALSE(movement->physicallyBlocked);
    AssertValidPhysicsBinding(agent);
}
