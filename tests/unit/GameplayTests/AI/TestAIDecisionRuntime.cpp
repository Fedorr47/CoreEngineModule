#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <vector>

import core;

using namespace rendern;

namespace
{
    constexpr AIWorldFactId kFact0{0u};
    constexpr AIWorldFactId kGoalFact{1u};
    constexpr AIWorldFactId kPreferredGoalFact{2u};
    constexpr AIWorldFactId kScoreFact{3u};
    constexpr AIActionId kActionA{1u};
    constexpr AIActionId kActionB{2u};

    struct RuntimeState
    {
        AIActionRuntimeResult start{AIActionRuntimeResult::Running};
        AIActionRuntimeResult tick{AIActionRuntimeResult::Running};
        int starts{};
        int cancels{};
    };

    class Runtime final : public IAIActionRuntime
    {
    public:
        explicit Runtime(RuntimeState& state) : state_(&state) {}
        AIActionRuntimeResult Start(const AIActionRuntimeContext&) override
        {
            ++state_->starts;
            return state_->start;
        }
        AIActionRuntimeResult Tick(const AIActionRuntimeContext&, float) override { return state_->tick; }
        void Cancel(const AIActionRuntimeContext&) noexcept override { ++state_->cancels; }
    private:
        RuntimeState* state_;
    };

    class Binding final : public IAIActionBinding
    {
    public:
        explicit Binding(RuntimeState& state, bool fail = false) : state_(&state), fail_(fail) {}
        std::unique_ptr<IAIActionRuntime> CreateRuntime(const AIActionRuntimeContext&) override
        {
            return fail_ ? nullptr : std::make_unique<Runtime>(*state_);
        }
    private:
        RuntimeState* state_;
        bool fail_;
    };

    EntityHandle AddAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        return entity;
    }

    AIGoalDefinition Goal(AIGoalId id = AIGoalId{1u})
    {
        return AIGoalDefinition{.goalId=id, .desiredFacts={AIFactCondition{kGoalFact, true}}};
    }

    std::vector<AIActionDefinition> OneStep(AIActionId id = kActionA)
    {
        return {AIActionDefinition{.actionId=id, .effects={AIFactEffect{kGoalFact, true}}}};
    }

    std::vector<AIActionDefinition> TwoSteps()
    {
        return {
            AIActionDefinition{.actionId=kActionA,
                .preconditions={AIFactCondition{kFact0, false}},
                .effects={AIFactEffect{kFact0, true}}},
            AIActionDefinition{.actionId=kActionB,
                .preconditions={AIFactCondition{kFact0, true}},
                .effects={AIFactEffect{kGoalFact, true}}}
        };
    }
}

TEST(AIDecisionRuntime, PlanningSuccessAndAlreadySatisfiedGoal)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); AIDecisionRuntime runtime(agent); AIAgentWorldState observed{};
    const auto actions=OneStep(); ASSERT_TRUE(runtime.Plan(observed,Goal(),actions,system));
    EXPECT_EQ(runtime.GetStatus(),AIPlanExecutionStatus::ReadyToStartStep);
    ASSERT_NE(runtime.GetPlanExecution(),nullptr); EXPECT_EQ(runtime.GetPlanExecution()->GetPlan().steps.size(),1u);

    observed.SetFact(kGoalFact); AIDecisionRuntime satisfied(agent);
    ASSERT_TRUE(satisfied.Plan(observed,Goal(),actions,system));
    EXPECT_EQ(satisfied.GetStatus(),AIPlanExecutionStatus::Succeeded);
    EXPECT_EQ(satisfied.Update(bindings,system,world),AIPlanExecutionStatus::Succeeded);
    EXPECT_FALSE(system.HasActiveAction(agent));
}

TEST(AIDecisionRuntime, InvalidOrUnsatisfiablePlanningFailsWithoutStaleExecution)
{
    GameplayWorld world{}; AISystem system{}; const EntityHandle agent=AddAgent(world);
    AIDecisionRuntime runtime(agent); AIAgentWorldState observed{}; const std::vector<AIActionDefinition> none{};
    EXPECT_FALSE(runtime.Plan(observed,Goal(),none,system));
    EXPECT_EQ(runtime.GetStatus(),AIPlanExecutionStatus::Failed); EXPECT_EQ(runtime.GetPlanExecution(),nullptr);
    EXPECT_FALSE(runtime.Plan(observed,Goal(AIGoalId{}),none,system));
    EXPECT_FALSE(system.HasActiveAction(agent));
}

TEST(AIDecisionRuntime, ExecutesOneAndMultipleStepsAcrossBoundedUpdates)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); RuntimeState a{},b{}; Binding bindingA(a),bindingB(b);
    ASSERT_TRUE(bindings.Register(kActionA,bindingA)); ASSERT_TRUE(bindings.Register(kActionB,bindingB));
    AIAgentWorldState observed{}; AIDecisionRuntime runtime(agent); const auto actions=TwoSteps();
    ASSERT_TRUE(runtime.Plan(observed,Goal(),actions,system));
    EXPECT_EQ(runtime.Update(bindings,system,world),AIPlanExecutionStatus::RunningStep); EXPECT_EQ(a.starts,1);
    a.tick=AIActionRuntimeResult::Succeeded; system.Update(world);
    EXPECT_EQ(runtime.Update(bindings,system,world),AIPlanExecutionStatus::ReadyToStartStep); EXPECT_EQ(b.starts,0);
    EXPECT_FALSE(observed.IsFactSet(kFact0)); // Planner predictions did not mutate observation.
    EXPECT_EQ(runtime.Update(bindings,system,world),AIPlanExecutionStatus::RunningStep); EXPECT_EQ(b.starts,1);
    b.tick=AIActionRuntimeResult::Succeeded; system.Update(world);
    EXPECT_EQ(runtime.Update(bindings,system,world),AIPlanExecutionStatus::Succeeded);

    RuntimeState one{}; Binding oneBinding(one); AIActionBindingRegistry oneRegistry{};
    ASSERT_TRUE(oneRegistry.Register(kActionA,oneBinding)); const auto oneActions=OneStep();
    AIDecisionRuntime oneRuntime(agent); ASSERT_TRUE(oneRuntime.Plan(observed,Goal(),oneActions,system));
    one.tick=AIActionRuntimeResult::Succeeded; oneRuntime.Update(oneRegistry,system,world); system.Update(world);
    EXPECT_EQ(oneRuntime.Update(oneRegistry,system,world),AIPlanExecutionStatus::Succeeded);
}

TEST(AIDecisionRuntime, BindingCreationAndActionFailuresAreDeterministic)
{
    GameplayWorld world{}; AISystem system{}; const EntityHandle agent=AddAgent(world);
    AIAgentWorldState observed{}; const auto actions=OneStep(); AIActionBindingRegistry bindings{};
    AIDecisionRuntime missing(agent); ASSERT_TRUE(missing.Plan(observed,Goal(),actions,system));
    EXPECT_EQ(missing.Update(bindings,system,world),AIPlanExecutionStatus::Failed);

    RuntimeState unused{}; Binding failedFactory(unused,true); ASSERT_TRUE(bindings.Register(kActionA,failedFactory));
    AIDecisionRuntime factory(agent); ASSERT_TRUE(factory.Plan(observed,Goal(),actions,system));
    EXPECT_EQ(factory.Update(bindings,system,world),AIPlanExecutionStatus::Failed);

    bindings.Reset(); RuntimeState failed{}; Binding failedBinding(failed); ASSERT_TRUE(bindings.Register(kActionA,failedBinding));
    AIDecisionRuntime action(agent); ASSERT_TRUE(action.Plan(observed,Goal(),actions,system)); action.Update(bindings,system,world);
    failed.tick=AIActionRuntimeResult::Failed; system.Update(world);
    EXPECT_EQ(action.Update(bindings,system,world),AIPlanExecutionStatus::Failed);
}

TEST(AIDecisionRuntime, CancellationReplanAndAgentIsolation)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agentA=AddAgent(world),agentB=AddAgent(world); RuntimeState a{},replacement{};
    Binding bindingA(a),bindingReplacement(replacement);
    ASSERT_TRUE(bindings.Register(kActionA,bindingA)); ASSERT_TRUE(bindings.Register(kActionB,bindingReplacement));
    AIAgentWorldState observed{}; const auto actionA=OneStep(kActionA), actionB=OneStep(kActionB);
    AIDecisionRuntime runtimeA(agentA),runtimeB(agentB);
    ASSERT_TRUE(runtimeA.Plan(observed,Goal(),actionA,system)); ASSERT_TRUE(runtimeB.Plan(observed,Goal(),actionA,system));
    runtimeA.Update(bindings,system,world); runtimeB.Update(bindings,system,world);
    runtimeA.Cancel(system); runtimeA.Cancel(system);
    EXPECT_EQ(runtimeA.GetStatus(),AIPlanExecutionStatus::Cancelled); EXPECT_FALSE(system.HasActiveAction(agentA));
    EXPECT_TRUE(system.HasActiveAction(agentB)); EXPECT_EQ(a.cancels,1);
    EXPECT_EQ(runtimeA.Update(bindings,system,world),AIPlanExecutionStatus::Cancelled);

    ASSERT_TRUE(runtimeA.Plan(observed,Goal(),actionA,system)); runtimeA.Update(bindings,system,world);
    ASSERT_TRUE(runtimeA.Plan(observed,Goal(),actionB,system)); EXPECT_FALSE(system.HasActiveAction(agentA));
    EXPECT_EQ(runtimeA.GetStatus(),AIPlanExecutionStatus::ReadyToStartStep);
    runtimeA.Update(bindings,system,world); EXPECT_EQ(replacement.starts,1); EXPECT_TRUE(system.HasActiveAction(agentB));
}

TEST(AIDecisionRuntime, ObservedScoreFactChangeReselectsGoal)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); RuntimeState a{},b{}; Binding bindingA(a),bindingB(b);
    ASSERT_TRUE(bindings.Register(kActionA,bindingA)); ASSERT_TRUE(bindings.Register(kActionB,bindingB));
    const std::vector<AIGoalSelectionCandidate> candidates{
        AIGoalSelectionCandidate{.goal=Goal(AIGoalId{1u}), .baseScore=2.0f},
        AIGoalSelectionCandidate{
            .goal=AIGoalDefinition{.goalId=AIGoalId{2u},
                .desiredFacts={AIFactCondition{kPreferredGoalFact,true}}},
            .baseScore=1.0f,
            .scoreRules={AIGoalScoreRule{AIFactCondition{kScoreFact,true},3.0f}}}};
    const std::vector<AIActionDefinition> actions{
        AIActionDefinition{.actionId=kActionA, .effects={AIFactEffect{kGoalFact,true}}},
        AIActionDefinition{.actionId=kActionB, .effects={AIFactEffect{kPreferredGoalFact,true}}}};
    AIAgentWorldState observed{};
    AIDecisionRuntime runtime(agent);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(a.starts,1);

    observed.SetFact(kScoreFact);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(a.cancels,1); EXPECT_FALSE(system.HasActiveAction(agent));
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(b.starts,1);
}

TEST(AIDecisionRuntime, HigherScoringGoalReplacesCurrentPlan)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); RuntimeState current{},preferred{};
    Binding currentBinding(current),preferredBinding(preferred);
    ASSERT_TRUE(bindings.Register(kActionA,currentBinding)); ASSERT_TRUE(bindings.Register(kActionB,preferredBinding));
    const std::vector<AIGoalSelectionCandidate> candidates{
        AIGoalSelectionCandidate{
            .goal=AIGoalDefinition{.goalId=AIGoalId{2u},
                .desiredFacts={AIFactCondition{kPreferredGoalFact,true}}}, .baseScore=10.0f},
        AIGoalSelectionCandidate{.goal=Goal(AIGoalId{1u}), .baseScore=1.0f}};
    const std::vector<AIActionDefinition> actions{
        AIActionDefinition{.actionId=kActionA, .effects={AIFactEffect{kGoalFact,true}}},
        AIActionDefinition{.actionId=kActionB, .effects={AIFactEffect{kPreferredGoalFact,true}}}};
    AIAgentWorldState observed{}; observed.SetFact(kPreferredGoalFact); AIDecisionRuntime runtime(agent);
    runtime.Update(observed,candidates,actions,bindings,system,world);
    runtime.Update(observed,candidates,actions,bindings,system,world);
    observed.ClearFact(kPreferredGoalFact);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(current.cancels,1); EXPECT_EQ(preferred.starts,0);
}

TEST(AIDecisionRuntime, ActionFailureReplansFromLatestObservedStateInSameDecisionUpdate)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); RuntimeState first{},replacement{};
    Binding firstBinding(first),replacementBinding(replacement);
    ASSERT_TRUE(bindings.Register(kActionA,firstBinding)); ASSERT_TRUE(bindings.Register(kActionB,replacementBinding));
    const std::vector<AIGoalSelectionCandidate> candidates{
        AIGoalSelectionCandidate{.goal=Goal(), .baseScore=1.0f}};
    const std::vector<AIActionDefinition> actions{
        AIActionDefinition{.actionId=kActionA,
            .preconditions={AIFactCondition{kFact0,false}}, .effects={AIFactEffect{kGoalFact,true}}},
        AIActionDefinition{.actionId=kActionB,
            .preconditions={AIFactCondition{kFact0,true}}, .effects={AIFactEffect{kGoalFact,true}}}};
    AIAgentWorldState observed{}; AIDecisionRuntime runtime(agent);
    runtime.Update(observed,candidates,actions,bindings,system,world);
    runtime.Update(observed,candidates,actions,bindings,system,world);
    first.tick=AIActionRuntimeResult::Failed; system.Update(world); observed.SetFact(kFact0);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_FALSE(observed.IsFactSet(kGoalFact));
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(replacement.starts,1);
}

TEST(AIDecisionRuntime, FailedReplacementPlanPreservesCurrentValidExecution)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); RuntimeState current{}; Binding currentBinding(current);
    ASSERT_TRUE(bindings.Register(kActionA,currentBinding));
    const std::vector<AIGoalSelectionCandidate> candidates{
        AIGoalSelectionCandidate{
            .goal=AIGoalDefinition{.goalId=AIGoalId{2u},
                .desiredFacts={AIFactCondition{kPreferredGoalFact,true}}}, .baseScore=10.0f},
        AIGoalSelectionCandidate{.goal=Goal(AIGoalId{1u}), .baseScore=1.0f}};
    const std::vector<AIActionDefinition> actions{
        AIActionDefinition{.actionId=kActionA, .effects={AIFactEffect{kGoalFact,true}}}};
    AIAgentWorldState observed{}; observed.SetFact(kPreferredGoalFact); AIDecisionRuntime runtime(agent);
    runtime.Update(observed,candidates,actions,bindings,system,world);
    ASSERT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::RunningStep);
    observed.ClearFact(kPreferredGoalFact);
    EXPECT_EQ(runtime.Update(observed,candidates,actions,bindings,system,world),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(current.cancels,0); EXPECT_TRUE(system.HasActiveAction(agent));
    ASSERT_NE(runtime.GetPlanExecution(),nullptr);
    EXPECT_EQ(runtime.GetPlanExecution()->GetPlan().goalId,AIGoalId{1u});
}

TEST(AIDecisionRuntime, SatisfiedOrUnplannableGoalsDoNotStartActions)
{
    GameplayWorld world{}; AISystem system{}; AIActionBindingRegistry bindings{};
    const EntityHandle agent=AddAgent(world); AIDecisionRuntime runtime(agent);
    AIAgentWorldState observed{}; observed.SetFact(kGoalFact);
    const std::vector<AIGoalSelectionCandidate> candidates{
        AIGoalSelectionCandidate{.goal=Goal(), .baseScore=1.0f}};
    const std::vector<AIActionDefinition> noActions{};
    EXPECT_EQ(runtime.Update(observed,candidates,noActions,bindings,system,world),AIPlanExecutionStatus::Succeeded);
    EXPECT_FALSE(system.HasActiveAction(agent));
    observed.ClearFact(kGoalFact);
    EXPECT_EQ(runtime.Update(observed,candidates,noActions,bindings,system,world),AIPlanExecutionStatus::Failed);
    EXPECT_FALSE(system.HasActiveAction(agent));
}