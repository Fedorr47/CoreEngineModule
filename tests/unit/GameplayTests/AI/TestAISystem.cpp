#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

import core;

using namespace rendern;

namespace
{
    inline constexpr AIActionId kTestActionId{42u};
    inline constexpr AIActionId kReplacementActionId{43u};

    struct FakeActionRuntimeState
    {
        bool bStartCalled{false};
        bool bTickCalled{false};
        bool bCancelCalled{false};
        AIActionRuntimeContext lastContext{};
        float lastDeltaSeconds{0.0f};
        AIActionRuntimeResult startResult{AIActionRuntimeResult::Running};
        AIActionRuntimeResult tickResult{AIActionRuntimeResult::Running};
    };

    class FakeActionRuntime final : public IAIActionRuntime
    {
    public:
        explicit FakeActionRuntime(FakeActionRuntimeState& state) noexcept
            : state_(&state)
        {
        }

        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            state_->bStartCalled = true;
            state_->lastContext = context;
            return state_->startResult;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            state_->bTickCalled = true;
            state_->lastContext = context;
            state_->lastDeltaSeconds = deltaSeconds;
            return state_->tickResult;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            state_->bCancelCalled = true;
            state_->lastContext = context;
        }

    private:
        FakeActionRuntimeState* state_{};
    };

    [[nodiscard]] std::unique_ptr<IAIActionRuntime> MakeFakeRuntime(
        FakeActionRuntimeState& state)
    {
        return std::make_unique<FakeActionRuntime>(state);
    }

    [[nodiscard]] AIActionRuntimeContext MakeActionContext(
        const EntityHandle agentEntity,
        const AIActionId actionId = kTestActionId) noexcept
    {
        return AIActionRuntimeContext{
            .agentEntity = agentEntity,
            .actionId = actionId
        };
    }
}

// Protects the ownership contract that AI-agent membership is derived from
// AIComponent presence and does not affect unrelated gameplay components.
TEST(AISystem, AIComponentLifecycleControlsAgentMembership)
{
    GameplayWorld world{};
    const EntityHandle entity = world.CreateEntity();

    world.AddTransform(entity, GameplayTransformComponent{ .position = { 1.0f, 2.0f, 3.0f } });
    world.AddAI(entity);

    std::vector<EntityHandle> discoveredEntities{};
    world.CollectAIEntities(discoveredEntities);

    EXPECT_TRUE(world.HasAI(entity));
    EXPECT_EQ(discoveredEntities, std::vector<EntityHandle>{ entity });

    world.RemoveAI(entity);
    world.CollectAIEntities(discoveredEntities);

    EXPECT_FALSE(world.HasAI(entity));
    EXPECT_TRUE(discoveredEntities.empty());
    ASSERT_NE(world.TryGetTransform(entity), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.x, 1.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.y, 2.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.z, 3.0f);
}

// Protects the filtering boundary so non-AI and destroyed entities cannot
// remain in a stale registration list and reach future AI behavior.
TEST(AISystem, DiscoveryContainsOnlyLiveEntitiesWithAIComponent)
{
    GameplayWorld world{};

    const EntityHandle liveAI = world.CreateEntity();
    const EntityHandle nonAI = world.CreateEntity();
    const EntityHandle destroyedAI = world.CreateEntity();

    world.AddAI(liveAI);
    world.AddAI(destroyedAI);
    world.DestroyEntity(destroyedAI);

    std::vector<EntityHandle> discoveredEntities{};
    world.CollectAIEntities(discoveredEntities);

    EXPECT_EQ(discoveredEntities, std::vector<EntityHandle>{ liveAI });
    EXPECT_TRUE(world.IsEntityValid(nonAI));
    EXPECT_FALSE(world.HasAI(nonAI));
}

// Protects deterministic AI traversal so later planning does not depend on
// EnTT storage order or change between otherwise identical frames.
TEST(AISystem, AgentDiscoveryUsesDeterministicEntityHandleOrder)
{
    GameplayWorld world{};

    const EntityHandle first = world.CreateEntity();
    const EntityHandle second = world.CreateEntity();
    const EntityHandle third = world.CreateEntity();

    world.AddAI(third);
    world.AddAI(first);
    world.AddAI(second);

    std::vector<EntityHandle> firstDiscovery{};
    std::vector<EntityHandle> secondDiscovery{};
    world.CollectAIEntities(firstDiscovery);
    world.CollectAIEntities(secondDiscovery);

    const std::vector<EntityHandle> expectedOrder{ first, second, third };
    EXPECT_EQ(firstDiscovery, expectedOrder);
    EXPECT_EQ(secondDiscovery, expectedOrder);
    EXPECT_TRUE(std::is_sorted(firstDiscovery.begin(), firstDiscovery.end()));
}

// Protects the initial no-op update contract so empty AI agents can be
// stepped repeatedly without mutating unrelated gameplay state.
TEST(AISystem, EmptyAgentsUpdateWithoutGameplaySideEffects)
{
    GameplayWorld world{};
    AISystem aiSystem{};

    EXPECT_EQ(aiSystem.Update(world), 0u);

    const EntityHandle aiEntity = world.CreateEntity();
    const EntityHandle nonAIEntity = world.CreateEntity();
    world.AddAI(aiEntity);
    world.AddTransform(aiEntity, GameplayTransformComponent{ .position = { 4.0f, 5.0f, 6.0f } });

    EXPECT_EQ(aiSystem.Update(world), 1u);
    EXPECT_EQ(aiSystem.Update(world), 1u);
    EXPECT_EQ(world.GetAliveCount(), 2u);
    EXPECT_TRUE(world.IsEntityValid(aiEntity));
    EXPECT_TRUE(world.IsEntityValid(nonAIEntity));
    EXPECT_TRUE(world.HasAI(aiEntity));
    EXPECT_FALSE(world.HasAI(nonAIEntity));

    ASSERT_NE(world.TryGetTransform(aiEntity), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.x, 4.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.y, 5.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.z, 6.0f);
}

// Protects the generic submission boundary: AISystem must store and start a
// valid runtime without knowing any concrete gameplay-action type.
TEST(AISystem, StartActionStoresAndStartsValidTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState runtimeState{};

    EXPECT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(runtimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_TRUE(runtimeState.bStartCalled);
    EXPECT_EQ(runtimeState.lastContext, MakeActionContext(agent));
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
    EXPECT_TRUE(aiSystem.HasActiveAction(agent));
}

// Protects replacement semantics at the generic task-owner layer rather than
// inside specialized action starters.
TEST(AISystem, StartActionReplacesExistingRunningTaskForSameAgent)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState firstRuntimeState{};
    FakeActionRuntimeState secondRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(firstRuntimeState)),
        AIActionExecutionStatus::Running);
    EXPECT_EQ(
        aiSystem.StartAction(
            world,
            MakeActionContext(agent, kReplacementActionId),
            MakeFakeRuntime(secondRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_TRUE(firstRuntimeState.bCancelCalled);
    EXPECT_TRUE(secondRuntimeState.bStartCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects validation ordering so an invalid generic context cannot cancel or
// replace the current task.
TEST(AISystem, StartActionRejectsInvalidContextWithoutReplacingCurrentTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState activeRuntimeState{};
    FakeActionRuntimeState rejectedRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(activeRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        aiSystem.StartAction(world, AIActionRuntimeContext{}, MakeFakeRuntime(rejectedRuntimeState)),
        AIActionExecutionStatus::Failed);

    EXPECT_FALSE(activeRuntimeState.bCancelCalled);
    EXPECT_FALSE(rejectedRuntimeState.bStartCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects validation ordering so a missing runtime cannot cancel or replace
// the current task.
TEST(AISystem, StartActionRejectsNullRuntimeWithoutReplacingCurrentTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState activeRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(activeRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent, kReplacementActionId), nullptr),
        AIActionExecutionStatus::Failed);

    EXPECT_FALSE(activeRuntimeState.bCancelCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
}

// Protects the one-action-per-agent invariant while allowing unrelated agents
// to keep independent active tasks.
TEST(AISystem, StartActionPreservesOneActiveTaskPerAgent)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle firstAgent = world.CreateEntity();
    const EntityHandle secondAgent = world.CreateEntity();
    world.AddAI(firstAgent);
    world.AddAI(secondAgent);
    FakeActionRuntimeState firstRuntimeState{};
    FakeActionRuntimeState secondRuntimeState{};
    FakeActionRuntimeState replacementRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(firstAgent), MakeFakeRuntime(firstRuntimeState)),
        AIActionExecutionStatus::Running);
    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(secondAgent), MakeFakeRuntime(secondRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        aiSystem.StartAction(
            world,
            MakeActionContext(firstAgent, kReplacementActionId),
            MakeFakeRuntime(replacementRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_TRUE(firstRuntimeState.bCancelCalled);
    EXPECT_FALSE(secondRuntimeState.bCancelCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(firstAgent), AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.GetActionStatus(secondAgent), AIActionExecutionStatus::Running);
}

// Protects validation ordering so a context for a destroyed entity cannot
// cancel or replace the current task.
TEST(AISystem, StartActionRejectsInvalidEntityWithoutReplacingCurrentTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    const EntityHandle destroyedAgent = world.CreateEntity();
    world.AddAI(agent);
    world.AddAI(destroyedAgent);
    world.DestroyEntity(destroyedAgent);
    FakeActionRuntimeState activeRuntimeState{};
    FakeActionRuntimeState rejectedRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(activeRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        aiSystem.StartAction(world, MakeActionContext(destroyedAgent), MakeFakeRuntime(rejectedRuntimeState)),
        AIActionExecutionStatus::Failed);

    EXPECT_FALSE(activeRuntimeState.bCancelCalled);
    EXPECT_FALSE(rejectedRuntimeState.bStartCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.GetActionStatus(destroyedAgent), AIActionExecutionStatus::NotStarted);
}

// Protects validation ordering so a non-AI entity cannot cancel or replace the
// current task even when it has a valid entity handle.
TEST(AISystem, StartActionRejectsNonAIAgentWithoutReplacingCurrentTask)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    const EntityHandle nonAIAgent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState activeRuntimeState{};
    FakeActionRuntimeState rejectedRuntimeState{};

    ASSERT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(activeRuntimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_EQ(
        aiSystem.StartAction(world, MakeActionContext(nonAIAgent), MakeFakeRuntime(rejectedRuntimeState)),
        AIActionExecutionStatus::Failed);

    EXPECT_FALSE(activeRuntimeState.bCancelCalled);
    EXPECT_FALSE(rejectedRuntimeState.bStartCalled);
    EXPECT_EQ(aiSystem.GetActionStatus(agent), AIActionExecutionStatus::Running);
    EXPECT_EQ(aiSystem.GetActionStatus(nonAIAgent), AIActionExecutionStatus::NotStarted);
}

// Protects the generic submission boundary from acquiring movement-specific
// requirements while preserving AISystem ownership of AI-agent tasks.
TEST(AISystem, StartActionDoesNotRequireMovementSpecificState)
{
    GameplayWorld world{};
    AISystem aiSystem{};
    const EntityHandle agent = world.CreateEntity();
    world.AddAI(agent);
    FakeActionRuntimeState runtimeState{};

    EXPECT_EQ(
        aiSystem.StartAction(world, MakeActionContext(agent), MakeFakeRuntime(runtimeState)),
        AIActionExecutionStatus::Running);

    EXPECT_TRUE(runtimeState.bStartCalled);
    EXPECT_FALSE(world.HasTransform(agent));
    EXPECT_FALSE(world.HasCharacterCommand(agent));
    EXPECT_FALSE(world.HasCharacterMotor(agent));
    EXPECT_FALSE(world.HasCharacterMovementState(agent));
}