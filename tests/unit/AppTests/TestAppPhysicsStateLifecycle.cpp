#include <gtest/gtest.h>

import core;
import std;

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "App/AppLifecycle.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

TEST(AppPhysicsStateLifecycle, StartsWithoutPhysicsObjects)
{
    appLifecycle::AppPhysicsState physicsState;

    EXPECT_EQ(physicsState.joltRuntime, nullptr);
    EXPECT_EQ(physicsState.joltPhysicsWorld, nullptr);
    EXPECT_EQ(physicsState.obstacleQuery, nullptr);
    EXPECT_EQ(physicsState.levelPhysicsRuntime, nullptr);
}

TEST(AppPhysicsStateLifecycle, InitializesRuntimeBeforeWorld)
{
    appLifecycle::AppPhysicsState physicsState;

    physicsState.Initialize();

    ASSERT_NE(physicsState.joltRuntime, nullptr);
    ASSERT_NE(physicsState.joltPhysicsWorld, nullptr);
    ASSERT_NE(physicsState.obstacleQuery, nullptr);
    ASSERT_NE(physicsState.levelPhysicsRuntime, nullptr);
    EXPECT_TRUE(physicsState.joltRuntime->IsInitialized());
    EXPECT_TRUE(physicsState.joltPhysicsWorld->IsInitialized());
}

TEST(AppPhysicsStateLifecycle, ShutdownDestroysWorldAndRuntime)
{
    appLifecycle::AppPhysicsState physicsState;
    physicsState.Initialize();

    physicsState.Shutdown();

    EXPECT_EQ(physicsState.joltPhysicsWorld, nullptr);
    EXPECT_EQ(physicsState.obstacleQuery, nullptr);
    EXPECT_EQ(physicsState.joltRuntime, nullptr);
    EXPECT_EQ(physicsState.levelPhysicsRuntime, nullptr);

    physics::JoltRuntime independentRuntime;
    EXPECT_TRUE(independentRuntime.Initialize());
}

TEST(AppPhysicsStateLifecycle, RepeatedShutdownIsSafe)
{
    appLifecycle::AppPhysicsState physicsState;
    physicsState.Initialize();

    physicsState.Shutdown();
    physicsState.Shutdown();

    EXPECT_EQ(physicsState.joltPhysicsWorld, nullptr);
    EXPECT_EQ(physicsState.obstacleQuery, nullptr);
    EXPECT_EQ(physicsState.joltRuntime, nullptr);
    EXPECT_EQ(physicsState.levelPhysicsRuntime, nullptr);
}

TEST(AppPhysicsStateLifecycle, CanInitializeAgainAfterShutdown)
{
    appLifecycle::AppPhysicsState physicsState;
    physicsState.Initialize();
    physicsState.Shutdown();

    physicsState.Initialize();

    ASSERT_NE(physicsState.joltRuntime, nullptr);
    ASSERT_NE(physicsState.joltPhysicsWorld, nullptr);
    ASSERT_NE(physicsState.obstacleQuery, nullptr);
    ASSERT_NE(physicsState.levelPhysicsRuntime, nullptr);
    EXPECT_TRUE(physicsState.joltRuntime->IsInitialized());
    EXPECT_TRUE(physicsState.joltPhysicsWorld->IsInitialized());
}
