#include <gtest/gtest.h>

import core;

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <limits>

namespace
{
    constexpr float FixedDeltaSeconds = 1.0f / 60.0f;
    constexpr float HalfFixedDeltaSeconds = 1.0f / 120.0f;

    class InitializedJoltPhysicsWorldTest : public testing::Test
    {
    protected:
        void SetUp() override
        {
            threadAffinity::ResetOwnerThreadRegistry();
            threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
            ASSERT_TRUE(runtime.Initialize());
            ASSERT_TRUE(world.Initialize());
        }

        void TearDown() override
        {
            world.Shutdown();
            runtime.Shutdown();
            threadAffinity::ResetOwnerThreadRegistry();
        }

        physics::JoltRuntime runtime;
        physics::JoltPhysicsWorld world{ runtime };
    };
}

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

TEST(JoltPhysicsWorldTests, UpdateBeforeInitializeExecutesNoSteps)
{
    physics::JoltRuntime runtime;
    physics::JoltPhysicsWorld world(runtime);
    EXPECT_EQ(world.Update(FixedDeltaSeconds), 0u);
}

TEST_F(InitializedJoltPhysicsWorldTest, ZeroDeltaExecutesNoSteps)
{
    EXPECT_EQ(world.Update(0.0f), 0u);
}

TEST_F(InitializedJoltPhysicsWorldTest, NegativeDeltaExecutesNoSteps)
{
    EXPECT_EQ(world.Update(-FixedDeltaSeconds), 0u);
}

TEST_F(InitializedJoltPhysicsWorldTest, NonFiniteDeltaExecutesNoSteps)
{
    EXPECT_EQ(world.Update(std::numeric_limits<float>::quiet_NaN()), 0u);
    EXPECT_EQ(world.Update(std::numeric_limits<float>::infinity()), 0u);
}

TEST_F(InitializedJoltPhysicsWorldTest, SixtyFpsExecutesSixtyStepsPerSecond)
{
    std::uint32_t totalSteps = 0u;
    for (std::uint32_t frameIndex = 0u; frameIndex < 60u; ++frameIndex)
    {
        totalSteps += world.Update(FixedDeltaSeconds);
    }
    EXPECT_EQ(totalSteps, 60u);
}

TEST_F(InitializedJoltPhysicsWorldTest, ThirtyFpsExecutesSixtyStepsPerSecond)
{
    std::uint32_t totalSteps = 0u;
    for (std::uint32_t frameIndex = 0u; frameIndex < 30u; ++frameIndex)
    {
        totalSteps += world.Update(1.0f / 30.0f);
    }
    EXPECT_EQ(totalSteps, 60u);
}

TEST_F(InitializedJoltPhysicsWorldTest, OneHundredTwentyFpsExecutesSixtyStepsPerSecond)
{
    std::uint32_t totalSteps = 0u;
    for (std::uint32_t frameIndex = 0u; frameIndex < 120u; ++frameIndex)
    {
        totalSteps += world.Update(HalfFixedDeltaSeconds);
    }
    EXPECT_EQ(totalSteps, 60u);
}

TEST_F(InitializedJoltPhysicsWorldTest, LongFrameExecutesAtMostMaximumSteps)
{
    EXPECT_EQ(world.Update(10.0f), 4u);
}

TEST_F(InitializedJoltPhysicsWorldTest, LongFrameDoesNotCreatePersistentBacklog)
{
    EXPECT_EQ(world.Update(10.0f), 4u);
    EXPECT_EQ(world.Update(0.0f), 0u);

    // A catch-up cap without dropping excess accumulated time would replay the old hitch here.
    EXPECT_EQ(world.Update(FixedDeltaSeconds), 1u);
}

TEST_F(InitializedJoltPhysicsWorldTest, FractionalFramesAccumulateIntoOneStep)
{
    EXPECT_EQ(world.Update(HalfFixedDeltaSeconds), 0u);
    EXPECT_EQ(world.Update(HalfFixedDeltaSeconds), 1u);
}

TEST_F(InitializedJoltPhysicsWorldTest, ResetSimulationClockDiscardsFractionalRemainder)
{
    EXPECT_EQ(world.Update(HalfFixedDeltaSeconds), 0u);
    world.ResetSimulationClock();
    EXPECT_EQ(world.Update(HalfFixedDeltaSeconds), 0u);
    EXPECT_EQ(world.Update(HalfFixedDeltaSeconds), 1u);
}

TEST_F(InitializedJoltPhysicsWorldTest, UpdateAfterShutdownExecutesNoSteps)
{
    world.Shutdown();
    EXPECT_EQ(world.Update(FixedDeltaSeconds), 0u);
}