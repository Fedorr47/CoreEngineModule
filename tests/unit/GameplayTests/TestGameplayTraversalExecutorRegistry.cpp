#include <gtest/gtest.h>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    class RegistryFakeTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        int startCallCount{0};

        GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext&) override
        {
            ++startCallCount;
            return GameplayTraversalExecutionResult::Succeeded;
        }

        GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext&, float) override
        {
            return GameplayTraversalExecutionResult::Succeeded;
        }

        void Cancel(const GameplayTraversalExecutionContext&) noexcept override {}
    };
}

// Protects registration lookup so a valid type ID resolves the same non-owned executor.
TEST(GameplayTraversalExecutorRegistry, RegistersAndFindsExecutor)
{
    GameplayTraversalExecutorRegistry registry{};
    RegistryFakeTraversalExecutor executor{};
    ASSERT_TRUE(registry.Register(kDoorTraversalTypeId, executor));
    EXPECT_EQ(registry.Find(kDoorTraversalTypeId), &executor);
}

// Protects structural validation so the invalid traversal type cannot be registered.
TEST(GameplayTraversalExecutorRegistry, RejectsInvalidTypeId)
{
    GameplayTraversalExecutorRegistry registry{};
    RegistryFakeTraversalExecutor executor{};
    EXPECT_FALSE(registry.Register(GameplayTraversalTypeId{}, executor));
    EXPECT_EQ(registry.Find(GameplayTraversalTypeId{}), nullptr);
}

// Protects stable dispatch so duplicate type registration cannot replace an executor.
TEST(GameplayTraversalExecutorRegistry, RejectsDuplicateTypeId)
{
    GameplayTraversalExecutorRegistry registry{};
    RegistryFakeTraversalExecutor firstExecutor{};
    RegistryFakeTraversalExecutor secondExecutor{};
    ASSERT_TRUE(registry.Register(kDoorTraversalTypeId, firstExecutor));
    EXPECT_FALSE(registry.Register(kDoorTraversalTypeId, secondExecutor));
    EXPECT_EQ(registry.Find(kDoorTraversalTypeId), &firstExecutor);
}

// Protects missing executor lookup without creating a registry entry.
TEST(GameplayTraversalExecutorRegistry, MissingLookupReturnsNull)
{
    const GameplayTraversalExecutorRegistry registry{};
    EXPECT_EQ(registry.Find(kJumpTraversalTypeId), nullptr);
}

// Protects non-owning removal so registration disappears while the executor remains usable.
TEST(GameplayTraversalExecutorRegistry, RemoveDeletesRegistration)
{
    GameplayTraversalExecutorRegistry registry{};
    RegistryFakeTraversalExecutor executor{};
    ASSERT_TRUE(registry.Register(kDoorTraversalTypeId, executor));
    ASSERT_TRUE(registry.Remove(kDoorTraversalTypeId));
    EXPECT_FALSE(registry.Contains(kDoorTraversalTypeId));
    EXPECT_EQ(executor.Start(GameplayTraversalExecutionContext{}),
        GameplayTraversalExecutionResult::Succeeded);
    EXPECT_EQ(executor.startCallCount, 1);
}

// Protects session cleanup by removing every non-owning executor registration.
TEST(GameplayTraversalExecutorRegistry, ResetClearsRegistrations)
{
    GameplayTraversalExecutorRegistry registry{};
    RegistryFakeTraversalExecutor doorExecutor{};
    RegistryFakeTraversalExecutor jumpExecutor{};
    ASSERT_TRUE(registry.Register(kDoorTraversalTypeId, doorExecutor));
    ASSERT_TRUE(registry.Register(kJumpTraversalTypeId, jumpExecutor));
    registry.Reset();
    EXPECT_FALSE(registry.Contains(kDoorTraversalTypeId));
    EXPECT_FALSE(registry.Contains(kJumpTraversalTypeId));
}

// Protects the thread-checked runtime facade for session executor registration and removal.
TEST(GameplayRuntime, ManagesGameplayTraversalExecutorRegistration)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime runtime{};
    RegistryFakeTraversalExecutor executor{};
    ASSERT_TRUE(runtime.RegisterGameplayTraversalExecutor(kDoorTraversalTypeId, executor));
    EXPECT_TRUE(runtime.HasGameplayTraversalExecutor(kDoorTraversalTypeId));
    EXPECT_TRUE(runtime.RemoveGameplayTraversalExecutor(kDoorTraversalTypeId));
    EXPECT_FALSE(runtime.HasGameplayTraversalExecutor(kDoorTraversalTypeId));
}