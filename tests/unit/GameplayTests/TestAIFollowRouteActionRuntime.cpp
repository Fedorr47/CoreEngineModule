#include <array>
#include <gtest/gtest.h>

import core;

using namespace rendern;

namespace
{
    [[nodiscard]] GameplayRoute MakeRoute(const float destinationX = 2.0f)
    {
        return GameplayRoute{
            .points = {
                GameplayRoutePoint{.worldPosition = {0.0f, 0.0f, 0.0f}},
                GameplayRoutePoint{.worldPosition = {destinationX, 0.0f, 0.0f}}
            },
            .segmentAnnotations = {GameplayRouteSegmentAnnotation{}}
        };
    }

    [[nodiscard]] AIActionRuntimeContext MakeContext(const EntityHandle entity)
    {
        return AIActionRuntimeContext{
            .agentEntity = entity,
            .actionId = kAIFollowRouteActionId
        };
    }
    
    [[nodiscard]] EntityHandle CreateMovingAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, GameplayTransformComponent{});
        world.AddCharacterCommand(entity, GameplayCharacterCommandComponent{});
        world.AddCharacterMotor(entity, GameplayCharacterMotorComponent{});
        world.AddCharacterMovementState(entity, GameplayCharacterMovementStateComponent{});
        return entity;
    }
}

// Protects AI/camera ownership so route execution updates character body
// facing without requiring or modifying a follow-camera component.
TEST(AIFollowRouteActionRuntime, TickUpdatesBodyFacingWithoutModifyingCameraFacing)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateMovingAgent(world);

    ASSERT_FALSE(world.HasFollowCamera(agent));

    GameplayCharacterMovementStateComponent* movementState =
        world.TryGetCharacterMovementState(agent);

    ASSERT_NE(movementState, nullptr);

    movementState->desiredFacingYawDegrees = 0.0f;
    movementState->cameraFacingYawDegrees = 137.0f;

    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    AIFollowRouteActionRuntime runtime{
        world, traversalRegistry, traversalExecutorRegistry, MakeRoute(2.0f)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 1.0f / 60.0f), AIActionRuntimeResult::Running);

    EXPECT_NEAR(movementState->desiredFacingYawDegrees, 90.0f, 0.001f);
    EXPECT_FLOAT_EQ(movementState->cameraFacingYawDegrees, 137.0f);
    EXPECT_FALSE(world.HasFollowCamera(agent));
}