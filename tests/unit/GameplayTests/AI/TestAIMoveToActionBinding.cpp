#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <unordered_map>

import core;

using namespace rendern;

namespace
{
    EntityHandle AddMovementAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        world.AddTransform(entity, {});
        world.AddCharacterCommand(entity, {});
        world.AddCharacterMotor(entity, {});
        world.AddCharacterMovementState(entity, {});
        return entity;
    }

    GameplayRouteGraph Graph(const float goalX)
    {
        const GameplayRouteNodeId startNodeId{1u};
        const GameplayRouteNodeId goalNodeId{2u};

        return GameplayRouteGraph{
            .nodes = {
                GameplayRouteGraphNode{
                    .nodeId = startNodeId,
                    .worldPosition = {0.0f, 0.0f, 0.0f}
                },
                GameplayRouteGraphNode{
                    .nodeId = goalNodeId,
                    .worldPosition = {goalX, 0.0f, 0.0f}
                }
            },
            .edges = {
                GameplayRouteGraphEdge{
                    .fromNodeId = startNodeId,
                    .toNodeId = goalNodeId,
                    .cost = 1.0f
                }
            }
        };
    }
    
    class Provider final : public IAIMoveToActionRequestProvider
    {
    public:
        std::unordered_map<EntityHandle, AIMoveToActionRequest> requests{};
        AIActionRuntimeContext received{};
        std::optional<AIMoveToActionRequest> ResolveRequest(
            const AIActionRuntimeContext& context) override
        {
            received = context;
            const auto it = requests.find(context.agentEntity);
            return it == requests.end() ? std::nullopt : std::optional{it->second};
        }
    };
    class ReservationProvider final : public IAIActionReservationTargetProvider
    {
    public:
        EntityHandle target{kNullEntity};
        EntityHandle ResolveReservationTarget(const AIActionRuntimeContext&) override
        {
            return target;
        }
    };
}

TEST(AIMoveToActionBinding, ReservedMoveAcquiresBlocksCompetitorAndCancelReleases)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle first = AddMovementAgent(world);
    const EntityHandle second = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    const AIMoveToActionRequest request{&graph, GameplayRouteNodeId{1u},
        GameplayRouteNodeId{2u}, {}};
    provider.requests[first] = request;
    provider.requests[second] = request;
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext firstContext{first, kAIMoveToActionId};
    const AIActionRuntimeContext secondContext{second, kAIMoveToActionId};
    auto firstRuntime = binding.CreateRuntime(firstContext);
    auto secondRuntime = binding.CreateRuntime(secondContext);
    ASSERT_NE(firstRuntime, nullptr);
    ASSERT_NE(secondRuntime, nullptr);

    EXPECT_EQ(firstRuntime->Start(firstContext), AIActionRuntimeResult::Running);
    EXPECT_TRUE(reservations.IsReservedBy(resource, first));
    EXPECT_EQ(secondRuntime->Start(secondContext), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(reservations.IsReservedBy(resource, first));
    firstRuntime->Cancel(firstContext);
    firstRuntime->Cancel(firstContext);
    EXPECT_FALSE(reservations.IsReserved(resource));
}

TEST(AIMoveToActionBinding, ReservedMoveDoesNotAdoptOrReleasePreExistingSameOwner)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle agent = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    provider.requests[agent] = {&graph, GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, {}};
    ASSERT_TRUE(reservations.TryReserve(world, resource, agent));
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext context{agent, kAIMoveToActionId};
    auto runtime = binding.CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Failed);
    runtime->Cancel(context);
    EXPECT_TRUE(reservations.IsReservedBy(resource, agent));
}

TEST(AIMoveToActionBinding, LostOwnershipFailsWithoutReleasingReplacementOwner)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle first = AddMovementAgent(world);
    const EntityHandle second = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    provider.requests[first] = {&graph, GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, {}};
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext context{first, kAIMoveToActionId};
    auto runtime = binding.CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    ASSERT_EQ(runtime->Start(context), AIActionRuntimeResult::Running);
    ASSERT_TRUE(reservations.Release(resource, first));
    ASSERT_TRUE(reservations.TryReserve(world, resource, second));
    EXPECT_EQ(runtime->Tick(context, 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_TRUE(reservations.IsReservedBy(resource, second));
}

TEST(AIMoveToActionBinding, SuccessReleasesReservation)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle agent = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    provider.requests[agent] = {&graph, GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, {}};
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext context{agent, kAIMoveToActionId};
    auto runtime = binding.CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    ASSERT_EQ(runtime->Start(context), AIActionRuntimeResult::Running);
    ASSERT_TRUE(reservations.IsReservedBy(resource, agent));

    world.TryGetTransform(agent)->position = {5.0f, 0.0f, 0.0f};
    EXPECT_EQ(runtime->Tick(context, 0.016f), AIActionRuntimeResult::Succeeded);
    EXPECT_FALSE(reservations.IsReserved(resource));
}

TEST(AIMoveToActionBinding, RuntimeFailureReleasesReservation)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle agent = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    provider.requests[agent] = {&graph, GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, {}};
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext context{agent, kAIMoveToActionId};
    auto runtime = binding.CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    ASSERT_EQ(runtime->Start(context), AIActionRuntimeResult::Running);
    ASSERT_TRUE(reservations.IsReservedBy(resource, agent));

    world.RemoveCharacterMotor(agent);
    EXPECT_EQ(runtime->Tick(context, 0.016f), AIActionRuntimeResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(resource));
}

TEST(AIMoveToActionBinding, StartFailureAfterReservationDoesNotLeak)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    GameplayObjectReservationSystem reservations{}; ReservationProvider targets{};
    const EntityHandle agent = AddMovementAgent(world);
    const EntityHandle resource = world.CreateEntity();
    world.AddInteractionPoint(resource, {});
    targets.target = resource;
    const GameplayRouteGraph graph = Graph(5.0f);
    provider.requests[agent] = {&graph, GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, {}};
    AIMoveToActionBinding binding(world, links, executors, provider, reservations, targets);
    const AIActionRuntimeContext context{agent, kAIMoveToActionId};
    auto runtime = binding.CreateRuntime(context);
    ASSERT_NE(runtime, nullptr);
    world.RemoveCharacterMotor(agent);

    EXPECT_EQ(runtime->Start(context), AIActionRuntimeResult::Failed);
    EXPECT_FALSE(reservations.IsReserved(resource));
    runtime->Cancel(context);
    EXPECT_FALSE(reservations.IsReserved(resource));
}

TEST(AIMoveToActionBinding, SemanticIdIsValidAndDistinctFromFollowRoute)
{
    EXPECT_TRUE(kAIMoveToActionId.IsValid());
    EXPECT_NE(kAIMoveToActionId, kAIFollowRouteActionId);
}

TEST(AIMoveToActionBinding, MissingAndInvalidRequestsReturnNoRuntime)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    AIMoveToActionBinding binding(world, links, executors, provider);
    const EntityHandle agent = AddMovementAgent(world);
    const AIActionRuntimeContext context{agent, kAIMoveToActionId};

    EXPECT_EQ(binding.CreateRuntime(context), nullptr);
    EXPECT_EQ(provider.received, context);
    AIActionBindingRegistry registry{};
    ASSERT_TRUE(registry.Register(kAIMoveToActionId, binding));
    AIPlanExecution execution(
    AIPlan{
        .goalId = AIGoalId{1u},
        .steps = {
            AIPlanStep{kAIMoveToActionId}
        }
    });
    AISystem system{};
    ASSERT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(AIPlanExecutionBridge::StartReadyPlanStep(
        execution, registry, system, world, agent), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(system.GetActionStatus(agent), AIActionExecutionStatus::NotStarted);

    provider.requests[agent] = AIMoveToActionRequest{};
    EXPECT_EQ(binding.CreateRuntime(context), nullptr);
    EXPECT_EQ(binding.CreateRuntime({agent, kAIFollowRouteActionId}), nullptr);
}

TEST(AIMoveToActionBinding, OneBindingResolvesDifferentAgents)
{
    GameplayWorld world{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; Provider provider{};
    AIMoveToActionBinding binding(world, links, executors, provider);
    const EntityHandle first = AddMovementAgent(world);
    const EntityHandle second = AddMovementAgent(world);
    const GameplayRouteGraph firstGraph = Graph(5.0f);
    const GameplayRouteGraph secondGraph = Graph(9.0f);
    provider.requests[first] = AIMoveToActionRequest{
        .routeGraph = &firstGraph,
        .startNodeId = GameplayRouteNodeId{1u},
        .goalNodeId = GameplayRouteNodeId{2u},
        .steeringSettings = {}
    };

    provider.requests[second] = AIMoveToActionRequest{
        .routeGraph = &secondGraph,
        .startNodeId = GameplayRouteNodeId{1u},
        .goalNodeId = GameplayRouteNodeId{2u},
        .steeringSettings = {}
    };

    EXPECT_NE(binding.CreateRuntime({first, kAIMoveToActionId}), nullptr);
    EXPECT_EQ(provider.received.agentEntity, first);
    EXPECT_NE(binding.CreateRuntime({second, kAIMoveToActionId}), nullptr);
    EXPECT_EQ(provider.received.agentEntity, second);
}