#include <array>
#include <gtest/gtest.h>

import core;

using namespace rendern;

namespace
{
    class ForwardBlockedQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest& request,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            requests[calls < 3u ? calls : 2u] = request;
            ++calls;
            if ((calls % 3u) == 1u)
            {
                hit.distance = 0.1f;
                return true;
            }
            return false;
        }
        mutable std::size_t calls{0u};
        mutable GameplayObstacleProbeRequest requests[3]{};
    };
    
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

TEST(AIFollowRouteActionRuntime, ExposesPhysicalBlockedFeedbackWithoutAddingRoutePolicy)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateMovingAgent(world);
    world.TryGetCharacterMovementState(agent)->physicallyBlocked = true;
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    AIFollowRouteActionRuntime runtime{
        world, traversalRegistry, traversalExecutorRegistry, MakeRoute(2.0f)};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.Tick(MakeContext(agent), 1.0f / 60.0f), AIActionRuntimeResult::Running);
    EXPECT_TRUE(runtime.IsPhysicallyBlocked());
    runtime.Cancel(MakeContext(agent));
    EXPECT_FALSE(runtime.IsPhysicallyBlocked());
}

TEST(AIFollowRouteActionRuntime, AvoidanceCorrectsFollowingAndPreservesArrivalMagnitude)
{
    GameplayWorld world{};
    const EntityHandle agent = CreateMovingAgent(world);
    world.AddCharacterPhysicalSettings(agent);
    GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{};
    ForwardBlockedQuery query{};
    AIFollowRouteActionRuntime runtime{
        world, links, executors, MakeRoute(0.5f),
        {.acceptanceRadius = 0.1f, .slowingRadius = 1.0f}, &query};

    ASSERT_EQ(runtime.Start(MakeContext(agent)), AIActionRuntimeResult::Running);
    ASSERT_EQ(runtime.Tick(MakeContext(agent), 1.0f / 60.0f), AIActionRuntimeResult::Running);
    const auto* command = world.TryGetCharacterCommand(agent);
    EXPECT_LT(command->moveWorld.z, 0.0f);
    EXPECT_NEAR(command->moveMagnitude, 4.0f / 9.0f, 0.0001f);
    EXPECT_EQ(query.calls, 3u);
    
    for (const GameplayObstacleProbeRequest& request : query.requests)
    {
        EXPECT_FLOAT_EQ(request.clearanceRadius, 0.32f);
    }
}