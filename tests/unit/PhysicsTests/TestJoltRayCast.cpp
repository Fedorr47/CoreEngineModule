#include <gtest/gtest.h>

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <limits>

import core;

namespace
{
    constexpr float Tolerance = 1.0e-4f;

    [[nodiscard]] physics::PhysicsBodyDescriptor BoxAt(
        const mathUtils::Vec3 position,
        const physics::PhysicsMotionType motionType = physics::PhysicsMotionType::Static)
    {
        return {
            .shape = physics::BoxShapeDescriptor{ .halfExtents = { 1.0f, 1.0f, 1.0f } },
            .transform = { .position = position },
            .motionType = motionType
        };
    }

    [[nodiscard]] physics::PhysicsRayCastRequest DownwardRay()
    {
        return {
            .origin = { 0.0f, 10.0f, 0.0f },
            .direction = { 0.0f, -1.0f, 0.0f },
            .maxDistance = 20.0f
        };
    }

    class JoltRayCastTest : public testing::Test
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

TEST_F(JoltRayCastTest, HitsStaticFloor)
{
    const auto floor = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));

    const auto hit = world.RayCastClosest(DownwardRay());

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, floor);
    EXPECT_NEAR(hit->position.y, 1.0f, Tolerance);
    EXPECT_NEAR(hit->normal.x, 0.0f, Tolerance);
    EXPECT_NEAR(hit->normal.y, 1.0f, Tolerance);
    EXPECT_NEAR(hit->normal.z, 0.0f, Tolerance);
    EXPECT_NEAR(hit->distance, 9.0f, Tolerance);
}

TEST_F(JoltRayCastTest, MissReturnsNullopt)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f })).IsValid());
    auto request = DownwardRay();
    request.direction = { 0.0f, 1.0f, 0.0f };

    EXPECT_FALSE(world.RayCastClosest(request).has_value());
}

TEST_F(JoltRayCastTest, ReturnsClosestBody)
{
    const auto nearer = world.CreateBody(BoxAt({ 0.0f, 5.0f, 0.0f }));
    const auto farther = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));

    const auto hit = world.RayCastClosest(DownwardRay());

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, nearer);
    EXPECT_NE(hit->body, farther);
}

TEST_F(JoltRayCastTest, QueryLayerMaskFiltersBodies)
{
    const auto staticBody = world.CreateBody(BoxAt({ -3.0f, 0.0f, 0.0f }));
    const auto dynamicBody = world.CreateBody(BoxAt(
        { 3.0f, 0.0f, 0.0f }, physics::PhysicsMotionType::Dynamic));
    auto request = DownwardRay();

    request.origin.x = -3.0f;
    request.layerMask = physics::PhysicsQueryLayerMask::StaticWorld;
    auto hit = world.RayCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, staticBody);
    request.layerMask = physics::PhysicsQueryLayerMask::DynamicWorld;
    EXPECT_FALSE(world.RayCastClosest(request).has_value());

    request.origin.x = 3.0f;
    hit = world.RayCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, dynamicBody);
    request.layerMask = physics::PhysicsQueryLayerMask::StaticWorld;
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request.layerMask = physics::PhysicsQueryLayerMask::None;
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
}

TEST_F(JoltRayCastTest, IgnoredBodyIsSkipped)
{
    const auto nearer = world.CreateBody(BoxAt({ 0.0f, 5.0f, 0.0f }));
    const auto farther = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));
    auto request = DownwardRay();
    const auto unfilteredHit = world.RayCastClosest(request);
    ASSERT_TRUE(unfilteredHit.has_value());
    EXPECT_EQ(unfilteredHit->body, nearer);

    request.ignoredBody = nearer;
    const auto hit = world.RayCastClosest(request);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, farther);
}

TEST_F(JoltRayCastTest, StaleIgnoredHandleDoesNotIgnoreReplacement)
{
    const auto stale = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));
    ASSERT_TRUE(world.DestroyBody(stale));
    const auto replacement = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));
    ASSERT_EQ(stale.index, replacement.index);
    ASSERT_NE(stale.generation, replacement.generation);
    auto request = DownwardRay();
    request.ignoredBody = stale;

    const auto hit = world.RayCastClosest(request);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, replacement);
}

TEST_F(JoltRayCastTest, ReturnedHandleIsGenerationSafe)
{
    const auto oldHandle = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));
    ASSERT_TRUE(world.DestroyBody(oldHandle));
    const auto liveHandle = world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f }));

    const auto hit = world.RayCastClosest(DownwardRay());

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, liveHandle);
    EXPECT_EQ(hit->body.index, oldHandle.index);
    EXPECT_NE(hit->body.generation, oldHandle.generation);
}

TEST_F(JoltRayCastTest, InvalidRequestsFailSafely)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f })).IsValid());
    auto request = DownwardRay();
    request.direction = {};
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request = DownwardRay();
    request.origin.x = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request = DownwardRay();
    request.direction.y = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request = DownwardRay();
    request.maxDistance = 0.0f;
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request.maxDistance = -1.0f;
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
    request.maxDistance = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(world.RayCastClosest(request).has_value());
}

TEST_F(JoltRayCastTest, NonNormalizedDirectionWorks)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 0.0f, 0.0f, 0.0f })).IsValid());
    auto request = DownwardRay();
    request.direction = { 0.0f, -25.0f, 0.0f };

    const auto hit = world.RayCastClosest(request);

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->position.y, 1.0f, Tolerance);
    EXPECT_NEAR(hit->distance, 9.0f, Tolerance);
}
