#include <gtest/gtest.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <optional>

import core;

namespace
{
    const JPH::BodyID FirstBodyId{ 10u, JPH::uint8{ 1u } };
    const JPH::BodyID SecondBodyId{ 20u, JPH::uint8{ 2u } };
    const JPH::BodyID ThirdBodyId{ 30u, JPH::uint8{ 3u } };

    void ExpectBodyId(const std::optional<JPH::BodyID>& actual, const JPH::BodyID expected)
    {
        ASSERT_TRUE(actual.has_value());
        EXPECT_EQ(actual->GetIndexAndSequenceNumber(), expected.GetIndexAndSequenceNumber());
    }
}

TEST(JoltBodyRegistry, RejectsInvalidBodyId)
{
    physics::jolt::JoltBodyRegistry registry;

    EXPECT_EQ(registry.AllocateHandle(JPH::BodyID{}), physics::InvalidPhysicsBodyHandle);
    const auto handle = registry.AllocateHandle(FirstBodyId);
    EXPECT_EQ(handle.index, 0u);
}

TEST(JoltBodyRegistry, AllocatesAndResolvesBodyId)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto handle = registry.AllocateHandle(FirstBodyId);

    EXPECT_TRUE(handle.IsValid());
    EXPECT_TRUE(registry.IsValid(handle));
    ExpectBodyId(registry.ResolveBodyID(handle), FirstBodyId);
}

TEST(JoltBodyRegistry, MultipleLiveBodiesUseDifferentSlots)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto firstHandle = registry.AllocateHandle(FirstBodyId);
    const auto secondHandle = registry.AllocateHandle(SecondBodyId);

    EXPECT_TRUE(firstHandle.IsValid());
    EXPECT_TRUE(secondHandle.IsValid());
    EXPECT_NE(firstHandle.index, secondHandle.index);
    ExpectBodyId(registry.ResolveBodyID(firstHandle), FirstBodyId);
    ExpectBodyId(registry.ResolveBodyID(secondHandle), SecondBodyId);
}

TEST(JoltBodyRegistry, ReleaseInvalidatesHandle)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto handle = registry.AllocateHandle(FirstBodyId);

    EXPECT_TRUE(registry.ReleaseHandle(handle));
    EXPECT_FALSE(registry.IsValid(handle));
    EXPECT_FALSE(registry.ResolveBodyID(handle).has_value());
    EXPECT_FALSE(registry.ReleaseHandle(handle));
}

TEST(JoltBodyRegistry, ReusesReleasedSlotWithNewGeneration)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto firstHandle = registry.AllocateHandle(FirstBodyId);
    ASSERT_TRUE(registry.ReleaseHandle(firstHandle));

    const auto secondHandle = registry.AllocateHandle(SecondBodyId);
    EXPECT_EQ(secondHandle.index, firstHandle.index);
    EXPECT_NE(secondHandle.generation, firstHandle.generation);
    EXPECT_TRUE(registry.IsValid(secondHandle));
    ExpectBodyId(registry.ResolveBodyID(secondHandle), SecondBodyId);
}

TEST(JoltBodyRegistry, StaleHandleDoesNotResolveAfterReuse)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto staleHandle = registry.AllocateHandle(FirstBodyId);
    ASSERT_TRUE(registry.ReleaseHandle(staleHandle));
    const auto currentHandle = registry.AllocateHandle(SecondBodyId);

    EXPECT_FALSE(registry.IsValid(staleHandle));
    EXPECT_FALSE(registry.ResolveBodyID(staleHandle).has_value());
    EXPECT_FALSE(registry.ReleaseHandle(staleHandle));
    EXPECT_TRUE(registry.IsValid(currentHandle));
}

TEST(JoltBodyRegistry, OutOfRangeHandleIsRejected)
{
    physics::jolt::JoltBodyRegistry registry;
    const physics::PhysicsBodyHandle handle{ .index = 42u, .generation = 1u };

    ASSERT_TRUE(handle.IsValid());
    EXPECT_FALSE(registry.IsValid(handle));
    EXPECT_FALSE(registry.ResolveBodyID(handle).has_value());
    EXPECT_FALSE(registry.ReleaseHandle(handle));
}

TEST(JoltBodyRegistry, DoubleReleaseDoesNotDuplicateFreeSlot)
{
    physics::jolt::JoltBodyRegistry registry;
    const auto firstHandle = registry.AllocateHandle(FirstBodyId);
    ASSERT_TRUE(registry.ReleaseHandle(firstHandle));
    EXPECT_FALSE(registry.ReleaseHandle(firstHandle));

    const auto secondHandle = registry.AllocateHandle(SecondBodyId);
    const auto thirdHandle = registry.AllocateHandle(ThirdBodyId);
    EXPECT_NE(secondHandle.index, thirdHandle.index);
}