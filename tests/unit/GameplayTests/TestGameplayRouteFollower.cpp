#include <initializer_list>
#include <cmath>
#include <limits>
#include <utility>

#include <gtest/gtest.h>

#include "TestSupport/MathTestHelper.h"
#include "TestSupport/GameplayRouteTestHelper.h"

import core;

using namespace rendern;
using namespace MathTestHelper;
using namespace GameplayRouteTestHelper;

namespace
{
    [[nodiscard]] GameplayRoute OrdinaryRoute(
        const std::initializer_list<GameplayRoutePoint> points)
    {
        GameplayRoute route{};
        route.points.assign(points.begin(), points.end());
        if (!route.points.empty())
        {
            route.segmentAnnotations.resize(route.points.size() - 1u);
        }
        return route;
    }

    void ExpectStationary(const GameplayMovementIntent& movement)
    {
        ExpectVec3Near(movement.moveWorld, mathUtils::Vec3::ZeroVector(), kEpsVec);
        EXPECT_FLOAT_EQ(movement.moveMagnitude, 0.0f);
        EXPECT_FALSE(movement.wantsRun);
        EXPECT_FALSE(movement.IsMoving());
    }
}

TEST(GameplayRouteFollower, DisabledLookAheadPreservesWaypointDirectedMovement)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(follower.Start(OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f),
        MakeRoutePoint(10.0f, 0.0f, 10.0f) }), {}, { .cornerLookAheadDistance = 0.0f }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick({9.0f, 0.0f, 0.0f});
    ExpectVec3Near(output.movement.moveWorld, {1.0f, 0.0f, 0.0f}, kEpsVec);
}

TEST(GameplayRouteFollower, LookAheadBendsProgressivelyNearOrdinaryCorner)
{
    const GameplayRoute route = OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f),
        MakeRoutePoint(10.0f, 0.0f, 10.0f) });
    GameplayRouteFollower fartherFollower{};
    GameplayRouteFollower closerFollower{};
    ASSERT_EQ(fartherFollower.Start(route, {}, { .cornerLookAheadDistance = 2.0f }), GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(closerFollower.Start(
        route,
        GameplayArrivalSteeringSettings{ .minimumMoveMagnitude = 0.25f, .wantsRun = true },
        { .cornerLookAheadDistance = 2.0f }), GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput far = fartherFollower.Tick({0.0f, 0.0f, 0.0f});
    const GameplayRouteFollowerOutput near = closerFollower.Tick({9.0f, 0.0f, 0.0f});
    ExpectVec3Near(far.movement.moveWorld, {1.0f, 0.0f, 0.0f}, kEpsVec);
    EXPECT_GT(near.movement.moveWorld.x, 0.0f);
    EXPECT_GT(near.movement.moveWorld.z, 0.0f);
    EXPECT_GT(near.movement.moveWorld.z, far.movement.moveWorld.z);
    EXPECT_NEAR(mathUtils::Length(near.movement.moveWorld), 1.0f, kTolerance);
    EXPECT_TRUE(std::isfinite(near.movement.moveWorld.x));
    EXPECT_TRUE(std::isfinite(near.movement.moveWorld.z));
    EXPECT_FLOAT_EQ(near.movement.moveMagnitude, 1.0f);
    EXPECT_TRUE(near.movement.wantsRun);
}

TEST(GameplayRouteFollower, LookAheadPassThroughAdvancesAtMostOneCornerPerTick)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(follower.Start(OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f),
        MakeRoutePoint(2.0f, 0.0f, 0.0f), MakeRoutePoint(3.0f, 0.0f, 0.0f) }),
        GameplayArrivalSteeringSettings{ .acceptanceRadius = 0.1f },
        { .cornerLookAheadDistance = 10.0f }), GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick({3.0f, 0.0f, 0.0f});

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    EXPECT_TRUE(output.movement.IsMoving());
    ExpectVec3Near(output.movement.moveWorld, {-1.0f, 0.0f, 0.0f}, kEpsVec);
}

TEST(GameplayRouteFollower, LookAheadDoesNotCrossTraversalBoundary)
{
    const GameplayTraversalLinkHandle link = MakeTraversalLink(42u);
    GameplayRouteFollower follower{};
    ASSERT_EQ(follower.Start(GameplayRoute{
        .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 10.0f) },
        .segmentAnnotations = { GameplayRouteSegmentAnnotation{}, GameplayRouteSegmentAnnotation{ .traversalLink = link } }
    }, {}, { .cornerLookAheadDistance = 2.0f }), GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput approaching = follower.Tick({9.0f, 0.0f, 0.0f});
    ExpectVec3Near(approaching.movement.moveWorld, {1.0f, 0.0f, 0.0f}, kEpsVec);
    const GameplayRouteFollowerOutput pending = follower.Tick({10.0f, 0.0f, 0.0f});
    EXPECT_EQ(pending.status, GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(pending.requiredTraversalLink.has_value());
    EXPECT_EQ(*pending.requiredTraversalLink, link);
}

TEST(GameplayRouteFollower, BoundedPassThroughAdvancesCornerCutWithoutSkippingFarCorner)
{
    const GameplayRoute route = OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f),
        MakeRoutePoint(10.0f, 0.0f, 10.0f) });
    const GameplayArrivalSteeringSettings steering{ .acceptanceRadius = 0.1f };
    GameplayRouteFollower nearFollower{};
    GameplayRouteFollower farFollower{};
    ASSERT_EQ(nearFollower.Start(route, steering, { .cornerLookAheadDistance = 2.0f }), GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(farFollower.Start(route, steering, { .cornerLookAheadDistance = 2.0f }), GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput passed = nearFollower.Tick({10.1f, 0.0f, 1.0f});
    EXPECT_LT(passed.movement.moveWorld.x, 0.0f);
    EXPECT_GT(passed.movement.moveWorld.z, 0.0f);
    const GameplayRouteFollowerOutput far = farFollower.Tick({20.0f, 0.0f, 1.0f});
    EXPECT_LT(far.movement.moveWorld.x, 0.0f);
    EXPECT_LT(far.movement.moveWorld.z, 0.0f);
}

TEST(GameplayRouteFollower, ShortOutgoingSegmentClampsVirtualTarget)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(follower.Start(OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f),
        MakeRoutePoint(10.0f, 0.0f, 0.5f) }), {}, { .cornerLookAheadDistance = 4.0f }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick({9.0f, 0.0f, 0.0f});
    EXPECT_NEAR(output.movement.moveWorld.x / output.movement.moveWorld.z, 2.0f, kTolerance);
    EXPECT_TRUE(std::isfinite(output.movement.moveWorld.z));
}

TEST(GameplayRouteFollower, MalformedLookAheadSettingsDisableLookAhead)
{
    const GameplayRoute route = OrdinaryRoute({
        MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f),
        MakeRoutePoint(10.0f, 0.0f, 10.0f) });
    for (const float malformed : {-1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
    {
        GameplayRouteFollower follower{};
        ASSERT_EQ(follower.Start(route, {}, { .cornerLookAheadDistance = malformed }), GameplayRouteFollowerStatus::Following);
        const GameplayRouteFollowerOutput output = follower.Tick({9.0f, 0.0f, 0.0f});
        ExpectVec3Near(output.movement.moveWorld, {1.0f, 0.0f, 0.0f}, kEpsVec);
    }
}

// Protects the default lifecycle state so an unstarted follower cannot accidentally move an agent.
TEST(GameplayRouteFollower, DefaultFollowerIsNotStartedAndStationary)
{
    const GameplayRouteFollower follower{};

    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::NotStarted);
}

// Protects pre-start ticks as deterministic no-ops for callers that poll before a route is assigned.
TEST(GameplayRouteFollower, TickBeforeStartRemainsStationary)
{
    GameplayRouteFollower follower{};

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::NotStarted);
    ExpectStationary(output.movement);
    EXPECT_FALSE(output.requiredTraversalLink.has_value());
}

// Protects route validation handoff so malformed manually constructed routes cannot be followed.
TEST(GameplayRouteFollower, InvalidRouteEntersInvalidRoute)
{
    GameplayRouteFollower follower{};
    GameplayRoute invalidRoute{
        .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) }
    };

    EXPECT_EQ(follower.Start(invalidRoute), GameplayRouteFollowerStatus::InvalidRoute);
    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::InvalidRoute);
    ExpectStationary(output.movement);
    EXPECT_FALSE(output.requiredTraversalLink.has_value());
}

// Protects the valid empty-route no-op contract used when no travel is required.
TEST(GameplayRouteFollower, EmptyRouteSucceedsOnStart)
{
    GameplayRouteFollower follower{};

    EXPECT_EQ(follower.Start(GameplayRoute{}), GameplayRouteFollowerStatus::Succeeded);
    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::Succeeded);
}

// Protects routes where the agent is already anchored at the only authored point.
TEST(GameplayRouteFollower, OnePointRouteSucceedsOnStart)
{
    GameplayRouteFollower follower{};

    EXPECT_EQ(
        follower.Start(GameplayRoute{ .points = { MakeRoutePoint(1.0f, 0.0f, 2.0f) } }),
        GameplayRouteFollowerStatus::Succeeded);
}

// Protects the normal lifecycle transition for the smallest route that contains a segment.
TEST(GameplayRouteFollower, ValidTwoPointRouteEntersFollowing)
{
    GameplayRouteFollower follower{};

    EXPECT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);
}

// Protects use of arrival steering by verifying the observable movement direction toward the segment target.
TEST(GameplayRouteFollower, OrdinarySegmentProducesMovementTowardTarget)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(3.0f, 0.0f, 4.0f) })),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    EXPECT_NEAR(output.movement.moveWorld.x, 0.6f, kTolerance);
    EXPECT_FLOAT_EQ(output.movement.moveWorld.y, 0.0f);
    EXPECT_NEAR(output.movement.moveWorld.z, 0.8f, kTolerance);
    EXPECT_FLOAT_EQ(output.movement.moveMagnitude, 1.0f);
    EXPECT_FALSE(output.requiredTraversalLink.has_value());
}

// Protects forwarding of steering settings rather than route follower-specific movement policy.
TEST(GameplayRouteFollower, OrdinarySegmentPropagatesWantsRun)
{
    GameplayRouteFollower follower{};
    const GameplayArrivalSteeringSettings settings{ .wantsRun = true };
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 0.0f) }), settings),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_TRUE(output.movement.wantsRun);
}

// Protects slowdown output from the shared steering helper so the follower does not duplicate speed math.
TEST(GameplayRouteFollower, OrdinarySegmentUsesSlowdownMagnitudeFromSteering)
{
    GameplayRouteFollower follower{};
    const GameplayArrivalSteeringSettings settings{ .acceptanceRadius = 1.0f, .slowingRadius = 5.0f };
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(3.0f, 0.0f, 0.0f) }), settings),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_NEAR(output.movement.moveMagnitude, 0.5f, kTolerance);
}

// Protects final ordinary arrival so completion is reported without an extra frame.
TEST(GameplayRouteFollower, ArrivalAtFinalOrdinaryPointSucceeds)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Succeeded);
    ExpectStationary(output.movement);
}

// Protects ordered target progression so later route segments are not surfaced before earlier ones complete.
TEST(GameplayRouteFollower, MultiSegmentRouteFollowsTargetsInOrder)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 3.0f) })),
        GameplayRouteFollowerStatus::Following);

    EXPECT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::Following);
    const GameplayRouteFollowerOutput second = follower.Tick(mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });

    EXPECT_EQ(second.status, GameplayRouteFollowerStatus::Following);
    ExpectVec3Near(second.movement.moveWorld, mathUtils::Vec3{ 0.0f, 0.0f, 1.0f }, kEpsVec);
}

// Protects same-tick advancement across several already-reached ordinary segments.
TEST(GameplayRouteFollower, MultipleAlreadyArrivedSegmentsAdvanceInOneTick)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(0.1f, 0.0f, 0.0f),
            MakeRoutePoint(0.2f, 0.0f, 0.0f),
            MakeRoutePoint(2.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    ExpectVec3Near(output.movement.moveWorld, mathUtils::Vec3{ 1.0f, 0.0f, 0.0f }, kEpsVec);
}

// Protects zero-length segment handling so repeated authored points cannot stall progression forever.
TEST(GameplayRouteFollower, RepeatedConsecutivePointsDoNotStall)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    EXPECT_TRUE(output.movement.IsMoving());
}

// Protects traversal boundaries so annotated segments stop ordinary steering immediately.
TEST(GameplayRouteFollower, TraversalLinkSegmentReportsTraversalRequired)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(7u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link } } }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(output.requiredTraversalLink.has_value());
    EXPECT_EQ(*output.requiredTraversalLink, link);
}

// Protects exact handle propagation so an external traversal system can resolve the required generic link.
TEST(GameplayRouteFollower, TraversalLinkOutputContainsExactHandle)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(99u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link } } }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 100.0f, 0.0f, 0.0f });

    ASSERT_TRUE(output.requiredTraversalLink.has_value());
    EXPECT_EQ(output.requiredTraversalLink->value, 99u);
}

// Protects traversal pause output so no movement leaks while a separate system owns the segment.
TEST(GameplayRouteFollower, TraversalLinkOutputContainsNoMovement)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(10.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = MakeTraversalLink(1u) } } }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    ExpectStationary(output.movement);
}

// Protects stable pending traversal state so repeated polling cannot advance a route implicitly.
TEST(GameplayRouteFollower, RepeatedPendingTraversalTicksRemainStable)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(11u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link } } }),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput first = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });
    const GameplayRouteFollowerOutput second = follower.Tick(mathUtils::Vec3{ 10.0f, 0.0f, 0.0f });

    EXPECT_EQ(first.status, GameplayRouteFollowerStatus::TraversalRequired);
    EXPECT_EQ(second.status, GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(second.requiredTraversalLink.has_value());
    EXPECT_EQ(*second.requiredTraversalLink, link);
}

// Protects matching completion as the only boundary that advances past a traversal segment.
TEST(GameplayRouteFollower, MatchingTraversalCompletionAdvances)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(12u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f), MakeRoutePoint(3.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link }, GameplayRouteSegmentAnnotation{} } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);

    EXPECT_TRUE(follower.CompleteTraversal(link));
    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    ExpectVec3Near(output.movement.moveWorld, mathUtils::Vec3{ 1.0f, 0.0f, 0.0f }, kEpsVec);
}

// Protects final traversal completion so terminal annotated routes complete without ordinary steering.
TEST(GameplayRouteFollower, FinalTraversalCompletionSucceedsRoute)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(13u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link } } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);

    EXPECT_TRUE(follower.CompleteTraversal(link));
    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::Succeeded);
}

// Protects pending traversal identity so an unrelated valid link cannot advance the route.
TEST(GameplayRouteFollower, MismatchedTraversalCompletionIsRejected)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(14u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link } } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);

    EXPECT_FALSE(follower.CompleteTraversal(MakeTraversalLink(15u)));
    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::TraversalRequired);
}

// Protects completion validation so the reserved invalid handle is never accepted as traversal success.
TEST(GameplayRouteFollower, InvalidTraversalCompletionHandleIsRejected)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = MakeTraversalLink(16u) } } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);

    EXPECT_FALSE(follower.CompleteTraversal(GameplayTraversalLinkHandle{}));
    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::TraversalRequired);
}

// Protects ordinary segments and terminal states from being advanced by traversal completion calls.
TEST(GameplayRouteFollower, CompletionWithoutPendingTraversalIsRejected)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);

    EXPECT_FALSE(follower.CompleteTraversal(MakeTraversalLink(17u)));
    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::Following);
}

// Protects resumption from traversal into ordinary steering without route re-anchoring or skipped segments.
TEST(GameplayRouteFollower, TraversalFollowedByOrdinaryMovementResumesCorrectly)
{
    GameplayRouteFollower follower{};
    const GameplayTraversalLinkHandle link = MakeTraversalLink(18u);
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 2.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = link }, GameplayRouteSegmentAnnotation{} } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(follower.CompleteTraversal(link));

    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    ExpectVec3Near(output.movement.moveWorld, mathUtils::Vec3{ 0.0f, 0.0f, 1.0f }, kEpsVec);
}

// Protects consecutive traversal links so each annotated segment requires its own explicit completion.
TEST(GameplayRouteFollower, ConsecutiveTraversalLinksAreSurfacedOneAtATime)
{
    GameplayRouteFollower follower{};

    const GameplayTraversalLinkHandle first = MakeTraversalLink(19u);
    const GameplayTraversalLinkHandle second = MakeTraversalLink(20u);

    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f),
            MakeRoutePoint(2.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{
                .traversalLink = first
            },
            GameplayRouteSegmentAnnotation{
                .traversalLink = second
            }
        }
    };

    ASSERT_EQ(
        follower.Start(route),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput firstOutput =
        follower.Tick(mathUtils::Vec3{0.0f, 0.0f, 0.0f});

    ASSERT_EQ(
        firstOutput.status,
        GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(firstOutput.requiredTraversalLink.has_value());
    EXPECT_EQ(*firstOutput.requiredTraversalLink, first);

    ASSERT_TRUE(follower.CompleteTraversal(first));
    EXPECT_EQ(
        follower.GetStatus(),
        GameplayRouteFollowerStatus::Following);

    const GameplayRouteFollowerOutput secondOutput =
        follower.Tick(mathUtils::Vec3{1.0f, 0.0f, 0.0f});

    ASSERT_EQ(
        secondOutput.status,
        GameplayRouteFollowerStatus::TraversalRequired);
    ASSERT_TRUE(secondOutput.requiredTraversalLink.has_value());
    EXPECT_EQ(*secondOutput.requiredTraversalLink, second);

    EXPECT_FALSE(follower.CompleteTraversal(first));
    EXPECT_EQ(
        follower.GetStatus(),
        GameplayRouteFollowerStatus::TraversalRequired);

    ASSERT_TRUE(follower.CompleteTraversal(second));
    EXPECT_EQ(
        follower.GetStatus(),
        GameplayRouteFollowerStatus::Succeeded);
}

// Protects reset as a full lifecycle clear that discards pending traversal and active route state.
TEST(GameplayRouteFollower, ResetClearsActiveState)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(GameplayRoute{
            .points = { MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(1.0f, 0.0f, 0.0f) },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{ .traversalLink = MakeTraversalLink(21u) } } }),
        GameplayRouteFollowerStatus::Following);
    ASSERT_EQ(follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }).status, GameplayRouteFollowerStatus::TraversalRequired);

    follower.Reset();
    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(follower.GetStatus(), GameplayRouteFollowerStatus::NotStarted);
    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::NotStarted);
    ExpectStationary(output.movement);
    EXPECT_FALSE(output.requiredTraversalLink.has_value());
}

// Protects callers from accidentally replacing an active route by calling Start twice without Reset.
TEST(GameplayRouteFollower, RepeatedStartWithoutResetDoesNotReplaceActiveRoute)
{
    GameplayRouteFollower follower{};
    ASSERT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 0.0f) })),
        GameplayRouteFollowerStatus::Following);

    EXPECT_EQ(
        follower.Start(OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(0.0f, 0.0f, 5.0f) })),
        GameplayRouteFollowerStatus::Following);
    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    ExpectVec3Near(output.movement.moveWorld, mathUtils::Vec3{ 1.0f, 0.0f, 0.0f }, kEpsVec);
}

// Protects route ownership by value so follower progress is independent of the caller's moved-from route object.
TEST(GameplayRouteFollower, FollowerOwnedRouteRemainsValidAfterSourceRouteIsMovedFrom)
{
    GameplayRouteFollower follower{};
    GameplayRoute route = OrdinaryRoute({ MakeRoutePoint(0.0f, 0.0f, 0.0f), MakeRoutePoint(0.0f, 0.0f, 3.0f) });

    ASSERT_EQ(follower.Start(std::move(route)), GameplayRouteFollowerStatus::Following);
    route = GameplayRoute{};
    const GameplayRouteFollowerOutput output = follower.Tick(mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });

    EXPECT_EQ(output.status, GameplayRouteFollowerStatus::Following);
    ExpectVec3Near(output.movement.moveWorld, mathUtils::Vec3{ 0.0f, 0.0f, 1.0f }, kEpsVec);
}
