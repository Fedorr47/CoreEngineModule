#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

import core;

// Protects the inline execution boundary: scheduling work must not run it immediately.
TEST(InlineRenderScheduler, EnqueueDoesNotExecuteImmediately)
{
    rendern::FInlineRenderScheduler scheduler;
    bool commandExecuted = false;

    ASSERT_TRUE(scheduler.Enqueue([&] { commandExecuted = true; }));

    EXPECT_FALSE(commandExecuted);

    scheduler.DrainReadyCommands();

    EXPECT_TRUE(commandExecuted);
}

// Protects deterministic command ordering for callers that rely on queue submission order.
TEST(InlineRenderScheduler, DrainExecutesQueuedCommandsInFifoOrder)
{
    rendern::FInlineRenderScheduler scheduler;
    std::vector<int> values;

    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(1); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(2); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(3); }));

    scheduler.DrainReadyCommands();

    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

// Protects the inline backend contract: render work runs on the explicit drain caller.
TEST(InlineRenderScheduler, DrainRunsCommandsOnCallingThread)
{
    rendern::FInlineRenderScheduler scheduler;
    std::thread::id drainThread;
    std::thread::id executionThread;

    ASSERT_TRUE(scheduler.Enqueue([&] { executionThread = std::this_thread::get_id(); }));

    std::thread drainer([&]
    {
        drainThread = std::this_thread::get_id();
        scheduler.DrainReadyCommands();
    });

    drainer.join();

    EXPECT_EQ(executionThread, drainThread);
}

// Protects deterministic shutdown behavior: stopped schedulers reject future submissions.
TEST(InlineRenderScheduler, RequestStopRejectsFutureCommands)
{
    rendern::FInlineRenderScheduler scheduler;
    bool rejectedCommandExecuted = false;

    scheduler.RequestStop();

    EXPECT_TRUE(scheduler.IsStopRequested());
    EXPECT_FALSE(scheduler.Enqueue([&] { rejectedCommandExecuted = true; }));
    scheduler.DrainReadyCommands();

    EXPECT_FALSE(rejectedCommandExecuted);
}

// Protects idle observation across queued and active work exposed through the scheduler boundary.
TEST(InlineRenderScheduler, WaitIdleReturnsAfterDrainedWorkCompletes)
{
    rendern::FInlineRenderScheduler scheduler;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable finishCommandCondition;
    std::condition_variable waitIdleStartedCondition;
    bool commandStarted = false;
    bool finishCommand = false;
    bool waitIdleStarted = false;
    std::atomic<bool> waitIdleReturned = false;

    ASSERT_TRUE(scheduler.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        finishCommandCondition.wait(lock, [&] { return finishCommand; });
    }));

    std::thread drainer([&]
    {
        scheduler.DrainReadyCommands();
    });

    {
        std::unique_lock lock(mutex);
        commandStartedCondition.wait(lock, [&] { return commandStarted; });
    }

    std::thread idleWaiter([&]
    {
        {
            std::lock_guard lock(mutex);
            waitIdleStarted = true;
        }
        waitIdleStartedCondition.notify_all();

        scheduler.WaitIdle();
        waitIdleReturned = true;
    });

    {
        std::unique_lock lock(mutex);
        waitIdleStartedCondition.wait(lock, [&] { return waitIdleStarted; });
    }
    EXPECT_FALSE(waitIdleReturned.load());

    {
        std::lock_guard lock(mutex);
        finishCommand = true;
    }
    finishCommandCondition.notify_all();

    drainer.join();
    idleWaiter.join();

    EXPECT_TRUE(waitIdleReturned.load());
}
