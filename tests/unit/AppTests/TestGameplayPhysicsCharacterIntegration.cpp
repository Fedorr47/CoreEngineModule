#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>

#include "App/Development/DevelopmentScenario.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

import core;
import std;

#include "App/GameplayPhysicsObstacleQuery.h"
#include "App/GameplayPhysicsCharacterIntegration.h"
#include "App/Development/AppDevelopmentScenarioRuntime.h"
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
            rendern::LevelNode npcNode{};
            npcNode.name = "NPC";
            npcNode.alive = true;
            npcNode.visible = false;
            levelAsset.nodes.push_back(npcNode);
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
            EXPECT_TRUE(appRuntime::DestroyGameplayPhysicsCharacters(
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
        
        void SetMoveAndJumpIntent(const float moveY, const bool requestJump)
        {
            gameplayRuntime.BindIntentSource(
                player,
                [moveY, requestJump](rendern::EntityHandle,
                    const rendern::GameplayUpdateContext&,
                    rendern::GameplayWorld&,
                    rendern::GameplayInputIntentComponent& intent,
                    rendern::GameplayActionComponent*)
                {
                    intent.moveY = moveY;
                    if (requestJump)
                    {
                        rendern::AddGameplayActionIntent(
                            intent.actionIntents, rendern::kGameplayActionJump);
                    }
                });
        }

        std::uint32_t StepFrameAndReturnPhysicsSteps(const float frameSeconds)
        {
            rendern::GameplayUpdateContext frameContext = gameContext;
            frameContext.deltaSeconds = frameSeconds;
            gameplayRuntime.BeginFrame();
            EXPECT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
                gameplayRuntime, physicsWorld, levelAsset));
            gameplayRuntime.PrePhysicsUpdate(frameContext);
            EXPECT_TRUE(appRuntime::SubmitGameplayPhysicsCharacterVelocities(
                gameplayRuntime, physicsWorld));
            const std::uint32_t physicsSteps = physicsWorld.Update(frameSeconds);
            EXPECT_TRUE(appRuntime::ApplyGameplayPhysicsCharacterFeedback(
                gameplayRuntime, physicsWorld));
            gameplayRuntime.PostPhysicsUpdate(frameContext);
            return physicsSteps;
        }

        rendern::EntityHandle SpawnNPC()
        {
            const rendern::EntityHandle npc = gameplayRuntime.SpawnNodeBoundEntity(
                editorContext, 1, false);
            if (npc != rendern::kNullEntity)
            {
                gameplayRuntime.GetWorld().SetCharacterPhysicalSettings(npc, {
                    .radius = 0.22f,
                    .cylinderHeight = 0.86f,
                    .maximumSlopeAngleDegrees = 38.0f,
                    .maximumStepHeight = 0.18f,
                    .mass = 54.0f
                });
                gameplayRuntime.GetWorld().AddAI(npc);
            }
            return npc;
        }

        void SetEntityMoveIntent(const rendern::EntityHandle entity, const float moveX, const float moveY)
        {
            gameplayRuntime.BindIntentSource(
                entity,
                [moveX, moveY](rendern::EntityHandle,
                    const rendern::GameplayUpdateContext&,
                    rendern::GameplayWorld&,
                    rendern::GameplayInputIntentComponent& intent,
                    rendern::GameplayActionComponent*)
                {
                    intent.moveX = moveX;
                    intent.moveY = moveY;
                });
        }

        void StepFrame(const float frameSeconds = StepSeconds)
        {
            static_cast<void>(StepFrameAndReturnPhysicsSteps(frameSeconds));
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

TEST_F(GameplayPhysicsCharacterIntegrationTest, ScenarioTeleportOperationUsesProductionCharacterPath)
{
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
        gameplayRuntime, physicsWorld, levelAsset));
    auto* transform = gameplayRuntime.GetWorld().TryGetTransform(player);
    ASSERT_NE(transform, nullptr);
    transform->position = {3.0f, 2.0f, -4.0f};

    appDevelopment::DevelopmentScenarioAsset asset{
        .id = "test.teleport",
        .title = "Teleport operation",
        .roles = {{"agent", "Player"}},
        .start = {appDevelopment::TeleportPhysicsCharacterOperation{"agent"}}};
    appDevelopment::ScenarioContext context{
        gameplayRuntime, levelAsset, levelInstance, scene,
        rendern::GameplayRuntimeMode::Game, &physicsWorld, nullptr};
    appDevelopment::DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(asset, context));
    EXPECT_TRUE(runner.Start(context));
    runner.Unload(context);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ObstacleQueryAdapterUsesStaticWorldLayerAndReportsMiss)
{
    physics::PhysicsBodyDescriptor obstacle{
        .shape = physics::BoxShapeDescriptor{.halfExtents = {0.5f, 0.5f, 0.5f}},
        .transform = {.position = {2.0f, 0.5f, 0.0f}},
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(obstacle).IsValid());
    appRuntime::GameplayPhysicsObstacleQuery query{physicsWorld};
    rendern::GameplayObstacleProbeHit hit{};
    EXPECT_TRUE(query.Probe({{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, 5.0f}, hit));
    EXPECT_NEAR(hit.distance, 1.5f, 0.001f);
    EXPECT_TRUE(query.Probe(
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, 5.0f, 0.35f}, hit));
    EXPECT_NEAR(mathUtils::Length(hit.normal), 1.0f, 0.001f);
    EXPECT_LT(mathUtils::Dot(hit.normal, {1.0f, 0.0f, 0.0f}), 0.0f);
    EXPECT_LT(hit.normal.x, -0.9f);
    EXPECT_FALSE(query.Probe({{0.0f, 0.5f, 0.0f}, {-1.0f, 0.0f, 0.0f}, 5.0f}, hit));

    obstacle.transform.position = {0.0f, 0.5f, 2.0f};
    obstacle.motionType = physics::PhysicsMotionType::Dynamic;
    ASSERT_TRUE(physicsWorld.CreateBody(obstacle).IsValid());
    EXPECT_FALSE(query.Probe({{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, 5.0f}, hit));
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ObstacleQueryUsesCharacterRadiusAtObstacleEdge)
{
    physics::PhysicsBodyDescriptor obstacle{
        .shape = physics::BoxShapeDescriptor{.halfExtents = {0.5f, 0.5f, 0.5f}},
        .transform = {.position = {2.0f, 0.5f, 0.7f}},
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(obstacle).IsValid());
    appRuntime::GameplayPhysicsObstacleQuery query{physicsWorld};
    rendern::GameplayObstacleProbeHit hit{};

    EXPECT_FALSE(query.Probe(
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, 5.0f, 0.0f}, hit));
    EXPECT_TRUE(query.Probe(
        {{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, 5.0f, 0.35f}, hit));
    EXPECT_LT(hit.distance, 1.5f);
    EXPECT_TRUE(mathUtils::IsFinite(hit.normal));
    EXPECT_NEAR(mathUtils::Length(hit.normal), 1.0f, 0.001f);
    EXPECT_LT(mathUtils::Dot(hit.normal, {1.0f, 0.0f, 0.0f}), 0.0f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, PhysicalCharacterEscapesStaticObstacleEdge)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    physics::PhysicsBodyDescriptor obstacle{
        .shape = physics::BoxShapeDescriptor{.halfExtents = {0.5f, 0.5f, 0.5f}},
        .transform = {.position = {0.5f, 0.5f, 0.0f}},
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(obstacle).IsValid());

    constexpr float characterRadius = 0.22f;
    const physics::PhysicsCharacterHandle character = physicsWorld.CreateCharacter({
        .collider = {.radius = characterRadius, .cylinderHeight = 0.86f},
        .position = {-0.25f, 0.65f, 0.0f},
        .maximumSlopeAngleDegrees = 38.0f,
        .maximumStepHeight = 0.18f,
        .mass = 54.0f,
        .maximumSpeed = 2.0f
    });
    ASSERT_TRUE(character.IsValid());

    appRuntime::GameplayPhysicsObstacleQuery query{physicsWorld};
    rendern::GameplayObstacleAvoidanceState avoidanceState{};
    const rendern::GameplayObstacleAvoidanceSettings settings{
        .forwardProbeDistance = 1.0f,
        .sideProbeDistance = 1.0f,
        .characterRadius = characterRadius
    };
    const rendern::GameplayMovementIntent baseMovement{
        .moveWorld = {1.0f, 0.0f, 0.0f},
        .moveMagnitude = 1.0f
    };
    bool avoidanceActivated = false;
    int consecutiveReleasedSteps = 0;
    float minimumZ = 0.0f;

    for (int step = 0; step < 120; ++step)
    {
        const auto position = physicsWorld.GetCharacterPosition(character);
        ASSERT_TRUE(position.has_value());
        const rendern::GameplayMovementIntent movement =
            rendern::ApplyGameplayObstacleAvoidance(
                baseMovement, *position, query, settings, avoidanceState);
        avoidanceActivated = avoidanceActivated ||
            avoidanceState.committedSide != rendern::GameplayObstacleAvoidanceSide::None;
        consecutiveReleasedSteps = avoidanceActivated &&
            avoidanceState.committedSide == rendern::GameplayObstacleAvoidanceSide::None
            ? consecutiveReleasedSteps + 1
            : 0;
        minimumZ = std::min(minimumZ, position->z);
        ASSERT_TRUE(physicsWorld.SetCharacterDesiredVelocity(
            character, movement.moveWorld * 1.5f));
        ASSERT_EQ(physicsWorld.Update(StepSeconds), 1u);
    }

    const auto finalPosition = physicsWorld.GetCharacterPosition(character);
    ASSERT_TRUE(finalPosition.has_value());
    EXPECT_TRUE(avoidanceActivated);
    EXPECT_LT(minimumZ, -0.5f - characterRadius);
    EXPECT_GE(consecutiveReleasedSteps, 10);
    EXPECT_GT(finalPosition->x, 0.5f);
    EXPECT_LT(finalPosition->z, -0.5f - characterRadius);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, DataDrivenStepResetRestoresGameplayAndPhysicalCharacter)
{
    rendern::LevelNode target{};
    target.name = "RouteTarget"; target.alive = true;
    target.transform.position = {0.0f, 0.0f, 6.0f};
    levelAsset.nodes.push_back(target);
    StepFrame();
    const auto scenario = appDevelopment::ParseDevelopmentScenarioAsset(R"({
      "id":"step.integration","title":"Step integration","roles":{"agent":"NPC","target":"RouteTarget"},
      "setup":[{"op":"ensureNodeBoundEntity","entity":"agent"},{"op":"captureTransform","entity":"agent","slot":"baseline"},{"op":"ensureAI","entity":"agent"}],
      "start":[{"op":"cancelAI","entity":"agent"},{"op":"restoreTransform","entity":"agent","slot":"baseline"},{"op":"resetEntitySimulationState","entity":"agent"},{"op":"teleportPhysicsCharacter","entity":"agent"},{"op":"startFollowRoute","entity":"agent","points":["agent","target"],"segmentTraversals":[],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false}],
      "stop":[{"op":"cancelAI","entity":"agent"}],
      "reset":[{"op":"cancelAI","entity":"agent"},{"op":"restoreTransform","entity":"agent","slot":"baseline"},{"op":"resetEntitySimulationState","entity":"agent"},{"op":"teleportPhysicsCharacter","entity":"agent"}]})");
    appDevelopment::ScenarioContext context{gameplayRuntime, levelAsset, levelInstance, scene,
        rendern::GameplayRuntimeMode::Game, &physicsWorld, nullptr};
    appDevelopment::DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    rendern::EntityHandle npc = rendern::kNullEntity;
    for (const auto entity : gameplayRuntime.GetNodeBoundEntities())
        if (const auto* link = gameplayRuntime.GetWorld().TryGetNodeLink(entity);
            link && link->nodeIndex == 1) npc = entity;
    ASSERT_NE(npc, rendern::kNullEntity);
    gameplayRuntime.GetWorld().SetCharacterPhysicalSettings(npc, {
        .radius=0.22f, .cylinderHeight=0.86f, .maximumSlopeAngleDegrees=38.0f,
        .maximumStepHeight=0.18f, .mass=54.0f});
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(gameplayRuntime, physicsWorld, levelAsset));
    const auto* binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(npc);
    ASSERT_NE(binding, nullptr);
    ASSERT_TRUE(physicsWorld.IsCharacterValid(binding->character));
    ASSERT_TRUE(runner.Start(context));
    EXPECT_EQ(gameplayRuntime.GetAIActionStatus(npc), rendern::AIActionExecutionStatus::Running);
    gameplayRuntime.GetWorld().TryGetTransform(npc)->position = {7.0f, 4.0f, 9.0f};
    ASSERT_TRUE(physicsWorld.TeleportCharacter(binding->character, {7.0f, 4.0f, 9.0f}));
    runner.Reset(context);
    const mathUtils::Vec3 canonical = levelAsset.nodes[1].transform.position;
    EXPECT_EQ(gameplayRuntime.GetWorld().TryGetTransform(npc)->position, canonical);
    const auto physical = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(physical.has_value());
    EXPECT_EQ(*physical, canonical - binding->visualRootOffset);
    EXPECT_EQ(gameplayRuntime.GetAIActionStatus(npc), rendern::AIActionExecutionStatus::NotStarted);
    runner.Unload(context);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, SingleEntityPhysicsTeardownPreservesGameplayEntity)
{
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
        gameplayRuntime, physicsWorld, levelAsset));
    const auto* binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(npc);
    ASSERT_NE(binding, nullptr);
    const physics::PhysicsCharacterHandle character = binding->character;
    ASSERT_TRUE(physicsWorld.IsCharacterValid(character));

    EXPECT_TRUE(appRuntime::DestroyGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, npc));
    EXPECT_FALSE(physicsWorld.IsCharacterValid(character));
    EXPECT_FALSE(gameplayRuntime.GetWorld().HasPhysicsCharacter(npc));
    EXPECT_TRUE(gameplayRuntime.GetWorld().IsEntityValid(npc));
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, SingleEntityPhysicsTeardownWithoutBindingIsNoOp)
{
    ASSERT_TRUE(gameplayRuntime.GetWorld().IsEntityValid(player));
    ASSERT_FALSE(gameplayRuntime.GetWorld().HasPhysicsCharacter(player));
    EXPECT_TRUE(appRuntime::DestroyGameplayPhysicsCharacter(
        gameplayRuntime, physicsWorld, player));
    EXPECT_TRUE(gameplayRuntime.GetWorld().IsEntityValid(player));
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, ScenarioUnloadDestroysPhysicsBeforeSpawnedGameplayEntity)
{
    appDevelopment::DevelopmentScenarioAsset asset{
        .id = "test.spawned.physics",
        .title = "Spawned physics teardown",
        .roles = {{"agent", "NPC"}},
        .setup = {
            appDevelopment::EnsureNodeBoundEntityOperation{"agent"},
            appDevelopment::EnsureAIOperation{"agent"}}};
    appDevelopment::ScenarioContext context{
        gameplayRuntime, levelAsset, levelInstance, scene,
        rendern::GameplayRuntimeMode::Game, &physicsWorld, nullptr};
    appDevelopment::DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(asset, context));
    rendern::EntityHandle npc = rendern::kNullEntity;
    for (const auto entity : gameplayRuntime.GetNodeBoundEntities())
        if (const auto* link = gameplayRuntime.GetWorld().TryGetNodeLink(entity);
            link && link->nodeIndex == 1) npc = entity;
    ASSERT_NE(npc, rendern::kNullEntity);
    gameplayRuntime.GetWorld().SetCharacterPhysicalSettings(npc, {
        .radius = 0.22f, .cylinderHeight = 0.86f,
        .maximumSlopeAngleDegrees = 38.0f, .maximumStepHeight = 0.18f, .mass = 54.0f});
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
        gameplayRuntime, physicsWorld, levelAsset));
    const auto* binding = gameplayRuntime.GetWorld().TryGetPhysicsCharacter(npc);
    ASSERT_NE(binding, nullptr);
    const physics::PhysicsCharacterHandle character = binding->character;

    runner.Unload(context);
    EXPECT_FALSE(physicsWorld.IsCharacterValid(character));
    EXPECT_FALSE(gameplayRuntime.GetWorld().IsEntityValid(npc));
    EXPECT_EQ(std::ranges::count(gameplayRuntime.GetNodeBoundEntities(), npc), 0);
}

TEST(GameplayCharacterPhysicalSettings, DerivesSynchronizedPhysicsAndNavigationDimensions)
{
    constexpr rendern::GameplayCharacterPhysicalSettingsComponent settings{
        .radius = 0.47f,
        .cylinderHeight = 1.13f,
        .maximumSlopeAngleDegrees = 37.0f,
        .maximumStepHeight = 0.29f,
        .mass = 63.0f
    };
    constexpr physics::PhysicsCharacterDescriptor physicsDescriptor =
        settings.BuildPhysicsCharacterDescriptor({ 1.0f, 2.0f, 3.0f }, 5.5f);
    constexpr navigation::AgentSettings navigationSettings =
        app::navigationRuntime::BuildAgentSettings(settings);

    EXPECT_FLOAT_EQ(physicsDescriptor.collider.radius, navigationSettings.radius);
    EXPECT_FLOAT_EQ(physicsDescriptor.collider.GetTotalHeight(), navigationSettings.height);
    EXPECT_FLOAT_EQ(navigationSettings.height, 1.13f + 2.0f * 0.47f);
    EXPECT_FLOAT_EQ(physicsDescriptor.maximumStepHeight, navigationSettings.maximumStepHeight);
    EXPECT_FLOAT_EQ(
        physicsDescriptor.maximumSlopeAngleDegrees, navigationSettings.maximumSlopeAngleDegrees);
    EXPECT_TRUE(physicsDescriptor.IsValid());
    EXPECT_TRUE(navigationSettings.IsValid());
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

TEST_F(GameplayPhysicsCharacterIntegrationTest, RunningJumpPreservesPlanarMovementThroughProductionPath)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    SetMoveIntent(1.0f);
    for (int frame = 0; frame < 60; ++frame)
    {
        StepFrame();
    }

    auto& world = gameplayRuntime.GetWorld();
    const float planarVelocityBefore = world.TryGetCharacterMotor(player)->velocity.z;
    ASSERT_GT(planarVelocityBefore, 1.0f);

    SetMoveAndJumpIntent(1.0f, true);
    StepFrame();
    SetMoveIntent(1.0f);

    const auto* motor = world.TryGetCharacterMotor(player);
    const auto* movement = world.TryGetCharacterMovementState(player);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::Airborne);
    EXPECT_EQ(movement->jumpRequestResult, rendern::GameplayJumpRequestResult::Accepted);
    EXPECT_TRUE(movement->jumping);
    EXPECT_GT(motor->velocity.y, 4.0f);
    EXPECT_NEAR(motor->velocity.z, planarVelocityBefore, 0.05f);
    EXPECT_GT(std::hypot(motor->velocity.x, motor->velocity.z), 1.0f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, AIJumpTraversalUsesPhysicalJumpAndResumesRoute)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);

    // Enter Game mode, create the physical character, and register built-in traversal executors.
    StepFrameAndReturnPhysicsSteps(StepSeconds);
    rendern::GameplayWorld& world = gameplayRuntime.GetWorld();
    const rendern::EntityHandle landingMarker = world.CreateEntity();
    constexpr rendern::GameplayTraversalLinkHandle jumpLink{447u};
    ASSERT_TRUE(gameplayRuntime.RegisterGameplayTraversalLink({
        .handle = jumpLink,
        .traversalTypeId = rendern::kJumpTraversalTypeId,
        .targetEntity = landingMarker,
        .jump = {
            .takeoffPosition = {0.0f, 0.0f, 0.0f},
            .landingPosition = {0.0f, 0.0f, 0.1f},
            .verticalSpeed = 5.5f,
            .takeoffTolerance = 0.25f,
            .landingHorizontalTolerance = 0.6f,
            .landingVerticalTolerance = 0.15f}}));
    rendern::GameplayRoute route{
        .points = {
            {.worldPosition = {0.0f, 0.0f, 0.0f}},
            {.worldPosition = {0.0f, 0.0f, 0.1f}},
            {.worldPosition = {0.0f, 0.0f, 2.0f}}},
        .segmentAnnotations = {{.traversalLink = jumpLink}, {}}};
    ASSERT_EQ(gameplayRuntime.StartAIFollowRoute(npc, std::move(route)),
        rendern::AIActionExecutionStatus::Running);

    bool accepted = false;
    bool physicallyAirborne = false;
    bool landed = false;
    bool routeResumed = false;
    float maximumPlanarSpeed = 0.0f;
    for (int frame = 0; frame < 300 && !routeResumed; ++frame)
    {
        StepFrameAndReturnPhysicsSteps(StepSeconds);
        const auto* movement = world.TryGetCharacterMovementState(npc);
        const auto* motor = world.TryGetCharacterMotor(npc);
        const auto* command = world.TryGetCharacterCommand(npc);
        ASSERT_NE(movement, nullptr);
        ASSERT_NE(motor, nullptr);
        ASSERT_NE(command, nullptr);
        accepted = accepted ||
            movement->jumpRequestResult == rendern::GameplayJumpRequestResult::Accepted;
        physicallyAirborne = physicallyAirborne || movement->jumpAirbornePhysicallyObserved;
        maximumPlanarSpeed = std::max(maximumPlanarSpeed,
            std::hypot(motor->velocity.x, motor->velocity.z));
        landed = landed || (physicallyAirborne && movement->grounded &&
            movement->jumpPhase == rendern::GameplayJumpPhase::None);
        routeResumed = landed && command->moveMagnitude > 0.0f &&
            gameplayRuntime.GetAIActionStatus(npc) == rendern::AIActionExecutionStatus::Running;
    }

    EXPECT_TRUE(accepted);
    EXPECT_TRUE(physicallyAirborne);
    EXPECT_GT(maximumPlanarSpeed, 0.01f);
    EXPECT_TRUE(landed);
    EXPECT_TRUE(routeResumed);
    EXPECT_NEAR(world.TryGetTransform(npc)->position.y, 0.0f, 0.15f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, AirborneStateSurvivesRenderFrameWithoutFixedStep)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    for (int frame = 0; frame < 30; ++frame)
    {
        StepFrame();
    }

    SetMoveAndJumpIntent(0.0f, true);
    ASSERT_EQ(StepFrameAndReturnPhysicsSteps(1.0f / 120.0f), 0u);
    SetMoveIntent(0.0f);

    auto& world = gameplayRuntime.GetWorld();
    const auto* movement = world.TryGetCharacterMovementState(player);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::Airborne);
    EXPECT_TRUE(movement->jumping);
    EXPECT_FALSE(movement->jumpAirbornePhysicallyObserved);

    ASSERT_EQ(StepFrameAndReturnPhysicsSteps(1.0f / 120.0f), 1u);
    movement = world.TryGetCharacterMovementState(player);
    ASSERT_NE(movement, nullptr);
    EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::Airborne);
    EXPECT_GT(world.TryGetCharacterMotor(player)->velocity.y, 4.0f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, RejectedJumpAttemptIsConsumedWithoutRetry)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    for (int frame = 0; frame < 30; ++frame)
    {
        StepFrame();
    }

    auto& world = gameplayRuntime.GetWorld();
    world.TryGetCharacterMotor(player)->jumpVerticalSpeed = 0.0f;
    SetMoveAndJumpIntent(0.0f, true);
    StepFrame();
    SetMoveIntent(0.0f);

    const auto* movement = world.TryGetCharacterMovementState(player);
    const auto* action = world.TryGetAction(player);
    ASSERT_NE(movement, nullptr);
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::None);
    EXPECT_EQ(movement->jumpRequestResult, rendern::GameplayJumpRequestResult::Rejected);
    EXPECT_FALSE(movement->jumping);
    EXPECT_EQ(movement->jumpLockedVelocity, mathUtils::Vec3{});
    EXPECT_EQ(rendern::GetGameplayRequestedActionId(*action),
        rendern::GameplayActionId{});
    EXPECT_TRUE(movement->jumpRequestConsumed);

    for (int frame = 0; frame < 10; ++frame)
    {
        StepFrame();
        movement = world.TryGetCharacterMovementState(player);
        ASSERT_NE(movement, nullptr);
        EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::None);
        EXPECT_LE(world.TryGetCharacterMotor(player)->velocity.y, 0.1f);
    }
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, PhysicalLandingClearsJumpState)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    for (int frame = 0; frame < 30; ++frame)
    {
        StepFrame();
    }

    SetMoveAndJumpIntent(0.0f, true);
    StepFrame();
    SetMoveIntent(0.0f);

    auto& world = gameplayRuntime.GetWorld();
    bool observedPhysicalAirborne = false;
    for (int frame = 0; frame < 240; ++frame)
    {
        StepFrame();
        const auto* movement = world.TryGetCharacterMovementState(player);
        ASSERT_NE(movement, nullptr);
        observedPhysicalAirborne = observedPhysicalAirborne ||
            movement->jumpAirbornePhysicallyObserved;
        if (observedPhysicalAirborne &&
            movement->jumpPhase == rendern::GameplayJumpPhase::None)
        {
            break;
        }
    }

    const auto* movement = world.TryGetCharacterMovementState(player);
    ASSERT_NE(movement, nullptr);
    EXPECT_TRUE(observedPhysicalAirborne);
    EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::None);
    EXPECT_FALSE(movement->jumping);
    EXPECT_TRUE(movement->grounded);

    const auto* action = world.TryGetAction(player);
    ASSERT_NE(action, nullptr);
    ASSERT_EQ(rendern::GetGameplayRequestedActionId(*action),
        rendern::kGameplayActionJump);
    ASSERT_FALSE(action->pendingDispatched);
    ASSERT_TRUE(movement->jumpRequestConsumed);
    for (int frame = 0; frame < 10; ++frame)
    {
        StepFrame();
        movement = world.TryGetCharacterMovementState(player);
        ASSERT_NE(movement, nullptr);
        EXPECT_EQ(movement->jumpPhase, rendern::GameplayJumpPhase::None);
        EXPECT_FALSE(movement->jumping);
        EXPECT_TRUE(movement->jumpRequestConsumed);
        EXPECT_LE(world.TryGetCharacterMotor(player)->velocity.y, 0.1f);
    }
}


TEST_F(GameplayPhysicsCharacterIntegrationTest, CreationConvertsVisualRootToCapsuleCenterExplicitly)
{
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
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

    ASSERT_TRUE(appRuntime::DestroyGameplayPhysicsCharacters(gameplayRuntime, physicsWorld));
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

    EXPECT_TRUE(appRuntime::ApplyGameplayPhysicsCharacterFeedback(
        gameplayRuntime, physicsWorld));
    EXPECT_EQ(world.TryGetTransform(player)->position, preservedPosition);
    EXPECT_FALSE(world.HasPhysicsCharacter(player));
    EXPECT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
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
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
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
    ASSERT_TRUE(appRuntime::DestroyGameplayPhysicsCharacters(gameplayRuntime, physicsWorld));
    gameplayRuntime.GetWorld().DestroyEntity(player);

    EXPECT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
        gameplayRuntime, physicsWorld, levelAsset));
    EXPECT_TRUE(appRuntime::SubmitGameplayPhysicsCharacterVelocities(
        gameplayRuntime, physicsWorld));
    EXPECT_TRUE(appRuntime::ApplyGameplayPhysicsCharacterFeedback(
        gameplayRuntime, physicsWorld));
    EXPECT_TRUE(appRuntime::DestroyGameplayPhysicsCharacters(
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

TEST_F(GameplayPhysicsCharacterIntegrationTest, NPCFreeMovementUsesSharedPhysicsFeedback)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    SetEntityMoveIntent(npc, 0.0f, 1.0f);

    for (int frame = 0; frame < 60; ++frame)
    {
        StepFrame();
    }

    const auto& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(npc);
    const auto* transform = world.TryGetTransform(npc);
    const auto* motor = world.TryGetCharacterMotor(npc);
    const auto* movement = world.TryGetCharacterMovementState(npc);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    const auto physicalPosition = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(physicalPosition.has_value());
    EXPECT_GT(transform->position.z, 0.5f);
    EXPECT_NEAR(binding->visualRootOffset.y, -0.65f, 0.001f);
    EXPECT_NE(binding->visualRootOffset, mathUtils::Vec3(0.0f, -0.9f, 0.0f));
    EXPECT_NEAR(transform->position.z, physicalPosition->z, 0.001f);
    EXPECT_GT(motor->velocity.z, 1.0f);
    EXPECT_FALSE(movement->physicallyBlocked);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, NPCWallReportsObservedStopAndBecomesPhysicallyBlocked)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const physics::PhysicsBodyDescriptor wall{
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 2.0f } },
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(wall).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    SetEntityMoveIntent(npc, 0.0f, 1.0f);

    for (int frame = 0; frame < 180; ++frame)
    {
        StepFrame();
    }

    const auto& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(npc);
    const auto* motor = world.TryGetCharacterMotor(npc);
    const auto* movement = world.TryGetCharacterMovementState(npc);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    const auto positionBefore = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(positionBefore.has_value());
    StepFrame();
    const auto positionAfter = physicsWorld.GetCharacterPosition(binding->character);
    ASSERT_TRUE(positionAfter.has_value());
    EXPECT_GT(motor->desiredVelocity.z, 1.5f);
    EXPECT_LT(std::abs(motor->velocity.z), 0.05f);
    EXPECT_LT(std::abs(positionAfter->z - positionBefore->z), 0.001f);
    EXPECT_TRUE(movement->physicallyBlocked);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, NPCWallSlidingPreservesTangentialObservedMovement)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const physics::PhysicsBodyDescriptor wall{
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 20.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 2.0f } },
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(wall).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    SetEntityMoveIntent(npc, 1.0f, 1.0f);

    for (int frame = 0; frame < 180; ++frame)
    {
        StepFrame();
    }

    const auto& world = gameplayRuntime.GetWorld();
    const auto* motor = world.TryGetCharacterMotor(npc);
    const auto* movement = world.TryGetCharacterMovementState(npc);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);
    const auto* transform = world.TryGetTransform(npc);
    ASSERT_NE(transform, nullptr);
    EXPECT_GT(std::abs(motor->velocity.x), 0.5f);
    EXPECT_NEAR(transform->position.z, 1.53f, 0.08f);
    EXPECT_LT(std::abs(motor->velocity.z), 0.1f);
    EXPECT_FALSE(movement->physicallyBlocked);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, CharacterTeleportResetsObservedVelocity)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    StepFrame();
    auto& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(npc);
    ASSERT_NE(binding, nullptr);
    ASSERT_TRUE(physicsWorld.TeleportCharacter(
        binding->character, {2.0f, 0.65f, 0.0f}));
    const auto velocity = physicsWorld.GetCharacterVelocity(binding->character);
    ASSERT_TRUE(velocity.has_value());
    EXPECT_LT(mathUtils::Length(*velocity), 0.001f);
    const auto teleportObservation =
        physicsWorld.ConsumeCharacterMotionObservation(binding->character);
    ASSERT_TRUE(teleportObservation.has_value());
    EXPECT_EQ(teleportObservation->stepCount, 0u);
    SetEntityMoveIntent(npc, 1.0f, 0.0f);
    StepFrame();
    const auto resumedVelocity = physicsWorld.GetCharacterVelocity(binding->character);
    ASSERT_TRUE(resumedVelocity.has_value());
    EXPECT_GT(resumedVelocity->x, 0.1f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, MixedThirtyFpsFrameCountsOnlyBlockedFixedStep)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    ASSERT_TRUE(physicsWorld.CreateBody({
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 0.75f } },
        .motionType = physics::PhysicsMotionType::Static }).IsValid());
    const auto npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    StepFrame();
    auto& world = gameplayRuntime.GetWorld();
    const auto* binding = world.TryGetPhysicsCharacter(npc);
    auto* motor = world.TryGetCharacterMotor(npc);
    auto* movement = world.TryGetCharacterMovementState(npc);
    ASSERT_NE(binding, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(movement, nullptr);

    ASSERT_TRUE(physicsWorld.TeleportCharacter(binding->character, {0.0f, 0.65f, 0.225f}));
    motor->desiredVelocity = {0.0f, 0.0f, 2.0f};
    ASSERT_TRUE(physicsWorld.SetCharacterDesiredVelocity(
        binding->character, motor->desiredVelocity));
    ASSERT_EQ(physicsWorld.Update(1.0f / 30.0f), 2u);
    ASSERT_TRUE(appRuntime::ApplyGameplayPhysicsCharacterFeedback(
        gameplayRuntime, physicsWorld));

    EXPECT_NEAR(movement->physicalBlockedSeconds, StepSeconds, 0.001f);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, BlockedPersistenceAt120FpsUsesFixedSteps)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    const physics::PhysicsBodyDescriptor wall{
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 0.75f } },
        .motionType = physics::PhysicsMotionType::Static
    };
    ASSERT_TRUE(physicsWorld.CreateBody(wall).IsValid());
    const rendern::EntityHandle npc = SpawnNPC();
    ASSERT_NE(npc, rendern::kNullEntity);
    SetEntityMoveIntent(npc, 0.0f, 1.0f);

    const float initialBlockedSeconds =
        gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicalBlockedSeconds;
    StepFrame(1.0f / 120.0f);
    EXPECT_FLOAT_EQ(
        gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicalBlockedSeconds,
        initialBlockedSeconds);
    for (int frame = 0; frame < 240 &&
        !gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicallyBlocked; ++frame)
    {
        StepFrame(1.0f / 120.0f);
    }
    EXPECT_TRUE(gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicallyBlocked);
    EXPECT_NEAR(gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicalBlockedSeconds,
        0.25f, StepSeconds);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, BlockedPersistenceAt60FpsUsesFixedSteps)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    ASSERT_TRUE(physicsWorld.CreateBody({
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 0.75f } },
        .motionType = physics::PhysicsMotionType::Static }).IsValid());
    const auto npc = SpawnNPC(); SetEntityMoveIntent(npc, 0.0f, 1.0f);
    for (int frame = 0; frame < 120 &&
        !gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicallyBlocked; ++frame)
    {
        StepFrame(1.0f / 60.0f);
    }
    EXPECT_TRUE(gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicallyBlocked);
    EXPECT_NEAR(gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicalBlockedSeconds,
        0.25f, StepSeconds);
}

TEST_F(GameplayPhysicsCharacterIntegrationTest, BlockedPersistenceAt30FpsUsesFixedSteps)
{
    ASSERT_TRUE(physicsWorld.CreateBody(FloorDescriptor()).IsValid());
    ASSERT_TRUE(physicsWorld.CreateBody({
        .shape = physics::BoxShapeDescriptor{ .halfExtents = { 3.0f, 2.0f, 0.25f } },
        .transform = { .position = { 0.0f, 2.0f, 0.75f } },
        .motionType = physics::PhysicsMotionType::Static }).IsValid());
    const auto npc = SpawnNPC(); SetEntityMoveIntent(npc, 0.0f, 1.0f);
    for (int frame = 0; frame < 60 &&
        !gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicallyBlocked; ++frame)
    {
        StepFrame(1.0f / 30.0f);
    }
    const float blockedSeconds =
        gameplayRuntime.GetWorld().TryGetCharacterMovementState(npc)->physicalBlockedSeconds;

    EXPECT_GE(blockedSeconds, 0.25f);
    EXPECT_LT(blockedSeconds, 0.25f + 2.0f * StepSeconds + 1e-4f);
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
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
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
    ASSERT_TRUE(appRuntime::EnsureGameplayPhysicsCharacters(
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
