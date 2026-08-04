#include <gtest/gtest.h>

import core;

TEST(PhysicsBodyHandle, DefaultHandleIsInvalid)
{
    constexpr physics::PhysicsBodyHandle handle;
    static_assert(!handle.IsValid());
    EXPECT_FALSE(handle.IsValid());
}

TEST(PhysicsBodyHandle, CanonicalInvalidHandleIsInvalid)
{
    static_assert(!physics::InvalidPhysicsBodyHandle.IsValid());
    EXPECT_FALSE(physics::InvalidPhysicsBodyHandle.IsValid());
}

TEST(PhysicsBodyHandle, NonZeroGenerationAndValidIndexIsValid)
{
    constexpr physics::PhysicsBodyHandle handle{ 4u, 1u };
    static_assert(handle.IsValid());
    EXPECT_TRUE(handle.IsValid());
}

TEST(PhysicsBodyHandle, InvalidIndexIsRejected)
{
    constexpr physics::PhysicsBodyHandle handle{ physics::PhysicsBodyHandle::InvalidIndex, 1u };
    static_assert(!handle.IsValid());
    EXPECT_FALSE(handle.IsValid());
}

TEST(PhysicsBodyHandle, ZeroGenerationIsRejected)
{
    constexpr physics::PhysicsBodyHandle handle{ 4u, physics::PhysicsBodyHandle::InvalidGeneration };
    static_assert(!handle.IsValid());
    EXPECT_FALSE(handle.IsValid());
}

TEST(PhysicsBodyHandle, MatchingIndexAndGenerationCompareEqual)
{
    constexpr physics::PhysicsBodyHandle first{ 4u, 2u };
    constexpr physics::PhysicsBodyHandle second{ 4u, 2u };
    static_assert(first == second);
    EXPECT_EQ(first, second);
}

TEST(PhysicsBodyHandle, ReusedSlotGenerationDoesNotMatchStaleHandle)
{
    constexpr physics::PhysicsBodyHandle staleHandle{ 4u, 2u };
    constexpr physics::PhysicsBodyHandle reusedSlotHandle{ 4u, 3u };

    static_assert(staleHandle.IsValid());
    static_assert(reusedSlotHandle.IsValid());
    static_assert(staleHandle != reusedSlotHandle);
    EXPECT_TRUE(staleHandle.IsValid());
    EXPECT_TRUE(reusedSlotHandle.IsValid());
    EXPECT_NE(staleHandle, reusedSlotHandle);
}

TEST(PhysicsTransform, DefaultValueIsIdentity)
{
    constexpr physics::PhysicsTransform transform;
    constexpr physics::PhysicsTransform identity{
        .position = { 0.0f, 0.0f, 0.0f },
        .rotationQuaternion = { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    static_assert(transform == identity);
    EXPECT_EQ(transform, identity);
}

TEST(PhysicsBodyDescriptor, DefaultsToStaticBody)
{
    constexpr physics::PhysicsBodyDescriptor descriptor;

    EXPECT_EQ(descriptor.transform, physics::PhysicsTransform{});
    EXPECT_EQ(descriptor.motionType, physics::PhysicsMotionType::Static);
}