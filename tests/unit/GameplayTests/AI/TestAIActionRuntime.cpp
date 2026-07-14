#include <gtest/gtest.h>

#include <memory>

import core;

using namespace rendern;

namespace
{
    class RecordingActionRuntime final : public IAIActionRuntime
    {
    public:
        explicit RecordingActionRuntime(bool* destroyedFlag = nullptr) noexcept
            : destroyedFlag_{ destroyedFlag }
        {
        }

        ~RecordingActionRuntime() override
        {
            if (destroyedFlag_ != nullptr)
            {
                *destroyedFlag_ = true;
            }
        }

        AIActionRuntimeResult startStatus{ AIActionRuntimeResult::Running };
        AIActionRuntimeResult tickStatus{ AIActionRuntimeResult::Running };
        AIActionRuntimeContext lastStartContext{};
        AIActionRuntimeContext lastTickContext{};
        AIActionRuntimeContext lastCancelContext{};
        float lastDeltaSeconds{ 0.0f };
        int startCallCount{ 0 };
        int tickCallCount{ 0 };
        int cancelCallCount{ 0 };

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            ++startCallCount;
            lastStartContext = context;
            return startStatus;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            ++tickCallCount;
            lastTickContext = context;
            lastDeltaSeconds = deltaSeconds;
            return tickStatus;
        }

        void Cancel(
            const AIActionRuntimeContext& context) noexcept override
        {
            ++cancelCallCount;
            lastCancelContext = context;
        }

    private:
        bool* destroyedFlag_{ nullptr };
    };
}

// Protects the runtime boundary's virtual start dispatch and context forwarding
// so future task ownership can observe the implementation's immediate result
// without embedding execution behavior in passive action definitions.
TEST(AIActionRuntime, StartForwardsExecutionContextAndReturnsRuntimeStatus)
{
    RecordingActionRuntime runtime{};
    IAIActionRuntime& interface = runtime;
    const AIActionRuntimeContext context{
        .agentEntity = 42u,
        .actionId = AIActionId{ 7u }
    };
    runtime.startStatus = AIActionRuntimeResult::Succeeded;

    const AIActionRuntimeResult status = interface.Start(context);

    EXPECT_EQ(status, AIActionRuntimeResult::Succeeded);
    EXPECT_EQ(runtime.startCallCount, 1);
    EXPECT_EQ(runtime.lastStartContext.agentEntity, context.agentEntity);
    EXPECT_EQ(runtime.lastStartContext.actionId, context.actionId);
}

// Protects running update dispatch and delta forwarding so the action task
// model can drive a running action deterministically without the runtime
// interface owning frame timing or lifecycle state.
TEST(AIActionRuntime, TickForwardsDeltaTimeAndReturnsRunningStatus)
{
    RecordingActionRuntime runtime{};
    IAIActionRuntime& interface = runtime;
    const AIActionRuntimeContext context{
        .agentEntity = 84u,
        .actionId = AIActionId{ 9u }
    };
    runtime.tickStatus = AIActionRuntimeResult::Running;

    const AIActionRuntimeResult status = interface.Tick(context, 0.25f);

    EXPECT_EQ(status, AIActionRuntimeResult::Running);
    EXPECT_EQ(runtime.tickCallCount, 1);
    EXPECT_EQ(runtime.lastTickContext.agentEntity, context.agentEntity);
    EXPECT_EQ(runtime.lastTickContext.actionId, context.actionId);
    EXPECT_FLOAT_EQ(runtime.lastDeltaSeconds, 0.25f);
}

// Protects terminal success propagation from Tick without introducing task-side
// terminal handling, which remains owned by the later action task model.
TEST(AIActionRuntime, TickCanReportSuccessfulCompletion)
{
    RecordingActionRuntime runtime{};
    IAIActionRuntime& interface = runtime;
    runtime.tickStatus = AIActionRuntimeResult::Succeeded;

    const AIActionRuntimeResult status = interface.Tick(
        AIActionRuntimeContext{ .agentEntity = 21u, .actionId = AIActionId{ 3u } },
        0.5f);

    EXPECT_EQ(status, AIActionRuntimeResult::Succeeded);
}

// Protects terminal failure propagation from Tick while keeping retries,
// replanning, and terminal-state ownership outside this runtime contract.
TEST(AIActionRuntime, TickCanReportFailure)
{
    RecordingActionRuntime runtime{};
    IAIActionRuntime& interface = runtime;
    runtime.tickStatus = AIActionRuntimeResult::Failed;

    const AIActionRuntimeResult status = interface.Tick(
        AIActionRuntimeContext{ .agentEntity = 22u, .actionId = AIActionId{ 4u } },
        0.75f);

    EXPECT_EQ(status, AIActionRuntimeResult::Failed);
}

// Protects explicit cancellation dispatch so cleanup and interruption have a
// separate boundary from success effects and remain caller-driven.
TEST(AIActionRuntime, CancelForwardsExecutionContext)
{
    RecordingActionRuntime runtime{};
    IAIActionRuntime& interface = runtime;
    const AIActionRuntimeContext context{
        .agentEntity = 126u,
        .actionId = AIActionId{ 11u }
    };

    interface.Cancel(context);

    EXPECT_EQ(runtime.cancelCallCount, 1);
    EXPECT_EQ(runtime.lastCancelContext.agentEntity, context.agentEntity);
    EXPECT_EQ(runtime.lastCancelContext.actionId, context.actionId);
}

// Protects polymorphic ownership safety for the future task model, which may
// own concrete runtimes behind the lifecycle interface without leaking cleanup.
TEST(AIActionRuntime, RuntimeCanBeDestroyedThroughInterface)
{
    bool destroyed = false;

    {
        std::unique_ptr<IAIActionRuntime> runtime =
            std::make_unique<RecordingActionRuntime>(&destroyed);
    }

    EXPECT_TRUE(destroyed);
}