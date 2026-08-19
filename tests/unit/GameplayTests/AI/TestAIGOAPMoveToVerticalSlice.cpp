#include <gtest/gtest.h>

#include <array>
#include <optional>

import core;

using namespace rendern;

namespace
{
    class StaticMoveToProvider final : public IAIMoveToActionRequestProvider
    {
    public:
        explicit StaticMoveToProvider(AIMoveToActionRequest inRequest)
            : request(inRequest) {}
        AIMoveToActionRequest request{};
        AIActionRuntimeContext received{};
        std::optional<AIMoveToActionRequest> ResolveRequest(
            const AIActionRuntimeContext& context) override
        {
            received = context;
            return request;
        }
    };

    EntityHandle AddAgent(GameplayWorld& world)
    {
        const EntityHandle agent = world.CreateEntity();
        world.AddAI(agent); world.AddTransform(agent, {});
        world.AddCharacterCommand(agent, {}); world.AddCharacterMotor(agent, {});
        world.AddCharacterMovementState(agent, {});
        return agent;
    }
}

TEST(AIGOAPMoveToVerticalSlice, PlansMovesSteersAndCompletesWithoutMutatingFacts)
{
    constexpr AIWorldFactId atDestination{0u};
    GameplayWorld world{}; AISystem system{};
    GameplayTraversalLinkRegistry links{}; GameplayTraversalExecutorRegistry executors{};
    const EntityHandle agent = AddAgent(world);
    const GameplayRouteNodeId startNodeId{1u};
    const GameplayRouteNodeId goalNodeId{2u};

    const GameplayRouteGraph graph{
        .nodes = {
            GameplayRouteGraphNode{
                .nodeId = startNodeId,
                .worldPosition = {0.0f, 0.0f, 0.0f}
            },
            GameplayRouteGraphNode{
                .nodeId = goalNodeId,
                .worldPosition = {5.0f, 0.0f, 0.0f}
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
    StaticMoveToProvider provider(AIMoveToActionRequest{
        .routeGraph = &graph,
        .startNodeId = startNodeId,
        .goalNodeId = goalNodeId,
        .steeringSettings = {}
    });
    AIMoveToActionBinding binding(world, links, executors, provider);
    AIActionBindingRegistry registry{};
    ASSERT_TRUE(registry.Register(kAIMoveToActionId, binding));

    AIAgentWorldState initialState{};
    const AIAgentWorldState originalState = initialState;
    const AIGoalDefinition goal{
        AIGoalId{1u},
        {
            AIFactCondition{atDestination, true}
        }
    };
    const std::array actions{AIActionDefinition{
        kAIMoveToActionId,
        {{atDestination, false}},
        {{atDestination, true}},
        1.0f}};
    const std::optional<AIPlan> plan = FindAIPlan(initialState, goal, actions);
    ASSERT_TRUE(plan); ASSERT_EQ(plan->steps.size(), 1u);
    EXPECT_EQ(plan->steps.front().actionId, kAIMoveToActionId);

    AIPlanExecution execution(*plan);
    ASSERT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(AIPlanExecutionBridge::StartReadyPlanStep(
        execution, registry, system, world, agent), AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(provider.received, (AIActionRuntimeContext{agent, kAIMoveToActionId}));
    EXPECT_EQ(system.GetActionStatus(agent, kAIMoveToActionId), AIActionExecutionStatus::Running);
    EXPECT_EQ(system.GetActionStatus(agent, kAIFollowRouteActionId), AIActionExecutionStatus::NotStarted);

    system.Update(world, 1.0f / 60.0f);
    ASSERT_NE(world.TryGetCharacterCommand(agent), nullptr);
    EXPECT_GT(world.TryGetCharacterCommand(agent)->moveWorld.x, 0.99f);
    world.TryGetTransform(agent)->position = {5.0f, 0.0f, 0.0f};
    system.Update(world, 1.0f / 60.0f);
    EXPECT_EQ(AIPlanExecutionBridge::SynchronizeRunningPlanStep(execution, system, agent),
        AIPlanExecutionStatus::Succeeded);
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_EQ(initialState, originalState);
}

TEST(AIGOAPMoveToVerticalSlice, StartEqualsGoalCompletesSynchronously)
{
    GameplayWorld world{}; AISystem system{}; GameplayTraversalLinkRegistry links{};
    GameplayTraversalExecutorRegistry executors{}; const EntityHandle agent = AddAgent(world);
    const GameplayRouteNodeId nodeId{1u};

    const GameplayRouteGraph graph{
        .nodes = {
            GameplayRouteGraphNode{
                .nodeId = nodeId,
                .worldPosition = {0.0f, 0.0f, 0.0f}
            }
        }
    };

    StaticMoveToProvider provider(AIMoveToActionRequest{
        .routeGraph = &graph,
        .startNodeId = nodeId,
        .goalNodeId = nodeId,
        .steeringSettings = {}
    });
    AIMoveToActionBinding binding(world, links, executors, provider);
    AIActionBindingRegistry registry{}; ASSERT_TRUE(registry.Register(kAIMoveToActionId, binding));
    AIPlan plan{
        .goalId = AIGoalId{1u},
        .steps = {
            AIPlanStep{kAIMoveToActionId}
        }
    };
    AIPlanExecution execution(std::move(plan));
    ASSERT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(AIPlanExecutionBridge::StartReadyPlanStep(execution, registry, system, world, agent),
        AIPlanExecutionStatus::Succeeded);
}