#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

import core;

using namespace rendern;

namespace
{
    AIPlan Plan(std::initializer_list<std::uint16_t> actionIds)
    {
        AIPlan plan{ AIGoalId{ 1 }, {} };
        for (const std::uint16_t id : actionIds)
        {
            plan.steps.push_back({ AIActionId{ id } });
        }
        return plan;
    }
}

TEST(AIPlanExecution, ConstructionOwnsPlanAndRejectsMalformedIds)
{
    AIPlan source = Plan({ 10, 20 });
    AIPlanExecution execution(std::move(source));
    EXPECT_EQ(execution.GetStatus(), AIPlanExecutionStatus::NotStarted);
    ASSERT_EQ(execution.GetPlan().steps.size(), 2u);
    EXPECT_EQ(execution.GetPlan().steps[0].actionId, AIActionId{ 10 });
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_FALSE(execution.IsRunningStep());

    EXPECT_EQ(AIPlanExecution(AIPlan{}).GetStatus(), AIPlanExecutionStatus::Failed);
    AIPlan invalidAction = Plan({ 10 });
    invalidAction.steps.push_back({});
    EXPECT_EQ(AIPlanExecution(std::move(invalidAction)).GetStatus(), AIPlanExecutionStatus::Failed);
}

TEST(AIPlanExecution, EmptyPlanStartsSucceededAndCannotRestart)
{
    AIPlanExecution execution(Plan({}));
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::Succeeded);
    EXPECT_TRUE(execution.IsTerminal());
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::Succeeded);
}

TEST(AIPlanExecution, StartAndMarkStartedExposeFirstStepWithoutAdvancing)
{
    AIPlanExecution execution(Plan({ 10, 20 }));
    EXPECT_EQ(execution.MarkCurrentStepStarted(), AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    ASSERT_NE(execution.GetCurrentStep(), nullptr);
    EXPECT_EQ(execution.GetCurrentStep()->actionId, AIActionId{ 10 });
    EXPECT_TRUE(execution.IsReadyToStartStep());
    EXPECT_EQ(execution.MarkCurrentStepStarted(), AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(execution.GetCurrentStep()->actionId, AIActionId{ 10 });
}

TEST(AIPlanExecution, RunningAndNotStartedActionStatusesDoNotAdvance)
{
    AIPlanExecution execution(Plan({ 10 }));
    execution.Start();
    execution.MarkCurrentStepStarted();
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Running),
        AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::NotStarted),
        AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
}

TEST(AIPlanExecution, StepStartFailureIsTerminalWithoutAdvancing)
{
    AIPlanExecution execution(Plan({ 10, 20 }));
    EXPECT_EQ(execution.MarkCurrentStepStartFailed(), AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(execution.MarkCurrentStepStartFailed(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
    EXPECT_TRUE(execution.IsTerminal());
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(execution.MarkCurrentStepStartFailed(), AIPlanExecutionStatus::Failed);
}

TEST(AIPlanExecution, StepStartFailureDoesNotOverrideRunningStepLifecycle)
{
    AIPlanExecution execution(Plan({ 10 }));
    execution.Start();
    execution.MarkCurrentStepStarted();
    EXPECT_EQ(execution.MarkCurrentStepStartFailed(), AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::Succeeded);
}

TEST(AIPlanExecution, SuccessAdvancesOneStepAndFinalSuccessIsStable)
{
    AIPlanExecution execution(Plan({ 10, 20 }));
    execution.Start();
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
    execution.MarkCurrentStepStarted();
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::ReadyToStartStep);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 1u);
    EXPECT_FALSE(execution.IsRunningStep());
    ASSERT_NE(execution.GetCurrentStep(), nullptr);
    EXPECT_EQ(execution.GetCurrentStep()->actionId, AIActionId{ 20 });
    execution.MarkCurrentStepStarted();
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::Succeeded);
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::Succeeded);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 2u);
}

TEST(AIPlanExecution, FailureDoesNotAdvanceAndTerminalStateIsStable)
{
    AIPlanExecution execution(Plan({ 10, 20 }));
    execution.Start();
    execution.MarkCurrentStepStarted();
    EXPECT_EQ(execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Failed),
        AIPlanExecutionStatus::Failed);
    EXPECT_EQ(execution.GetCurrentStepIndex(), 0u);
    EXPECT_FALSE(execution.HasCurrentStep());
    EXPECT_EQ(execution.Start(), AIPlanExecutionStatus::Failed);
    execution.Cancel();
    EXPECT_EQ(execution.GetStatus(), AIPlanExecutionStatus::Failed);
}

TEST(AIPlanExecution, ExplicitAndActionCancellationRemainDistinctAndStable)
{
    AIPlanExecution explicitCancellation(Plan({ 10 }));
    explicitCancellation.Cancel();
    EXPECT_EQ(explicitCancellation.GetStatus(), AIPlanExecutionStatus::Cancelled);
    EXPECT_NE(explicitCancellation.GetStatus(), AIPlanExecutionStatus::Failed);
    EXPECT_EQ(explicitCancellation.Start(), AIPlanExecutionStatus::Cancelled);

    AIPlanExecution actionCancellation(Plan({ 10 }));
    actionCancellation.Start();
    actionCancellation.MarkCurrentStepStarted();
    EXPECT_EQ(actionCancellation.ApplyCurrentStepStatus(AIActionExecutionStatus::Cancelled),
        AIPlanExecutionStatus::Cancelled);
    EXPECT_EQ(actionCancellation.GetCurrentStepIndex(), 0u);
    EXPECT_EQ(actionCancellation.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded),
        AIPlanExecutionStatus::Cancelled);
}

TEST(AIPlanExecution, ThreeStepsAreExposedInPlanOrder)
{
    AIPlanExecution execution(Plan({ 30, 10, 20 }));
    execution.Start();
    for (const std::uint16_t expected : { 30, 10, 20 })
    {
        ASSERT_NE(execution.GetCurrentStep(), nullptr);
        EXPECT_EQ(execution.GetCurrentStep()->actionId, AIActionId{ expected });
        execution.MarkCurrentStepStarted();
        execution.ApplyCurrentStepStatus(AIActionExecutionStatus::Succeeded);
    }
    EXPECT_EQ(execution.GetStatus(), AIPlanExecutionStatus::Succeeded);
}