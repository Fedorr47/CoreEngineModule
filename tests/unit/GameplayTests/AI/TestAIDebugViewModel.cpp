#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <span>
#include <vector>

import core;

using namespace rendern;

namespace
{
    constexpr AIWorldFactId kFirstBoolean{1u};
    constexpr AIWorldFactId kSecondBoolean{2u};
    constexpr AIWorldIntegerFactId kFirstInteger{3u};
    constexpr AIWorldIntegerFactId kSecondInteger{4u};
    constexpr AIActionId kActionA{10u};
    constexpr AIActionId kActionB{11u};
    constexpr AIActionContextId kContextA{20u};
    constexpr AIActionContextId kContextB{21u};

    AIDebugViewModel Snapshot(
        const AIAgentWorldState& state,
        const std::span<const AIActionDefinition> actions = {},
        const AIPlanExecution* execution = nullptr)
    {
        constexpr std::array booleanFacts{kFirstBoolean, kSecondBoolean};
        constexpr std::array integerFacts{kFirstInteger, kSecondInteger};
        return BuildAIDebugViewModel(
            state, booleanFacts, integerFacts, actions, execution);
    }
}

TEST(AIDebugViewModel, CapturesFalseAndZeroFactsWithoutMutatingObservedState)
{
    AIAgentWorldState state{};
    state.SetIntegerFact(kFirstInteger, 2);
    const AIAgentWorldState before = state;

    const AIDebugViewModel snapshot = Snapshot(state);

    ASSERT_EQ(snapshot.booleanFacts.size(), 2u);
    EXPECT_EQ(snapshot.booleanFacts[0].factId, kFirstBoolean);
    EXPECT_FALSE(snapshot.booleanFacts[0].value);
    ASSERT_EQ(snapshot.integerFacts.size(), 2u);
    EXPECT_EQ(snapshot.integerFacts[0].value, 2);
    EXPECT_EQ(snapshot.integerFacts[1].value, 0);
    EXPECT_EQ(state, before);
}

TEST(AIDebugViewModel, CapturesSelectedPlanCostsAndCurrentExecutionStep)
{
    AIPlanExecution execution(AIPlan{
        .goalId = AIGoalId{7u},
        .steps = {
            AIPlanStep{kActionA, kContextA},
            AIPlanStep{kActionB, kContextB}}});
    ASSERT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    execution.MarkCurrentStepStarted();
    execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded);

    const std::vector actions{
        AIActionDefinition{.actionId = kActionA, .contextId = kContextA,
            .baseCost = 2.0f},
        AIActionDefinition{.actionId = kActionB, .contextId = kContextB,
            .baseCost = 3.0f}};
    const AIDebugViewModel snapshot = Snapshot({}, actions, &execution);

    ASSERT_EQ(snapshot.selectedGoalId, AIGoalId{7u});
    ASSERT_EQ(snapshot.selectedPlan.size(), 2u);
    EXPECT_EQ(snapshot.selectedPlan[0].actionId, kActionA);
    EXPECT_EQ(snapshot.selectedPlan[0].contextId, kContextA);
    EXPECT_FLOAT_EQ(snapshot.selectedPlan[0].cost, 2.0f);
    EXPECT_TRUE(snapshot.selectedPlan[0].bCostResolved);
    EXPECT_EQ(snapshot.selectedPlan[1].actionId, kActionB);
    EXPECT_EQ(snapshot.selectedPlan[1].contextId, kContextB);
    EXPECT_FLOAT_EQ(snapshot.totalPlanCost, 5.0f);
    EXPECT_TRUE(snapshot.bPlanCostComplete);
    ASSERT_TRUE(snapshot.currentStepIndex.has_value());
    EXPECT_EQ(*snapshot.currentStepIndex, 1u);
    ASSERT_TRUE(snapshot.executionStatus.has_value());
    EXPECT_EQ(*snapshot.executionStatus, AIPlanExecutionStatus::ReadyToStartStep);
}

TEST(AIDebugViewModel, ReportsBooleanAndNumericFailuresInDefinitionOrder)
{
    AIAgentWorldState state{};
    state.SetIntegerFact(kFirstInteger, 2);
    const std::vector actions{
        AIActionDefinition{
            .actionId = kActionA,
            .preconditions = {
                AIFactCondition{kFirstBoolean, true},
                AIFactCondition{kSecondBoolean, true}},
            .contextId = kContextA,
            .numericPreconditions = {
                AINumericCondition{kFirstInteger,
                    AINumericConditionOperator::GreaterOrEqual, 3},
                AINumericCondition{kSecondInteger,
                    AINumericConditionOperator::NotEqual, 0}}}};

    const AIDebugViewModel snapshot = Snapshot(state, actions);

    ASSERT_EQ(snapshot.actionApplicability.size(), 1u);
    const AIDebugActionApplicabilityView& action = snapshot.actionApplicability[0];
    EXPECT_EQ(action.actionId, kActionA);
    EXPECT_EQ(action.contextId, kContextA);
    EXPECT_FALSE(action.applicable);
    ASSERT_EQ(action.failedBooleanConditions.size(), 2u);
    EXPECT_EQ(action.failedBooleanConditions[0].factId, kFirstBoolean);
    EXPECT_TRUE(action.failedBooleanConditions[0].expected);
    EXPECT_FALSE(action.failedBooleanConditions[0].actual);
    EXPECT_EQ(action.failedBooleanConditions[1].factId, kSecondBoolean);
    ASSERT_EQ(action.failedNumericConditions.size(), 2u);
    EXPECT_EQ(action.failedNumericConditions[0].factId, kFirstInteger);
    EXPECT_EQ(action.failedNumericConditions[0].comparison,
        AINumericConditionOperator::GreaterOrEqual);
    EXPECT_EQ(action.failedNumericConditions[0].expected, 3);
    EXPECT_EQ(action.failedNumericConditions[0].actual, 2);
    EXPECT_EQ(action.failedNumericConditions[1].factId, kSecondInteger);
}

TEST(AIDebugViewModel, NumericBoundarySuccessHasNoFailures)
{
    AIAgentWorldState state{};
    state.SetIntegerFact(kFirstInteger, 3);
    const std::vector actions{AIActionDefinition{
        .actionId = kActionA,
        .numericPreconditions = {AINumericCondition{kFirstInteger,
            AINumericConditionOperator::GreaterOrEqual, 3}}}};

    const AIDebugViewModel snapshot = Snapshot(state, actions);

    ASSERT_EQ(snapshot.actionApplicability.size(), 1u);
    EXPECT_TRUE(snapshot.actionApplicability[0].applicable);
    EXPECT_TRUE(snapshot.actionApplicability[0].failedBooleanConditions.empty());
    EXPECT_TRUE(snapshot.actionApplicability[0].failedNumericConditions.empty());
}

TEST(AIDebugViewModel, MissingContextDefinitionMarksPlanCostIncomplete)
{
    AIPlanExecution execution(AIPlan{.goalId = AIGoalId{8u},
        .steps = {AIPlanStep{kActionA, kContextB}}});
    const std::vector actions{AIActionDefinition{
        .actionId = kActionA, .contextId = kContextA, .baseCost = 2.0f}};

    const AIDebugViewModel snapshot = Snapshot({}, actions, &execution);

    ASSERT_EQ(snapshot.selectedPlan.size(), 1u);
    EXPECT_FALSE(snapshot.selectedPlan[0].bCostResolved);
    EXPECT_FALSE(snapshot.bPlanCostComplete);
    EXPECT_FLOAT_EQ(snapshot.totalPlanCost, 0.0f);
}