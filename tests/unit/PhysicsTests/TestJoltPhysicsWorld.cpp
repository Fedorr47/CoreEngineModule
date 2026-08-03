#include <gtest/gtest.h>

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

TEST(JoltPhysicsWorldTests, StartsUninitialized)
{
    physics::JoltRuntime runtime;
    physics::JoltPhysicsWorld world(runtime);
    EXPECT_FALSE(world.IsInitialized());
}

TEST(JoltPhysicsWorldTests, RequiresInitializedRuntime)
{
    physics::JoltRuntime runtime;
    physics::JoltPhysicsWorld world(runtime);
    EXPECT_FALSE(world.Initialize());
}

TEST(JoltPhysicsWorldTests, InitializesAndShutsDown)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld world(runtime);
    ASSERT_TRUE(world.Initialize());
    EXPECT_TRUE(world.IsInitialized());
    world.Shutdown();
    EXPECT_FALSE(world.IsInitialized());
}

TEST(JoltPhysicsWorldTests, RepeatedInitializeIsIdempotent)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld world(runtime);
    ASSERT_TRUE(world.Initialize());
    EXPECT_TRUE(world.Initialize());
}

TEST(JoltPhysicsWorldTests, RepeatedShutdownIsSafe)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld world(runtime);
    ASSERT_TRUE(world.Initialize());
    world.Shutdown();
    world.Shutdown();
    EXPECT_FALSE(world.IsInitialized());
}

TEST(JoltPhysicsWorldTests, CanInitializeAgainAfterShutdown)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld world(runtime);
    ASSERT_TRUE(world.Initialize());
    world.Shutdown();
    EXPECT_TRUE(world.Initialize());
}

TEST(JoltPhysicsWorldTests, TwoWorldsCanShareOneRuntime)
{
    physics::JoltRuntime runtime;
    ASSERT_TRUE(runtime.Initialize());
    physics::JoltPhysicsWorld first(runtime);
    physics::JoltPhysicsWorld second(runtime);

    // The process-wide factory is shared, while each world owns independent simulation state.
    ASSERT_TRUE(first.Initialize());
    ASSERT_TRUE(second.Initialize());
    EXPECT_TRUE(first.IsInitialized());
    EXPECT_TRUE(second.IsInitialized());

    first.Shutdown();
    EXPECT_FALSE(first.IsInitialized());
    EXPECT_TRUE(second.IsInitialized());
}