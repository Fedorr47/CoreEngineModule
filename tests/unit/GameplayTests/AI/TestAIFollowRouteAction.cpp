#include <gtest/gtest.h>

#include <utility>

import core;

using namespace rendern;

namespace
{
    [[nodiscard]] EntityHandle CreateFollowRouteAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, GameplayTransformComponent{});
        world.AddCharacterCommand(entity, GameplayCharacterCommandComponent{});
        world.AddCharacterMotor(entity, GameplayCharacterMotorComponent{});
        world.AddCharacterMovementState(entity, GameplayCharacterMovementStateComponent{});
        return entity;
    }

    [[nodiscard]] GameplayRoute MakeFollowRoute(const mathUtils::Vec3 destination)
    {
        return GameplayRoute{
            .points = {
                GameplayRoutePoint{.worldPosition = {0.0f, 0.0f, 0.0f}},
                GameplayRoutePoint{.worldPosition = destination}
            },
            .segmentAnnotations = {GameplayRouteSegmentAnnotation{}}
        };
    }

    [[nodiscard]] GameplayRoute MakeOnePointRoute(const mathUtils::Vec3 point)
    {
        return GameplayRoute{
            .points = {
                GameplayRoutePoint{.worldPosition = point}
            }
        };
    }

    [[nodiscard]] GameplayRoute MakeTraversalRoute()
    {
        return GameplayRoute{
            .points = {
                GameplayRoutePoint{.worldPosition = {0.0f, 0.0f, 0.0f}},
                GameplayRoutePoint{.worldPosition = {5.0f, 0.0f, 0.0f}}
            },
            .segmentAnnotations = {
                GameplayRouteSegmentAnnotation{
                    .traversalLink = GameplayTraversalLinkHandle{77u}
                }
            }
        };
    }
}

// Protects the FollowRoute action boundary: valid ready routes should create a
// concrete runtime only after specialized validation succeeds.
TEST(AIFollowRouteAction, ValidRouteAndMovementAgentSubmitsRunningTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateFollowRouteAgent(world);

    EXPECT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeFollowRoute({5.0f, 0.0f, 0.0f})),
        AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects validation-before-submission so an invalid route cannot replace an
// already running task.
TEST(AIFollowRouteAction, InvalidRouteDoesNotReplaceExistingTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateFollowRouteAgent(world);

    ASSERT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeFollowRoute({5.0f, 0.0f, 0.0f})),
        AIActionExecutionStatus::Running);
    ASSERT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    const mathUtils::Vec3 previousMoveWorld = command->moveWorld;
    const float previousMoveMagnitude = command->moveMagnitude;

    GameplayRoute invalidRoute{
        .points = {
            GameplayRoutePoint{.worldPosition = {0.0f, 0.0f, 0.0f}},
            GameplayRoutePoint{.worldPosition = {5.0f, 0.0f, 0.0f}}
        }
    };
    ASSERT_FALSE(invalidRoute.IsValid());

    EXPECT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, std::move(invalidRoute)),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
    EXPECT_FLOAT_EQ(command->moveWorld.x, previousMoveWorld.x);
    EXPECT_FLOAT_EQ(command->moveWorld.y, previousMoveWorld.y);
    EXPECT_FLOAT_EQ(command->moveWorld.z, previousMoveWorld.z);
    EXPECT_FLOAT_EQ(command->moveMagnitude, previousMoveMagnitude);
}

// Protects validation-before-submission so missing movement components cannot
// cancel or replace an existing action.
TEST(AIFollowRouteAction, MissingMovementComponentsDoNotReplaceExistingTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateFollowRouteAgent(world);

    ASSERT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeFollowRoute({5.0f, 0.0f, 0.0f})),
        AIActionExecutionStatus::Running);

    world.RemoveCharacterMotor(agent);

    EXPECT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeFollowRoute({0.0f, 0.0f, 5.0f})),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects AI membership validation so ordinary entities cannot submit a
// FollowRoute runtime through the specialized action boundary.
TEST(AIFollowRouteAction, NonAIEntityDoesNotSubmitTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle nonAIEntity = world.CreateEntity();
    world.AddTransform(nonAIEntity, GameplayTransformComponent{});
    world.AddCharacterCommand(nonAIEntity, GameplayCharacterCommandComponent{});
    world.AddCharacterMotor(nonAIEntity, GameplayCharacterMotorComponent{});
    world.AddCharacterMovementState(nonAIEntity, GameplayCharacterMovementStateComponent{});

    EXPECT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, nonAIEntity, MakeFollowRoute({0.0f, 0.0f, 5.0f})),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(nonAIEntity), AIActionExecutionStatus::NotStarted);
}

// Protects the ready-route degenerate case: a one-point route should be
// submitted and complete successfully without movement.
TEST(AIFollowRouteAction, OnePointRouteCompletesSuccessfully)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateFollowRouteAgent(world);

    EXPECT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeOnePointRoute({0.0f, 0.0f, 0.0f})),
        AIActionExecutionStatus::Succeeded);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Succeeded);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    EXPECT_FLOAT_EQ(command->moveMagnitude, 0.0f);
}

// Protects the production unsupported executor boundary after successful link
// resolution rather than conflating unsupported behavior with a missing link.
TEST(AIFollowRouteAction, RegisteredTraversalFailsThroughUnsupportedExecutor)
{
    GameplayWorld world{};
    GameplayUnsupportedTraversalExecutor traversalExecutor{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateFollowRouteAgent(world);
    const EntityHandle target = world.CreateEntity();
    ASSERT_TRUE(traversalRegistry.Register({
        .handle = GameplayTraversalLinkHandle{77u},
        .traversalTypeId = kDoorTraversalTypeId,
        .targetEntity = target}));
    ASSERT_TRUE(traversalExecutorRegistry.Register(
        kDoorTraversalTypeId, traversalExecutor));

    ASSERT_EQ(
        AIFollowRouteAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, MakeTraversalRoute()),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Failed);
}