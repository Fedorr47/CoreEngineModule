#include <gtest/gtest.h>

#include "Physics/Jolt/JoltRuntime.h"

TEST(JoltRuntimeTests, InitializesAndShutsDown)
{
    physics::JoltRuntime runtime;

    EXPECT_FALSE(runtime.IsInitialized());

    ASSERT_TRUE(runtime.Initialize());
    EXPECT_TRUE(runtime.IsInitialized());

    runtime.Shutdown();

    EXPECT_FALSE(runtime.IsInitialized());
}

TEST(JoltRuntimeTests, RepeatedInitializationOfSameOwnerIsIdempotent)
{
    physics::JoltRuntime runtime;

    ASSERT_TRUE(runtime.Initialize());

    // Reinitializing the same owner must not register Jolt types twice.
    EXPECT_TRUE(runtime.Initialize());
    EXPECT_TRUE(runtime.IsInitialized());
}

TEST(JoltRuntimeTests, RejectsSecondProcessWideOwner)
{
    physics::JoltRuntime firstRuntime;
    physics::JoltRuntime secondRuntime;

    ASSERT_TRUE(firstRuntime.Initialize());

    // Jolt exposes one process-wide RTTI factory, so two owners would make
    // shutdown order ambiguous and could leave Factory::sInstance dangling.
    EXPECT_FALSE(secondRuntime.Initialize());
    EXPECT_FALSE(secondRuntime.IsInitialized());

    firstRuntime.Shutdown();

    // Once the original owner releases Jolt, a new runtime can take ownership.
    EXPECT_TRUE(secondRuntime.Initialize());
    EXPECT_TRUE(secondRuntime.IsInitialized());
}