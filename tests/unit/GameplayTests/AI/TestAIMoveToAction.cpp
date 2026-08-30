#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    class ForwardBlockedQuery final : public IGameplayObstacleQuery
    {
    public:
        [[nodiscard]] bool Probe(
            const GameplayObstacleProbeRequest&,
            GameplayObstacleProbeHit& hit) const noexcept override
        {
            ++calls;
            if ((calls % 3u) == 1u)
            {
                hit.distance = 0.1f;
                return true;
            }
            return false;
        }
        mutable std::size_t calls{0u};
    };
    [[nodiscard]] GameplayRouteNodeId MoveNodeId(const std::uint64_t value) noexcept
    {
        return GameplayRouteNodeId{ value };
    }

    [[nodiscard]] GameplayRouteGraphNode MoveNode(
        const std::uint64_t id,
        const mathUtils::Vec3 position) noexcept
    {
        return GameplayRouteGraphNode{
            .nodeId = MoveNodeId(id),
            .worldPosition = position
        };
    }

    [[nodiscard]] GameplayRouteGraphEdge MoveEdge(
        const std::uint64_t from,
        const std::uint64_t to,
        const float cost,
        const GameplayRouteSegmentAnnotation annotation = {}) noexcept
    {
        return GameplayRouteGraphEdge{
            .fromNodeId = MoveNodeId(from),
            .toNodeId = MoveNodeId(to),
            .cost = cost,
            .annotation = annotation
        };
    }

    [[nodiscard]] EntityHandle CreateMoveToAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, GameplayTransformComponent{});
        world.AddCharacterCommand(entity, GameplayCharacterCommandComponent{});
        world.AddCharacterMotor(entity, GameplayCharacterMotorComponent{});
        world.AddCharacterMovementState(entity, GameplayCharacterMovementStateComponent{});
        return entity;
    }

    [[nodiscard]] GameplayRouteGraph MakeLinearMoveToGraph(
        const mathUtils::Vec3 start,
        const mathUtils::Vec3 goal)
    {
        return GameplayRouteGraph{
            .nodes = {
                MoveNode(1u, start),
                MoveNode(2u, goal)
            },
            .edges = {
                MoveEdge(1u, 2u, 1.0f)
            }
        };
    }

    void ExpectMoveToGraphUnchanged(
        const GameplayRouteGraph& actual,
        const GameplayRouteGraph& expected)
    {
        ASSERT_EQ(actual.nodes.size(), expected.nodes.size());
        ASSERT_EQ(actual.edges.size(), expected.edges.size());

        for (std::size_t index = 0; index < actual.nodes.size(); ++index)
        {
            EXPECT_EQ(actual.nodes[index].nodeId, expected.nodes[index].nodeId);
            EXPECT_FLOAT_EQ(actual.nodes[index].worldPosition.x, expected.nodes[index].worldPosition.x);
            EXPECT_FLOAT_EQ(actual.nodes[index].worldPosition.y, expected.nodes[index].worldPosition.y);
            EXPECT_FLOAT_EQ(actual.nodes[index].worldPosition.z, expected.nodes[index].worldPosition.z);
        }

        for (std::size_t index = 0; index < actual.edges.size(); ++index)
        {
            EXPECT_EQ(actual.edges[index].fromNodeId, expected.edges[index].fromNodeId);
            EXPECT_EQ(actual.edges[index].toNodeId, expected.edges[index].toNodeId);
            EXPECT_FLOAT_EQ(actual.edges[index].cost, expected.edges[index].cost);
            EXPECT_EQ(actual.edges[index].annotation.traversalLink, expected.edges[index].annotation.traversalLink);
        }
    }
}

// Protects the MoveTo action from bypassing weighted search: the lower-cost
// intermediate route must be the route consumed by the existing follower.
TEST(AIMoveToAction, LowerCostBranchIsConsumedByFollower)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    GameplayRouteGraph graph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f}),
            MoveNode(2u, {10.0f, 0.0f, 0.0f}),
            MoveNode(3u, {0.0f, 0.0f, 10.0f})
        },
        .edges = {
            MoveEdge(1u, 3u, 10.0f),
            MoveEdge(1u, 2u, 1.0f),
            MoveEdge(2u, 3u, 1.0f)
        }
    };

    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, graph, MoveNodeId(1u), MoveNodeId(3u)),
        AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);
    EXPECT_EQ(aiSystem.GetActionStatus(agent, kAIMoveToActionId), AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.GetActionStatus(agent, kAIFollowRouteActionId), AIActionExecutionStatus::NotStarted);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    EXPECT_GT(command->moveWorld.x, 0.99f);
    EXPECT_NEAR(command->moveWorld.z, 0.0f, 0.001f);
    EXPECT_GT(command->moveMagnitude, 0.0f);
}

// Protects running actions from destructive failed path queries: a disconnected
// MoveTo request must not cancel the task or clear movement already produced.
TEST(AIMoveToAction, NoRouteDoesNotReplaceActiveAction)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph activeGraph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f});
    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, activeGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);
    ASSERT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    const mathUtils::Vec3 previousMoveWorld = command->moveWorld;
    const float previousMoveMagnitude = command->moveMagnitude;

    const GameplayRouteGraph disconnectedGraph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f}),
            MoveNode(2u, {5.0f, 0.0f, 0.0f})
        }
    };

    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, disconnectedGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
    EXPECT_FLOAT_EQ(command->moveWorld.x, previousMoveWorld.x);
    EXPECT_FLOAT_EQ(command->moveWorld.y, previousMoveWorld.y);
    EXPECT_FLOAT_EQ(command->moveWorld.z, previousMoveWorld.z);
    EXPECT_FLOAT_EQ(command->moveMagnitude, previousMoveMagnitude);
}

// Protects route-search validation ordering so malformed graph data cannot
// replace or cancel an existing per-agent task.
TEST(AIMoveToAction, InvalidGraphDoesNotReplaceActiveAction)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph activeGraph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f});
    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, activeGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);

    const GameplayRouteGraph invalidGraph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f}),
            MoveNode(1u, {1.0f, 0.0f, 0.0f})
        }
    };

    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, invalidGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects request validation before mutation: invalid endpoint IDs must fail
// without replacing the currently running movement action.
TEST(AIMoveToAction, InvalidEndpointDoesNotReplaceActiveAction)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph activeGraph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f});
    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, activeGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, activeGraph, GameplayRouteNodeId{}, MoveNodeId(2u)),
        AIActionExecutionStatus::Failed);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects the single-action ownership rule: a resolved MoveTo request may
// reuse existing generic replacement semantics for the same agent.
TEST(AIMoveToAction, SuccessfulRequestReplacesPreviousAction)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph firstGraph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f});
    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, firstGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);

    const GameplayRouteGraph secondGraph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 10.0f});
    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, secondGraph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    EXPECT_NEAR(command->moveWorld.x, 0.0f, 0.001f);
    EXPECT_GT(command->moveWorld.z, 0.99f);
}

// Protects the degenerate valid-route contract: start==goal should consume the
// one-point weighted-search result and finish without producing movement.
TEST(AIMoveToAction, StartEqualsGoalSucceedsWithoutMovement)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph graph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f})
        }
    };

    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, graph, MoveNodeId(1u), MoveNodeId(1u)),
        AIActionExecutionStatus::Succeeded);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Succeeded);

    const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agent);
    ASSERT_NE(command, nullptr);
    EXPECT_FLOAT_EQ(command->moveMagnitude, 0.0f);
    EXPECT_FLOAT_EQ(command->moveWorld.x, 0.0f);
    EXPECT_FLOAT_EQ(command->moveWorld.y, 0.0f);
    EXPECT_FLOAT_EQ(command->moveWorld.z, 0.0f);
}

// Protects selected segment metadata from being rebuilt or dropped by MoveTo:
// unsupported traversal must reach the follower and fail during action ticking.
TEST(AIMoveToAction, SelectedRouteAnnotationsReachFollowerUnchanged)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);
    const GameplayRouteSegmentAnnotation traversalAnnotation{
        .traversalLink = GameplayTraversalLinkHandle{77u}
    };

    const GameplayRouteGraph graph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f}),
            MoveNode(2u, {5.0f, 0.0f, 0.0f})
        },
        .edges = {
            MoveEdge(1u, 2u, 1.0f, traversalAnnotation)
        }
    };

    ASSERT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, graph, MoveNodeId(1u), MoveNodeId(2u)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Failed);
}

// Protects graph ownership: route resolution must read the source graph without
// mutating nodes, edges, costs, or selected annotations.
TEST(AIMoveToAction, SourceGraphRemainsUnchanged)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry traversalRegistry{};
    GameplayTraversalExecutorRegistry traversalExecutorRegistry{};
    const EntityHandle agent = CreateMoveToAgent(world);
    GameplayRouteGraph graph{
        .nodes = {
            MoveNode(1u, {0.0f, 0.0f, 0.0f}),
            MoveNode(2u, {5.0f, 0.0f, 0.0f}),
            MoveNode(3u, {0.0f, 0.0f, 5.0f})
        },
        .edges = {
            MoveEdge(1u, 2u, 1.5f),
            MoveEdge(2u, 3u, 2.5f)
        }
    };
    const GameplayRouteGraph originalGraph = graph;

    EXPECT_EQ(
        AIMoveToAction::Start(aiSystem, world, traversalRegistry, traversalExecutorRegistry, agent, graph, MoveNodeId(1u), MoveNodeId(3u)),
        AIActionExecutionStatus::Running);

    ExpectMoveToGraphUnchanged(graph, originalGraph);
}

// Protects the runtime facade boundary and thread-affinity contract while route
// resolution remains owned by the specialized MoveTo action.
TEST(GameplayRuntime, StartAIMoveToForwardsThroughSpecializedAction)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime runtime{};
    GameplayWorld& world = runtime.GetWorld();
    const EntityHandle agent = CreateMoveToAgent(world);

    const GameplayRouteGraph graph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 10.0f});

    EXPECT_EQ(runtime.StartAIMoveTo(agent, graph, MoveNodeId(1u), MoveNodeId(2u)), AIActionExecutionStatus::Running);
    EXPECT_EQ(runtime.GetAIActionStatus(agent), AIActionExecutionStatus::Running);
}

TEST(AIMoveToAction, FacadePropagatesObstacleQueryWithoutDirectRuntimeConstruction)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{};
    const EntityHandle agent = CreateMoveToAgent(world);
    world.AddCharacterPhysicalSettings(agent);
    const GameplayRouteGraph graph = MakeLinearMoveToGraph(
        {0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f});
    ForwardBlockedQuery query{};

    ASSERT_EQ(AIMoveToAction::Start(
        aiSystem, world, links, executors, agent, graph, MoveNodeId(1u), MoveNodeId(2u),
        {}, &query), AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.Update(world, 1.0f / 60.0f), 1u);
    EXPECT_EQ(query.calls, 3u);
    EXPECT_LT(world.TryGetCharacterCommand(agent)->moveWorld.z, 0.0f);
}
