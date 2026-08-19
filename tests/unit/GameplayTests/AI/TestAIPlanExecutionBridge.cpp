#include <gtest/gtest.h>

#include <memory>
#include <utility>

import core;

using namespace rendern;

namespace
{
    constexpr AIActionId kAction{20u};
    constexpr AIActionId kOtherAction{21u};

    struct RuntimeState
    {
        AIActionRuntimeResult start{AIActionRuntimeResult::Running};
        AIActionRuntimeResult tick{AIActionRuntimeResult::Running};
        AIActionRuntimeContext context{};
        bool started{};
        bool cancelled{};
    };

    class Runtime final : public IAIActionRuntime
    {
    public:
        explicit Runtime(RuntimeState& state) : state_(&state) {}
        AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            state_->started = true;
            state_->context = context;
            return state_->start;
        }
        AIActionRuntimeResult Tick(const AIActionRuntimeContext&, float) override
        {
            return state_->tick;
        }
        void Cancel(const AIActionRuntimeContext&) noexcept override { state_->cancelled = true; }
    private:
        RuntimeState* state_{};
    };

    class Binding final : public IAIActionBinding
    {
    public:
        explicit Binding(RuntimeState& runtimeState, const bool shouldFailCreation = false)
            : state(&runtimeState), failCreation(shouldFailCreation) {}
        RuntimeState* state{};
        bool failCreation{};
        AIActionRuntimeContext received{};
        int calls{};
        std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext& context) override
        {
            received = context;
            ++calls;
            if (failCreation)
            {
                return nullptr;
            }
            return std::make_unique<Runtime>(*state);
        }
    };

    AIPlanExecution Plan(std::initializer_list<AIActionId> actions)
    {
        AIPlan plan{.goalId = AIGoalId{1u}};
        for (AIActionId action : actions) plan.steps.push_back({action});
        return AIPlanExecution(std::move(plan));
    }

    EntityHandle AddAgent(GameplayWorld& world)
    {
        EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        return entity;
    }

    void Start(AIPlanExecution& plan, AIActionBindingRegistry& registry, AISystem& system,
        GameplayWorld& world, EntityHandle agent)
    {
        ASSERT_EQ(plan.Start(), AIPlanExecutionStatus::ReadyToStartStep);
        AIPlanExecutionBridge::StartReadyPlanStep(plan, registry, system, world, agent);
    }
}

TEST(AIPlanExecutionBridge, StartsRunningRuntimeWithExactSemanticContext)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    RuntimeState state{}; Binding binding(state); AIActionBindingRegistry registry{};
    ASSERT_TRUE(registry.Register(kAction, binding)); auto plan = Plan({kAction});
    Start(plan, registry, system, world, agent);
    EXPECT_EQ(plan.GetStatus(), AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(binding.received, (AIActionRuntimeContext{agent, kAction}));
    EXPECT_EQ(state.context, binding.received);
}

TEST(AIPlanExecutionBridge, MissingBindingAndFactoryFailurePreserveExistingTask)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    RuntimeState existing{};
    ASSERT_EQ(system.StartAction(world, {agent, kOtherAction}, std::make_unique<Runtime>(existing)),
        AIActionExecutionStatus::Running);
    AIActionBindingRegistry registry{}; auto missing = Plan({kAction});
    Start(missing, registry, system, world, agent);
    EXPECT_EQ(missing.GetStatus(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(missing.GetCurrentStepIndex(), 0u);
    EXPECT_EQ(system.GetActionStatus(agent, kOtherAction), AIActionExecutionStatus::Running);

    RuntimeState unused{}; Binding binding(unused, true);
    ASSERT_TRUE(registry.Register(kAction, binding)); auto failedFactory = Plan({kAction});
    Start(failedFactory, registry, system, world, agent);
    EXPECT_EQ(failedFactory.GetStatus(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(system.GetActionStatus(agent, kOtherAction), AIActionExecutionStatus::Running);
}

TEST(AIPlanExecutionBridge, MapsStartFailureAndSynchronousSuccess)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    AIActionBindingRegistry registry{}; RuntimeState failed{.start=AIActionRuntimeResult::Failed};
    Binding binding(failed); ASSERT_TRUE(registry.Register(kAction, binding));
    auto failedPlan = Plan({kAction}); Start(failedPlan, registry, system, world, agent);
    EXPECT_EQ(failedPlan.GetStatus(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(failedPlan.GetCurrentStepIndex(), 0u);

    RuntimeState succeeded{.start=AIActionRuntimeResult::Succeeded}; binding.state=&succeeded;
    auto one = Plan({kAction}); Start(one, registry, system, world, agent);
    EXPECT_EQ(one.GetStatus(), AIPlanExecutionStatus::Succeeded);
    auto two = Plan({kAction, kAction}); Start(two, registry, system, world, agent);
    EXPECT_EQ(two.GetStatus(), AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(two.GetCurrentStepIndex(), 1u);
    EXPECT_EQ(binding.calls, 3);
}

TEST(AIPlanExecutionBridge, SynchronizesRunningSuccessFailureAndCancellation)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    AIActionBindingRegistry registry{}; RuntimeState state{}; Binding binding(state);
    ASSERT_TRUE(registry.Register(kAction, binding)); auto plan = Plan({kAction, kAction});
    Start(plan, registry, system, world, agent);
    AIPlanExecutionBridge::SynchronizeRunningPlanStep(plan, system, agent);
    EXPECT_EQ(plan.GetStatus(), AIPlanExecutionStatus::RunningStep);
    state.tick=AIActionRuntimeResult::Succeeded; system.Update(world);
    AIPlanExecutionBridge::SynchronizeRunningPlanStep(plan, system, agent);
    EXPECT_EQ(plan.GetCurrentStepIndex(), 1u);

    state.tick=AIActionRuntimeResult::Running; AIPlanExecutionBridge::StartReadyPlanStep(plan, registry, system, world, agent);
    state.tick=AIActionRuntimeResult::Failed; system.Update(world);
    AIPlanExecutionBridge::SynchronizeRunningPlanStep(plan, system, agent);
    EXPECT_EQ(plan.GetStatus(), AIPlanExecutionStatus::Failed);

    RuntimeState cancelled{}; binding.state=&cancelled; auto cancelPlan=Plan({kAction});
    Start(cancelPlan, registry, system, world, agent); system.CancelAction(agent);
    AIPlanExecutionBridge::SynchronizeRunningPlanStep(cancelPlan, system, agent);
    EXPECT_EQ(cancelPlan.GetStatus(), AIPlanExecutionStatus::Cancelled);
}

TEST(AIPlanExecutionBridge, LostOrDifferentActionTaskFailsRunningPlan)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    AIActionBindingRegistry registry{}; RuntimeState expected{}; Binding binding(expected);
    ASSERT_TRUE(registry.Register(kAction, binding)); auto plan=Plan({kAction}); Start(plan, registry, system, world, agent);
    system.ClearAction(agent); AIPlanExecutionBridge::SynchronizeRunningPlanStep(plan, system, agent);
    EXPECT_EQ(plan.GetStatus(), AIPlanExecutionStatus::Failed);

    auto replaced=Plan({kAction}); Start(replaced, registry, system, world, agent); RuntimeState other{};
    system.StartAction(world, {agent, kOtherAction}, std::make_unique<Runtime>(other));
    AIPlanExecutionBridge::SynchronizeRunningPlanStep(replaced, system, agent);
    EXPECT_EQ(replaced.GetStatus(), AIPlanExecutionStatus::Failed);
}

TEST(AIPlanExecutionBridge, CancellationTouchesOnlyMatchingRunningRuntime)
{
    GameplayWorld world{}; const EntityHandle agent = AddAgent(world); AISystem system{};
    AIActionBindingRegistry registry{}; RuntimeState unrelated{};
    system.StartAction(world, {agent, kOtherAction}, std::make_unique<Runtime>(unrelated));
    auto ready=Plan({kAction}); ASSERT_EQ(ready.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    AIPlanExecutionBridge::CancelPlanExecution(ready, system, agent);
    EXPECT_EQ(ready.GetStatus(), AIPlanExecutionStatus::Cancelled); EXPECT_FALSE(unrelated.cancelled);

    RuntimeState matching{}; Binding binding(matching); ASSERT_TRUE(registry.Register(kAction,binding));
    auto running=Plan({kAction}); Start(running,registry,system,world,agent);
    AIPlanExecutionBridge::CancelPlanExecution(running,system,agent);
    EXPECT_TRUE(matching.cancelled); EXPECT_EQ(running.GetStatus(),AIPlanExecutionStatus::Cancelled);

    auto replacement=Plan({kAction}); Start(replacement,registry,system,world,agent); RuntimeState other{};
    system.StartAction(world,{agent,kOtherAction},std::make_unique<Runtime>(other));
    AIPlanExecutionBridge::CancelPlanExecution(replacement,system,agent);
    EXPECT_FALSE(other.cancelled); EXPECT_EQ(replacement.GetStatus(),AIPlanExecutionStatus::Cancelled);
    AIPlanExecutionBridge::CancelPlanExecution(replacement,system,agent);
    EXPECT_FALSE(other.cancelled);
}