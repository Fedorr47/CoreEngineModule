#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

import core;

TEST(RenderCommandQueue, ExecutesCommandsInFifoOrder)
{
    rendern::FRenderCommandQueue queue;
    std::vector<int> values;

    ASSERT_TRUE(queue.Enqueue([&] { values.push_back(1); }));
    ASSERT_TRUE(queue.Enqueue([&] { values.push_back(2); }));
    ASSERT_TRUE(queue.Enqueue([&] { values.push_back(3); }));

    queue.DrainReadyCommands();

    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST(RenderCommandQueue, RejectsCommandsAfterStop)
{
    rendern::FRenderCommandQueue queue;
    bool rejectedCommandExecuted = false;

    queue.RequestStop();

    EXPECT_FALSE(queue.Enqueue([&] { rejectedCommandExecuted = true; }));
    queue.DrainReadyCommands();

    EXPECT_FALSE(rejectedCommandExecuted);
}

TEST(RenderCommandQueue, StopWakesBlockedConsumer)
{
    rendern::FRenderCommandQueue queue;
    std::atomic<bool> waitResult = true;

    std::thread consumer([&]
    {
        waitResult = queue.WaitAndDrainReadyCommands();
    });

    queue.RequestStop();
    consumer.join();

    EXPECT_FALSE(waitResult.load());
}

TEST(RenderCommandQueue, WaitIdleWaitsForActiveCommand)
{
    rendern::FRenderCommandQueue queue;
    std::mutex mutex;
    std::condition_variable commandStartedCondition;
    std::condition_variable finishCommandCondition;
    bool commandStarted = false;
    bool finishCommand = false;
    std::atomic<bool> waitIdleReturned = false;

    ASSERT_TRUE(queue.Enqueue([&]
    {
        std::unique_lock lock(mutex);
        commandStarted = true;
        commandStartedCondition.notify_all();
        finishCommandCondition.wait(lock, [&] { return finishCommand; });
    }));

    std::thread drainer([&]
    {
        queue.DrainReadyCommands();
    });

    {
        std::unique_lock lock(mutex);
        commandStartedCondition.wait(lock, [&] { return commandStarted; });
    }

    std::thread idleWaiter([&]
    {
        queue.WaitIdle();
        waitIdleReturned = true;
    });

    std::this_thread::yield();
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

TEST(RenderCommandQueue, CommandsCanEnqueueCommandsWithoutDeadlock)
{
    rendern::FRenderCommandQueue queue;
    std::vector<int> values;

    ASSERT_TRUE(queue.Enqueue([&]
    {
        values.push_back(1);
        EXPECT_TRUE(queue.Enqueue([&] { values.push_back(2); }));
    }));

    queue.DrainReadyCommands();

    EXPECT_EQ(values, (std::vector<int>{1, 2}));
}


TEST(RenderCommandQueue, ThrowingCommandLeavesQueueDrainableAndIdleObservable)
{
    rendern::FRenderCommandQueue queue;
    std::atomic<bool> trailingCommandExecuted = false;

    ASSERT_TRUE(queue.Enqueue([] { throw std::runtime_error("queue command failure"); }));
    ASSERT_TRUE(queue.Enqueue([&] { trailingCommandExecuted = true; }));

    EXPECT_THROW(queue.DrainReadyCommands(), std::runtime_error);

    queue.RequestStop();
    EXPECT_TRUE(queue.WaitAndDrainReadyCommands());
    queue.WaitIdle();

    EXPECT_TRUE(trailingCommandExecuted.load());
}
