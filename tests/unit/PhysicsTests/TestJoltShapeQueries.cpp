#include <gtest/gtest.h>

#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <algorithm>
#include <limits>

import core;

namespace
{
    constexpr float Tolerance = 1.0e-3f;

    physics::PhysicsBodyDescriptor BoxAt(
        mathUtils::Vec3 position,
        physics::PhysicsMotionType motion = physics::PhysicsMotionType::Static)
    {
        return { .shape = physics::BoxShapeDescriptor{ .halfExtents = { 1, 1, 1 } },
            .transform = { .position = position }, .motionType = motion };
    }

    physics::PhysicsShapeCastRequest CapsuleCast(float distance = 10.0f)
    {
        return { .shape = physics::CapsuleShapeDescriptor{ .radius = 0.5f, .cylinderHeight = 1.0f },
            .startTransform = { .position = { 0, 0, 0 } }, .direction = { 1, 0, 0 },
            .maxDistance = distance };
    }

    physics::PhysicsOverlapRequest SphereOverlap(mathUtils::Vec3 position = {})
    {
        return { .shape = physics::SphereShapeDescriptor{ .radius = 1.25f },
            .transform = { .position = position } };
    }

    bool Contains(const std::vector<physics::PhysicsBodyHandle>& values, physics::PhysicsBodyHandle value)
    {
        return std::ranges::find(values, value) != values.end();
    }

    class JoltShapeQueriesTest : public testing::Test
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
            world.Shutdown(); runtime.Shutdown(); threadAffinity::ResetOwnerThreadRegistry();
        }
        physics::JoltRuntime runtime;
        physics::JoltPhysicsWorld world{ runtime };
    };
}

TEST_F(JoltShapeQueriesTest, CapsuleCastHitsWall)
{
    const auto wall = world.CreateBody(BoxAt({ 5, 0, 0 }));
    const auto hit = world.ShapeCastClosest(CapsuleCast());
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, wall);
    EXPECT_NEAR(hit->distance, 3.5f, Tolerance);
    EXPECT_NEAR(hit->position.x, 4.0f, Tolerance);
    EXPECT_LT(hit->normal.x, -0.99f);
}

TEST_F(JoltShapeQueriesTest, ShapeCastMissReturnsNullopt)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 5, 0, 0 })).IsValid());
    auto request = CapsuleCast(); request.direction = { -1, 0, 0 };
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
}

TEST_F(JoltShapeQueriesTest, ShapeCastReturnsClosestBody)
{
    const auto nearBody = world.CreateBody(BoxAt({ 5, 0, 0 }));
    ASSERT_TRUE(world.CreateBody(BoxAt({ 9, 0, 0 })).IsValid());
    const auto hit = world.ShapeCastClosest(CapsuleCast());
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, nearBody);
}

TEST_F(JoltShapeQueriesTest, ShapeCastSupportsNonNormalizedDirection)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 5, 0, 0 })).IsValid());
    auto request = CapsuleCast(); request.direction = { 25, 0, 0 };
    const auto hit = world.ShapeCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 3.5f, Tolerance);
}

TEST_F(JoltShapeQueriesTest, ShapeCastRespectsMaxDistance)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 5, 0, 0 })).IsValid());
    EXPECT_FALSE(world.ShapeCastClosest(CapsuleCast(3.0f)).has_value());
}

TEST_F(JoltShapeQueriesTest, ShapeCastLayerFiltering)
{
    const auto dynamicBody = world.CreateBody(BoxAt({ 5, 0, 0 }, physics::PhysicsMotionType::Dynamic));
    auto request = CapsuleCast(); request.layerMask = physics::PhysicsQueryLayerMask::StaticWorld;
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
    request.layerMask = physics::PhysicsQueryLayerMask::DynamicWorld;
    const auto hit = world.ShapeCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, dynamicBody);
}

TEST_F(JoltShapeQueriesTest, ShapeCastIgnoredBody)
{
    const auto nearBody = world.CreateBody(BoxAt({ 5, 0, 0 }));
    const auto farBody = world.CreateBody(BoxAt({ 9, 0, 0 }));
    auto request = CapsuleCast(); request.ignoredBody = nearBody;
    const auto hit = world.ShapeCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, farBody);
}

TEST_F(JoltShapeQueriesTest, ShapeCastStaleIgnoredHandle)
{
    const auto stale = world.CreateBody(BoxAt({ 5, 0, 0 }));
    ASSERT_TRUE(world.DestroyBody(stale));
    const auto replacement = world.CreateBody(BoxAt({ 5, 0, 0 }));
    auto request = CapsuleCast(); request.ignoredBody = stale;
    const auto hit = world.ShapeCastClosest(request);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->body, replacement);
}

TEST_F(JoltShapeQueriesTest, ShapeCastSupportsEveryCurrentShapeDescriptor)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({ 5, 0, 0 })).IsValid());
    auto request = CapsuleCast();
    request.shape = physics::SphereShapeDescriptor{ .radius = 0.5f };
    EXPECT_TRUE(world.ShapeCastClosest(request).has_value());
    request.shape = physics::BoxShapeDescriptor{ .halfExtents = { 0.5f, 0.5f, 0.5f } };
    EXPECT_TRUE(world.ShapeCastClosest(request).has_value());
}

TEST_F(JoltShapeQueriesTest, InvalidShapeCastFailsSafely)
{
    auto request = CapsuleCast(); request.direction = {};
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
    request = CapsuleCast(); request.startTransform.position.x = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
    request = CapsuleCast(); request.shape = physics::SphereShapeDescriptor{};
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
    request = CapsuleCast(); request.layerMask = physics::PhysicsQueryLayerMask::None;
    EXPECT_FALSE(world.ShapeCastClosest(request).has_value());
}

TEST_F(JoltShapeQueriesTest, OverlapFindsBodyInsideVolume)
{
    const auto body = world.CreateBody(BoxAt({}));
    const auto hits = world.OverlapShape(SphereOverlap());
    EXPECT_TRUE(Contains(hits, body));
}

TEST_F(JoltShapeQueriesTest, OverlapMissReturnsEmpty)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({})).IsValid());
    EXPECT_TRUE(world.OverlapShape(SphereOverlap({ 10, 0, 0 })).empty());
}

TEST_F(JoltShapeQueriesTest, OverlapReturnsMultipleUniqueBodies)
{
    const auto first = world.CreateBody(BoxAt({ -1, 0, 0 }));
    const auto second = world.CreateBody(BoxAt({ 1, 0, 0 }));
    auto request = SphereOverlap(); std::get<physics::SphereShapeDescriptor>(request.shape).radius = 2.0f;
    const auto hits = world.OverlapShape(request);
    EXPECT_TRUE(Contains(hits, first)); EXPECT_TRUE(Contains(hits, second));
    EXPECT_EQ(std::ranges::count(hits, first), 1); EXPECT_EQ(std::ranges::count(hits, second), 1);
}

TEST_F(JoltShapeQueriesTest, OverlapLayerFiltering)
{
    const auto dynamicBody = world.CreateBody(BoxAt({}, physics::PhysicsMotionType::Dynamic));
    auto request = SphereOverlap(); request.layerMask = physics::PhysicsQueryLayerMask::StaticWorld;
    EXPECT_TRUE(world.OverlapShape(request).empty());
    request.layerMask = physics::PhysicsQueryLayerMask::DynamicWorld;
    EXPECT_TRUE(Contains(world.OverlapShape(request), dynamicBody));
}

TEST_F(JoltShapeQueriesTest, OverlapIgnoredBody)
{
    const auto ignored = world.CreateBody(BoxAt({ -1, 0, 0 }));
    const auto other = world.CreateBody(BoxAt({ 1, 0, 0 }));
    auto request = SphereOverlap(); request.ignoredBody = ignored;
    const auto hits = world.OverlapShape(request);
    EXPECT_FALSE(Contains(hits, ignored)); EXPECT_TRUE(Contains(hits, other));
}

TEST_F(JoltShapeQueriesTest, OverlapStaleIgnoredHandle)
{
    const auto stale = world.CreateBody(BoxAt({})); ASSERT_TRUE(world.DestroyBody(stale));
    const auto replacement = world.CreateBody(BoxAt({}));
    auto request = SphereOverlap(); request.ignoredBody = stale;
    EXPECT_TRUE(Contains(world.OverlapShape(request), replacement));
}

TEST_F(JoltShapeQueriesTest, InvalidOverlapReturnsEmpty)
{
    auto request = SphereOverlap();
    request.layerMask = physics::PhysicsQueryLayerMask::None;
    EXPECT_TRUE(world.OverlapShape(request).empty());

    request = SphereOverlap();
    request.shape = physics::SphereShapeDescriptor{};
    EXPECT_TRUE(world.OverlapShape(request).empty());

    request = SphereOverlap();
    request.transform.rotationQuaternion = {};
    EXPECT_TRUE(world.OverlapShape(request).empty());
}

TEST_F(JoltShapeQueriesTest, PlacementIsBlockedByGeometryAndFreeInEmptySpace)
{
    ASSERT_TRUE(world.CreateBody(BoxAt({})).IsValid());
    EXPECT_FALSE(world.CanPlaceShape(SphereOverlap()));
    EXPECT_TRUE(world.CanPlaceShape(SphereOverlap({ 10, 0, 0 })));
}

TEST_F(JoltShapeQueriesTest, PlacementRespectsIgnoredBodyAndLayerMask)
{
    const auto body = world.CreateBody(BoxAt({}));
    auto request = SphereOverlap(); request.ignoredBody = body;
    EXPECT_TRUE(world.CanPlaceShape(request));
    request.ignoredBody = {}; request.layerMask = physics::PhysicsQueryLayerMask::DynamicWorld;
    EXPECT_TRUE(world.CanPlaceShape(request));
}

TEST_F(JoltShapeQueriesTest, InvalidPlacementRequestFailsClosed)
{
    auto request = SphereOverlap(); request.layerMask = physics::PhysicsQueryLayerMask::None;
    EXPECT_FALSE(world.CanPlaceShape(request));
    request = SphereOverlap(); request.shape = physics::SphereShapeDescriptor{};
    EXPECT_FALSE(world.CanPlaceShape(request));
    request = SphereOverlap(); request.transform.rotationQuaternion = {};
    EXPECT_FALSE(world.CanPlaceShape(request));
}