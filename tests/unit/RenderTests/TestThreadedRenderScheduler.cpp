#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

import core;

// Protects explicit lifecycle startup: threaded schedulers reject commands before Start accepts ownership of a worker thread.
TEST(ThreadedRenderScheduler, EnqueueBeforeStartIsRejected)
{
    rendern::FThreadedRenderScheduler scheduler;
    bool commandExecuted = false;

    EXPECT_FALSE(scheduler.Enqueue([&] { commandExecuted = true; }));

    EXPECT_FALSE(commandExecuted);
}

// Protects the threaded backend boundary: accepted work runs on the dedicated scheduler thread, not the caller thread.
TEST(ThreadedRenderScheduler, StartExecutesCommandsOnSchedulerThread)
{
    rendern::FThreadedRenderScheduler scheduler;
    const std::thread::id callerThread = std::this_thread::get_id();
    std::thread::id executionThread;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&] { executionThread = std::this_thread::get_id(); }));

    scheduler.WaitIdle();
    scheduler.StopAndJoin();

    EXPECT_NE(executionThread, callerThread);
}

// Protects deterministic render command ordering through the threaded scheduler backend.
TEST(ThreadedRenderScheduler, CommandsExecuteInFifoOrder)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::vector<int> values;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(1); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(2); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { values.push_back(3); }));

    scheduler.WaitIdle();
    scheduler.StopAndJoin();

    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

// Protects stop semantics: once stop is requested, later threaded submissions are rejected deterministically.
TEST(ThreadedRenderScheduler, RequestStopRejectsFutureCommands)
{
    rendern::FThreadedRenderScheduler scheduler;
    bool rejectedCommandExecuted = false;

    ASSERT_TRUE(scheduler.Start());
    scheduler.RequestStop();

    EXPECT_TRUE(scheduler.IsStopRequested());
    EXPECT_FALSE(scheduler.Enqueue([&] { rejectedCommandExecuted = true; }));

    scheduler.StopAndJoin();

    EXPECT_FALSE(rejectedCommandExecuted);
}

// Protects idle shutdown: StopAndJoin wakes an idle scheduler thread and leaves no running worker behind.
TEST(ThreadedRenderScheduler, StopAndJoinWakesIdleScheduler)
{
    rendern::FThreadedRenderScheduler scheduler;

    ASSERT_TRUE(scheduler.Start());

    scheduler.StopAndJoin();

    EXPECT_FALSE(scheduler.IsRunning());
    EXPECT_TRUE(scheduler.IsStopRequested());
}

// Protects drain-on-stop semantics: work accepted before StopAndJoin finishes before StopAndJoin returns.
TEST(ThreadedRenderScheduler, StopAndJoinDrainsAcceptedCommands)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::atomic<bool> commandCompleted = false;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&] { commandCompleted = true; }));

    scheduler.StopAndJoin();

    EXPECT_TRUE(commandCompleted.load());
}

// Protects enqueue/shutdown races: every command reported as accepted while shutdown races must complete before join returns.
TEST(ThreadedRenderScheduler, StopAndJoinDrainsCommandsAcceptedDuringConcurrentEnqueue)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable finishCommandCondition;
    bool commandStarted = false;
    bool finishCommand = false;
    std::atomic<bool> stopStarted = false;
    std::atomic<bool> stopReturned = false;
    std::atomic<bool> enqueuerSawRejectionAfterStop = false;
    std::atomic<int> acceptedCommandCount = 0;
    std::atomic<int> executedCommandCount = 0;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        finishCommandCondition.wait(lock, [&] { return finishCommand; });
    }));

    {
        std::unique_lock lock(mutex);
        commandStartedCondition.wait(lock, [&] { return commandStarted; });
    }

    std::thread enqueuer([&]
    {
        while (acceptedCommandCount.load() < 16)
        {
            if (scheduler.Enqueue([&] { ++executedCommandCount; }))
            {
                ++acceptedCommandCount;
            }
        }

        while (!stopReturned.load())
        {
            if (scheduler.Enqueue([&] { ++executedCommandCount; }))
            {
                ++acceptedCommandCount;
            }
            else if (stopStarted.load())
            {
                enqueuerSawRejectionAfterStop = true;
                return;
            }
        }
    });

    while (acceptedCommandCount.load() < 16)
    {
        std::this_thread::yield();
    }

    std::thread stopper([&]
    {
        stopStarted = true;
        scheduler.StopAndJoin();
        stopReturned = true;
    });

    for (int attempts = 0; attempts < 100000 && !enqueuerSawRejectionAfterStop.load(); ++attempts)
    {
        std::this_thread::yield();
    }
    EXPECT_TRUE(enqueuerSawRejectionAfterStop.load());

    {
        std::lock_guard lock(mutex);
        finishCommand = true;
    }
    finishCommandCondition.notify_all();

    enqueuer.join();
    stopper.join();

    EXPECT_EQ(executedCommandCount.load(), acceptedCommandCount.load());
    EXPECT_TRUE(scheduler.IsStopRequested());
}

// Protects idle observation: WaitIdle must include commands that are actively executing on the scheduler thread.
TEST(ThreadedRenderScheduler, WaitIdleWaitsForActiveCommand)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable finishCommandCondition;
    std::condition_variable waitIdleStartedCondition;
    bool commandStarted = false;
    bool finishCommand = false;
    bool waitIdleStarted = false;
    std::atomic<bool> waitIdleReturned = false;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        finishCommandCondition.wait(lock, [&] { return finishCommand; });
    }));

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

    idleWaiter.join();
    scheduler.StopAndJoin();

    EXPECT_TRUE(waitIdleReturned.load());
}

// Protects the lifecycle state machine: Start is a one-shot transition and cannot be performed twice.
TEST(ThreadedRenderScheduler, StartCannotBePerformedTwice)
{
    rendern::FThreadedRenderScheduler scheduler;

    EXPECT_TRUE(scheduler.Start());
    EXPECT_FALSE(scheduler.Start());

    scheduler.StopAndJoin();
}

// Protects threaded command error propagation: command failures are rethrown on the caller thread after shutdown.
TEST(ThreadedRenderScheduler, CommandFailureIsRethrownOnCallerThread)
{
    rendern::FThreadedRenderScheduler scheduler;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([] { throw std::runtime_error("threaded scheduler failure"); }));

    scheduler.WaitIdle();
    scheduler.StopAndJoin();

    EXPECT_TRUE(scheduler.HasFailure());

    try
    {
        scheduler.RethrowIfFailed();
        FAIL() << "Expected RethrowIfFailed to rethrow the captured command failure.";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_STREQ(error.what(), "threaded scheduler failure");
    }
}

// Protects failure stop semantics: once a command fails, the scheduler rejects later submissions.
TEST(ThreadedRenderScheduler, CommandFailureStopsFutureSubmissions)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::atomic<bool> trailingCommandExecuted = false;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([] { throw std::runtime_error("stop after failure"); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { trailingCommandExecuted = true; }));

    scheduler.WaitIdle();

    EXPECT_TRUE(trailingCommandExecuted.load());
    EXPECT_TRUE(scheduler.IsStopRequested());
    EXPECT_FALSE(scheduler.Enqueue([] {}));

    scheduler.StopAndJoin();

    EXPECT_TRUE(scheduler.HasFailure());
}

// Protects the drain policy: commands accepted before a failure request stop still execute.
TEST(ThreadedRenderScheduler, AcceptedCommandsDrainAfterEarlierCommandFails)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable releaseCommandCondition;
    bool commandStarted = false;
    bool releaseCommand = false;
    std::atomic<bool> trailingCommandExecuted = false;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        releaseCommandCondition.wait(lock, [&] { return releaseCommand; });
    }));

    {
        std::unique_lock lock(mutex);
        commandStartedCondition.wait(lock, [&] { return commandStarted; });
    }

    ASSERT_TRUE(scheduler.Enqueue([] { throw std::runtime_error("accepted failure"); }));
    ASSERT_TRUE(scheduler.Enqueue([&] { trailingCommandExecuted = true; }));

    {
        std::lock_guard lock(mutex);
        releaseCommand = true;
    }
    releaseCommandCondition.notify_all();

    scheduler.WaitIdle();
    scheduler.StopAndJoin();

    EXPECT_TRUE(trailingCommandExecuted.load());
    EXPECT_THROW(scheduler.RethrowIfFailed(), std::runtime_error);
}

// Protects first-failure preservation: later command exceptions do not replace the original failure.
TEST(ThreadedRenderScheduler, FirstCommandFailureIsPreserved)
{
    rendern::FThreadedRenderScheduler scheduler;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable releaseCommandCondition;
    bool commandStarted = false;
    bool releaseCommand = false;

    ASSERT_TRUE(scheduler.Start());
    ASSERT_TRUE(scheduler.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        releaseCommandCondition.wait(lock, [&] { return releaseCommand; });
    }));

    {
        std::unique_lock lock(mutex);
        commandStartedCondition.wait(lock, [&] { return commandStarted; });
    }

    ASSERT_TRUE(scheduler.Enqueue([] { throw std::runtime_error("first failure"); }));
    ASSERT_TRUE(scheduler.Enqueue([] { throw std::runtime_error("second failure"); }));

    {
        std::lock_guard lock(mutex);
        releaseCommand = true;
    }
    releaseCommandCondition.notify_all();

    scheduler.WaitIdle();
    scheduler.StopAndJoin();

    try
    {
        scheduler.RethrowIfFailed();
        FAIL() << "Expected RethrowIfFailed to rethrow the first captured command failure.";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_STREQ(error.what(), "first failure");
    }
}

TEST(ThreadedRenderScheduler, ConcurrentStopAndJoinWaitsForSingleJoin)
{
    rendern::FThreadedRenderScheduler scheduler;
    ASSERT_TRUE(scheduler.Start());

    std::thread first([&] { scheduler.StopAndJoin(); });
    std::thread second([&] { scheduler.StopAndJoin(); });

    first.join();
    second.join();

    EXPECT_FALSE(scheduler.IsRunning());
    EXPECT_TRUE(scheduler.IsStopRequested());
}
