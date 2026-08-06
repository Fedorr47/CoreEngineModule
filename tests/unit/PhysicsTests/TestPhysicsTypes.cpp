#include <gtest/gtest.h>

#include <limits>
#include <variant>

import core;

TEST(BoxShapeDescriptor, PositiveHalfExtentsAreValid)
{
    EXPECT_TRUE((physics::BoxShapeDescriptor{ { 1.0f, 2.0f, 3.0f } }).IsValid());
}

TEST(BoxShapeDescriptor, ZeroAxisIsInvalid)
{
    EXPECT_FALSE((physics::BoxShapeDescriptor{ { 1.0f, 0.0f, 3.0f } }).IsValid());
}

TEST(BoxShapeDescriptor, NegativeAxisIsInvalid)
{
    EXPECT_FALSE((physics::BoxShapeDescriptor{ { 1.0f, -2.0f, 3.0f } }).IsValid());
}

TEST(BoxShapeDescriptor, NonFiniteAxisIsInvalid)
{
    const float negativeInfinity = -std::numeric_limits<float>::infinity();

    EXPECT_FALSE((physics::BoxShapeDescriptor{
        { 1.0f, std::numeric_limits<float>::infinity(), 3.0f } }).IsValid());
    EXPECT_FALSE((physics::BoxShapeDescriptor{
        { 1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN() } }).IsValid());
    EXPECT_FALSE((physics::BoxShapeDescriptor{ { negativeInfinity, 2.0f, 3.0f } }).IsValid());
}

TEST(SphereShapeDescriptor, PositiveRadiusIsValid)
{
    EXPECT_TRUE((physics::SphereShapeDescriptor{ 2.0f }).IsValid());
}

TEST(SphereShapeDescriptor, ZeroRadiusIsInvalid)
{
    EXPECT_FALSE((physics::SphereShapeDescriptor{}).IsValid());
}

TEST(SphereShapeDescriptor, NegativeRadiusIsInvalid)
{
    EXPECT_FALSE((physics::SphereShapeDescriptor{ -1.0f }).IsValid());
}

TEST(SphereShapeDescriptor, NonFiniteRadiusIsInvalid)
{
    const float negativeInfinity = -std::numeric_limits<float>::infinity();

    EXPECT_FALSE((physics::SphereShapeDescriptor{ std::numeric_limits<float>::infinity() }).IsValid());
    EXPECT_FALSE((physics::SphereShapeDescriptor{ std::numeric_limits<float>::quiet_NaN() }).IsValid());
    EXPECT_FALSE((physics::SphereShapeDescriptor{ negativeInfinity }).IsValid());
}

TEST(CapsuleShapeDescriptor, PositiveDimensionsAreValid)
{
    EXPECT_TRUE((physics::CapsuleShapeDescriptor{ 1.0f, 4.0f }).IsValid());
}

TEST(CapsuleShapeDescriptor, ZeroRadiusIsInvalid)
{
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{ 0.0f, 4.0f }).IsValid());
}

TEST(CapsuleShapeDescriptor, ZeroCylinderHeightIsInvalid)
{
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{ 1.0f, 0.0f }).IsValid());
}

TEST(CapsuleShapeDescriptor, NegativeDimensionsAreInvalid)
{
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{ -1.0f, 4.0f }).IsValid());
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{ 1.0f, -4.0f }).IsValid());
}

TEST(CapsuleShapeDescriptor, NonFiniteDimensionsAreInvalid)
{
    const float negativeInfinity = -std::numeric_limits<float>::infinity();

    EXPECT_FALSE((physics::CapsuleShapeDescriptor{
        std::numeric_limits<float>::infinity(), 4.0f }).IsValid());
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{
        1.0f, std::numeric_limits<float>::quiet_NaN() }).IsValid());
    EXPECT_FALSE((physics::CapsuleShapeDescriptor{ 1.0f, negativeInfinity }).IsValid());
}

TEST(CapsuleShapeDescriptor, TotalHeightIncludesCylinderAndCaps)
{
    constexpr physics::CapsuleShapeDescriptor descriptor{ 1.5f, 4.0f };
    static_assert(descriptor.GetTotalHeight() == 7.0f);
    EXPECT_FLOAT_EQ(descriptor.GetTotalHeight(), 7.0f);
}

TEST(PhysicsShapeDescriptor, SupportsBoxSphereAndCapsule)
{
    physics::PhysicsShapeDescriptor descriptor = physics::BoxShapeDescriptor{ { 1.0f, 2.0f, 3.0f } };
    EXPECT_TRUE(std::holds_alternative<physics::BoxShapeDescriptor>(descriptor));

    descriptor = physics::SphereShapeDescriptor{ 2.0f };
    EXPECT_TRUE(std::holds_alternative<physics::SphereShapeDescriptor>(descriptor));

    descriptor = physics::CapsuleShapeDescriptor{ 1.0f, 4.0f };
    EXPECT_TRUE(std::holds_alternative<physics::CapsuleShapeDescriptor>(descriptor));
}

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