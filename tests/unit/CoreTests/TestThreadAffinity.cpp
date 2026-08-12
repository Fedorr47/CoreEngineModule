#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <string>
#include <thread>

#include "FakeRHI.h"
#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    LevelAsset MakeMinimalLevelAsset()
    {
        LevelAsset asset;
        asset.name = "ThreadAffinitySmokeFixture";

        LevelNode node{};
        node.name = "Player";
        node.alive = true;
        node.visible = true;
        node.transform.position = { 0.0f, 0.0f, 0.0f };
        asset.nodes.push_back(node);
        return asset;
    }
}

TEST(ThreadAffinityCore, RegisterRoleToCurrentThread)
{
    threadAffinity::ResetOwnerThreadRegistry();

    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);

    EXPECT_TRUE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Main));
    EXPECT_TRUE(threadAffinity::IsOwnerThread(threadAffinity::ThreadOwnerRole::Main));
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main), std::this_thread::get_id());
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Main));
}

TEST(ThreadAffinityCore, RegisterMultipleRolesToSameThread)
{
    threadAffinity::ResetOwnerThreadRegistry();

    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Runtime);
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);

    const std::thread::id currentThreadId = std::this_thread::get_id();
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main), currentThreadId);
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Runtime), currentThreadId);
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Render), currentThreadId);
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Main));
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Runtime));
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Render));
}

TEST(ThreadAffinityCore, ReRegisterRoleUpdatesOwner)
{
    threadAffinity::ResetOwnerThreadRegistry();

    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
    const std::thread::id mainThreadId = threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main);

    std::promise<std::thread::id> workerThreadIdPromise;
    std::future<std::thread::id> workerThreadIdFuture = workerThreadIdPromise.get_future();

    std::thread worker([&workerThreadIdPromise]()
    {
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
        workerThreadIdPromise.set_value(std::this_thread::get_id());
    });

    const std::thread::id workerThreadId = workerThreadIdFuture.get();
    worker.join();

    EXPECT_NE(workerThreadId, mainThreadId);
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main), workerThreadId);

    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main), std::this_thread::get_id());
}

TEST(ThreadAffinityCore, UnregisterRoleInvalidatesOwner)
{
    threadAffinity::ResetOwnerThreadRegistry();

    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);
    EXPECT_TRUE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Render));

    threadAffinity::UnregisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);
    EXPECT_FALSE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Render));
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Render), std::thread::id{});
    EXPECT_FALSE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Render));
}

TEST(ThreadAffinityCore, ResetClearsAllRoles)
{
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Runtime);
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);

    threadAffinity::ResetOwnerThreadRegistry();

    EXPECT_FALSE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Main));
    EXPECT_FALSE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Runtime));
    EXPECT_FALSE(threadAffinity::IsOwnerThreadRegistered(threadAffinity::ThreadOwnerRole::Render));
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Main), std::thread::id{});
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Runtime), std::thread::id{});
    EXPECT_EQ(threadAffinity::GetOwnerThreadId(threadAffinity::ThreadOwnerRole::Render), std::thread::id{});
}

TEST(ThreadAffinityCore, MismatchIsDetectableThroughNonAssertApi)
{
    threadAffinity::ResetOwnerThreadRegistry();
    threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);

    std::promise<threadAffinity::ThreadOwnerCheckResult> resultPromise;
    std::future<threadAffinity::ThreadOwnerCheckResult> resultFuture = resultPromise.get_future();

    std::thread worker([&resultPromise]()
    {
        resultPromise.set_value(threadAffinity::CheckOwnerThreadDetailed(threadAffinity::ThreadOwnerRole::Main));
    });

    const threadAffinity::ThreadOwnerCheckResult result = resultFuture.get();
    worker.join();

    EXPECT_FALSE(result.matches);
    EXPECT_TRUE(result.registered);
    EXPECT_NE(result.message.find("Main"), std::string::npos);
    EXPECT_NE(result.ownerThreadId, result.currentThreadId);
}

TEST(ThreadAffinityCore, InvalidRoleDoesNotIndexOutsideRegistry)
{
    threadAffinity::ResetOwnerThreadRegistry();

    const auto invalidRole = static_cast<threadAffinity::ThreadOwnerRole>(255);
    const threadAffinity::ThreadOwnerCheckResult result =
        threadAffinity::CheckOwnerThreadDetailed(invalidRole);

    EXPECT_FALSE(result.matches);
    EXPECT_FALSE(result.registered);
    EXPECT_EQ(result.ownerThreadId, std::thread::id{});
    EXPECT_EQ(result.currentThreadId, std::this_thread::get_id());
    EXPECT_NE(result.message.find("invalid"), std::string::npos);
}

TEST(ThreadAffinityCore, InlineModeAllowsMainRuntimeAndRenderToShareThread)
{
    InlineThreadOwnerRolesGuard guard{};

    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Main));
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Runtime));
    EXPECT_TRUE(threadAffinity::CheckOwnerThread(threadAffinity::ThreadOwnerRole::Render));
}

TEST(ThreadAffinityWorkerContext, WorkerContextIsScopedToWorkerJobs)
{
    rendern::JobSystemThreadPool pool(1);

    std::atomic<bool> sawWorkerThread{ false };
    std::atomic<bool> sawWorkerThreadForPool{ false };
    std::atomic<bool> sawOutsideWorkerThread{ false };

    pool.Enqueue([&]()
    {
        sawWorkerThread = threadAffinity::IsWorkerThread();
        sawWorkerThreadForPool = threadAffinity::IsWorkerThreadFor(&pool);
    });

    pool.WaitIdle();

    sawOutsideWorkerThread = threadAffinity::IsWorkerThread();

    EXPECT_TRUE(sawWorkerThread.load());
    EXPECT_TRUE(sawWorkerThreadForPool.load());
    EXPECT_FALSE(sawOutsideWorkerThread.load());
    EXPECT_FALSE(threadAffinity::IsWorkerThreadFor(&pool));
}

TEST(ThreadAffinitySmoke, InlineModeGuardsAllowGameplayAndRendererEntryPoints)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeMinimalLevelAsset();
    LevelInstance levelInstance{};
    Scene scene{};

    GameplayRuntime runtime{};
    runtime.Initialize(levelAsset, levelInstance, scene);

    GameplayUpdateContext context{};
    context.mode = GameplayRuntimeMode::Editor;
    context.levelAsset = &levelAsset;
    context.levelInstance = &levelInstance;
    context.scene = &scene;

    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(context);
    runtime.PostPhysicsUpdate(context);
    runtime.PostAnimationUpdate(context);

    FakeRHIDevice device{};
    FakeRHISwapChain swapChain(rhi::SwapChainDesc{
        .extent = { 640u, 360u },
        .backbufferFormat = rhi::Format::BGRA8_UNORM,
        .vsync = false
    });

    rendern::Renderer renderer(device);
    renderer.RenderFrame(swapChain, rendern::RenderSceneExtractor::BuildFrameView(scene));

    EXPECT_EQ(swapChain.GetPresentCount(), 1u);

    runtime.Shutdown();
}
