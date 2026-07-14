#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <vector>

import core;

using namespace rendern;

namespace
{
    static_assert(!std::is_copy_constructible_v<AIActionTask>);
    static_assert(!std::is_copy_assignable_v<AIActionTask>);
    static_assert(std::is_move_constructible_v<AIActionTask>);
    static_assert(std::is_move_assignable_v<AIActionTask>);

    class RecordingTaskRuntime final : public IAIActionRuntime
    {
    public:
        explicit RecordingTaskRuntime(bool* destroyedFlag = nullptr) noexcept
            : destroyedFlag_{ destroyedFlag }
        {
        }

        ~RecordingTaskRuntime() override
        {
            if (destroyedFlag_ != nullptr)
            {
                *destroyedFlag_ = true;
            }
        }

        AIActionRuntimeResult startResult{ AIActionRuntimeResult::Running };
        AIActionRuntimeResult tickResult{ AIActionRuntimeResult::Running };
        std::vector<AIActionRuntimeContext> startContexts{};
        std::vector<AIActionRuntimeContext> tickContexts{};
        std::vector<AIActionRuntimeContext> cancelContexts{};
        std::vector<float> tickDeltaSeconds{};

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            startContexts.push_back(context);
            return startResult;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            tickContexts.push_back(context);
            tickDeltaSeconds.push_back(deltaSeconds);
            return tickResult;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            cancelContexts.push_back(context);
        }

        [[nodiscard]] int StartCallCount() const noexcept
        {
            return static_cast<int>(startContexts.size());
        }

        [[nodiscard]] int TickCallCount() const noexcept
        {
            return static_cast<int>(tickContexts.size());
        }

        [[nodiscard]] int CancelCallCount() const noexcept
        {
            return static_cast<int>(cancelContexts.size());
        }

    private:
        bool* destroyedFlag_{ nullptr };
    };

    [[nodiscard]] constexpr AIActionRuntimeContext MakeContext() noexcept
    {
        return AIActionRuntimeContext{
            .agentEntity = 100u,
            .actionId = AIActionId{ 10u }
        };
    }

    [[nodiscard]] AIActionTask MakeTask(RecordingTaskRuntime*& runtimeView)
    {
        auto runtime = std::make_unique<RecordingTaskRuntime>();
        runtimeView = runtime.get();
        return AIActionTask{ MakeContext(), std::move(runtime) };
    }
}

// Protects the task construction boundary so creating a task cannot accidentally
// begin runtime execution before a caller explicitly starts it.
TEST(AIActionTask, NewTaskStartsNotStartedWithoutCallingRuntime)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::NotStarted);
    EXPECT_FALSE(task.IsRunning());
    EXPECT_FALSE(task.IsTerminal());
    EXPECT_EQ(task.GetContext(), MakeContext());
    EXPECT_EQ(runtime->StartCallCount(), 0);
    EXPECT_EQ(runtime->TickCallCount(), 0);
    EXPECT_EQ(runtime->CancelCallCount(), 0);
}

// Protects the only valid start transition and context forwarding, preventing
// future callers from starting a runtime without moving the task to Running.
TEST(AIActionTask, StartTransitionsTaskToRunning)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);

    const AIActionExecutionStatus status = task.Start();

    EXPECT_EQ(status, AIActionExecutionStatus::Running);
    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Running);
    EXPECT_TRUE(task.IsRunning());
    EXPECT_FALSE(task.IsTerminal());
    ASSERT_EQ(runtime->StartCallCount(), 1);
    EXPECT_EQ(runtime->startContexts.front(), MakeContext());
}

// Protects immediate runtime success so tasks can complete during Start without
// requiring a synthetic Tick that could duplicate effects later.
TEST(AIActionTask, StartCanCompleteImmediately)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    runtime->startResult = AIActionRuntimeResult::Succeeded;

    EXPECT_EQ(task.Start(), AIActionExecutionStatus::Succeeded);

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Succeeded);
    EXPECT_FALSE(task.IsRunning());
    EXPECT_TRUE(task.IsTerminal());
    EXPECT_EQ(runtime->StartCallCount(), 1);
}

// Protects immediate runtime failure so tasks can distinguish natural failure
// from an action that has never been started.
TEST(AIActionTask, StartCanFailImmediately)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    runtime->startResult = AIActionRuntimeResult::Failed;

    EXPECT_EQ(task.Start(), AIActionExecutionStatus::Failed);

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Failed);
    EXPECT_TRUE(task.IsTerminal());
    EXPECT_EQ(runtime->StartCallCount(), 1);
}

// Protects tick legality, context forwarding, and delta forwarding while
// preventing a not-started task from dispatching runtime updates.
TEST(AIActionTask, TickAdvancesOnlyRunningTask)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);

    EXPECT_EQ(task.Tick(1.0f), AIActionExecutionStatus::NotStarted);
    EXPECT_EQ(runtime->TickCallCount(), 0);

    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Running);
    EXPECT_EQ(task.Tick(0.125f), AIActionExecutionStatus::Running);

    ASSERT_EQ(runtime->TickCallCount(), 1);
    EXPECT_EQ(runtime->tickContexts.front(), MakeContext());
    EXPECT_FLOAT_EQ(runtime->tickDeltaSeconds.front(), 0.125f);
}

// Protects natural successful completion from Tick so terminal success is owned
// by the task and cannot require planner-side lifecycle bookkeeping.
TEST(AIActionTask, TickCanTransitionRunningTaskToSucceeded)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Running);
    runtime->tickResult = AIActionRuntimeResult::Succeeded;

    EXPECT_EQ(task.Tick(0.5f), AIActionExecutionStatus::Succeeded);

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Succeeded);
    EXPECT_TRUE(task.IsTerminal());
}

// Protects natural runtime failure from Tick so failure remains distinct from
// explicit cancellation and is stable once reported.
TEST(AIActionTask, TickCanTransitionRunningTaskToFailed)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Running);
    runtime->tickResult = AIActionRuntimeResult::Failed;

    EXPECT_EQ(task.Tick(0.5f), AIActionExecutionStatus::Failed);

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Failed);
    EXPECT_TRUE(task.IsTerminal());
}

// Protects terminal-state stability so later Start, Tick, or Cancel calls cannot
// dispatch a completed runtime again or mutate the completed task status.
TEST(AIActionTask, TerminalTaskDoesNotDispatchAdditionalRuntimeCalls)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    runtime->startResult = AIActionRuntimeResult::Succeeded;
    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Succeeded);

    EXPECT_EQ(task.Start(), AIActionExecutionStatus::Succeeded);
    EXPECT_EQ(task.Tick(0.25f), AIActionExecutionStatus::Succeeded);
    task.Cancel();

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Succeeded);
    EXPECT_EQ(runtime->StartCallCount(), 1);
    EXPECT_EQ(runtime->TickCallCount(), 0);
    EXPECT_EQ(runtime->CancelCallCount(), 0);
}

// Protects explicit interruption so cancellation invokes runtime cleanup exactly
// once and records Cancelled instead of conflating interruption with failure.
TEST(AIActionTask, CancelTransitionsRunningTaskToCancelled)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);
    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Running);

    task.Cancel();

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Cancelled);
    EXPECT_FALSE(task.IsRunning());
    EXPECT_TRUE(task.IsTerminal());
    ASSERT_EQ(runtime->CancelCallCount(), 1);
    EXPECT_EQ(runtime->cancelContexts.front(), MakeContext());
}

// Protects idempotent terminal cancellation behavior so repeated cancellation
// cannot issue duplicate runtime cleanup or advance the lifecycle again.
TEST(AIActionTask, RepeatedCancelDoesNotDispatchAgain)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask task = MakeTask(runtime);

    task.Cancel();
    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::NotStarted);
    EXPECT_EQ(runtime->CancelCallCount(), 0);

    ASSERT_EQ(task.Start(), AIActionExecutionStatus::Running);
    task.Cancel();
    task.Cancel();

    EXPECT_EQ(task.GetStatus(), AIActionExecutionStatus::Cancelled);
    EXPECT_EQ(runtime->CancelCallCount(), 1);
}

// Protects exclusive polymorphic ownership so destroying a task destroys its
// runtime through the virtual interface without requiring manual cleanup.
TEST(AIActionTask, TaskOwnsAndDestroysRuntime)
{
    bool destroyed = false;

    {
        auto runtime = std::make_unique<RecordingTaskRuntime>(&destroyed);
        AIActionTask task{ MakeContext(), std::move(runtime) };
        EXPECT_FALSE(destroyed);
    }

    EXPECT_TRUE(destroyed);
}

// Protects move ownership so a moved task keeps the single runtime invocation
// and the moved-from object remains safely destructible.
TEST(AIActionTask, MoveConstructedTaskKeepsRuntimeOwnership)
{
    RecordingTaskRuntime* runtime = nullptr;
    AIActionTask original = MakeTask(runtime);

    AIActionTask moved{ std::move(original) };

    EXPECT_EQ(moved.Start(), AIActionExecutionStatus::Running);
    EXPECT_EQ(runtime->StartCallCount(), 1);
}