#include <gtest/gtest.h>

#include "Physics/Jolt/JoltRuntime.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

import core;

class JoltShapeFactoryTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(runtime.Initialize());
    }

    physics::JoltRuntime runtime;
};

TEST_F(JoltShapeFactoryTest, CreatesBoxShape)
{
    const auto result = physics::jolt::CreateShape(
        physics::BoxShapeDescriptor{ { 1.0f, 2.0f, 3.0f } });

    ASSERT_TRUE(result.has_value()) << result.error();
    ASSERT_NE(result->GetPtr(), nullptr);
    ASSERT_EQ((*result)->GetSubType(), JPH::EShapeSubType::Box);
    const auto* box = static_cast<const JPH::BoxShape*>(result->GetPtr());
    EXPECT_EQ(box->GetHalfExtent(), JPH::Vec3(1.0f, 2.0f, 3.0f));
}

TEST_F(JoltShapeFactoryTest, CreatesSmallPositiveBox)
{
    const auto result = physics::jolt::CreateShape(
        physics::BoxShapeDescriptor{ { 0.01f, 0.02f, 0.03f } });

    ASSERT_TRUE(result.has_value()) << result.error();
    ASSERT_NE(result->GetPtr(), nullptr);
    ASSERT_EQ((*result)->GetSubType(), JPH::EShapeSubType::Box);
    const auto* box = static_cast<const JPH::BoxShape*>(result->GetPtr());
    EXPECT_EQ(box->GetHalfExtent(), JPH::Vec3(0.01f, 0.02f, 0.03f));
}

TEST_F(JoltShapeFactoryTest, CreatesSphereShape)
{
    const auto result = physics::jolt::CreateShape(physics::SphereShapeDescriptor{ 2.5f });

    ASSERT_TRUE(result.has_value()) << result.error();
    ASSERT_NE(result->GetPtr(), nullptr);
    ASSERT_EQ((*result)->GetSubType(), JPH::EShapeSubType::Sphere);
    const auto* sphere = static_cast<const JPH::SphereShape*>(result->GetPtr());
    EXPECT_FLOAT_EQ(sphere->GetRadius(), 2.5f);
}

TEST_F(JoltShapeFactoryTest, CreatesCapsuleShape)
{
    const auto result = physics::jolt::CreateShape(physics::CapsuleShapeDescriptor{ 1.5f, 6.0f });

    ASSERT_TRUE(result.has_value()) << result.error();
    ASSERT_NE(result->GetPtr(), nullptr);
    ASSERT_EQ((*result)->GetSubType(), JPH::EShapeSubType::Capsule);
    const auto* capsule = static_cast<const JPH::CapsuleShape*>(result->GetPtr());
    EXPECT_FLOAT_EQ(capsule->GetRadius(), 1.5f);
    EXPECT_FLOAT_EQ(capsule->GetHalfHeightOfCylinder(), 3.0f);
}

TEST_F(JoltShapeFactoryTest, RejectsInvalidBox)
{
    const auto result = physics::jolt::CreateShape(physics::BoxShapeDescriptor{});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Box shape half extents must be finite and greater than zero.");
}

TEST_F(JoltShapeFactoryTest, RejectsInvalidSphere)
{
    const auto result = physics::jolt::CreateShape(physics::SphereShapeDescriptor{});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Sphere shape radius must be finite and greater than zero.");
}

TEST_F(JoltShapeFactoryTest, RejectsInvalidCapsule)
{
    const auto result = physics::jolt::CreateShape(physics::CapsuleShapeDescriptor{});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
        result.error(),
        "Capsule shape radius and cylinder height must be finite and greater than zero.");
}