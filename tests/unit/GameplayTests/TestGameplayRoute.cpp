#include <limits>

#include <gtest/gtest.h>

#include "TestSupport/GameplayRouteTestHelper.h"

import core;

using namespace rendern;
using namespace GameplayRouteTestHelper;

// Protects the stationary empty-route contract so future followers can treat
// missing route data as a valid no-op rather than as malformed input.
TEST(GameplayRoute, DefaultRouteIsEmptyAndValid)
{
    const GameplayRoute route{};

    EXPECT_TRUE(route.IsEmpty());
    EXPECT_TRUE(route.IsValid());
    EXPECT_TRUE(route.points.empty());
    EXPECT_TRUE(route.segmentAnnotations.empty());
}

// Protects single-position routes used when an agent is already at its target
// and no traversal segment needs to be executed.
TEST(GameplayRoute, OnePointRouteIsValidWithoutAnnotations)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(1.0f, 2.0f, 3.0f)
        }
    };

    EXPECT_FALSE(route.IsEmpty());
    EXPECT_TRUE(route.IsValid());
}

// Protects the basic point-to-segment mapping required by route following:
// one annotation describes travel between two adjacent points.
TEST(GameplayRoute, TwoPointsWithOrdinarySegmentAreValid)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{}
        }
    };

    EXPECT_TRUE(route.IsValid());
}

// Protects ordered multi-segment routes so route search can combine ordinary
// movement and externally resolved traversal links without redundant indices.
TEST(GameplayRoute, MultiplePointsWithMatchingAnnotationsAreValid)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f),
            MakeRoutePoint(2.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{},
            GameplayRouteSegmentAnnotation{
                .traversalLink = MakeTraversalLink(42u)
            }
        }
    };

    EXPECT_TRUE(route.IsValid());
}

// Protects structural validation against under-described routes where a future
// follower would have no traversal contract for one of the route segments.
TEST(GameplayRoute, MissingSegmentAnnotationInvalidatesRoute)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f)
        }
    };

    EXPECT_FALSE(route.IsValid());
}

// Protects structural validation against stale annotations that no longer map
// to a pair of adjacent route points.
TEST(GameplayRoute, ExtraSegmentAnnotationInvalidatesRoute)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{}
        }
    };

    EXPECT_FALSE(route.IsValid());
}

// Protects the empty-route invariant so segment annotations cannot imply hidden
// route points or runtime state outside the explicit route data.
TEST(GameplayRoute, AnnotationOnEmptyRouteInvalidatesRoute)
{
    const GameplayRoute route{
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{}
        }
    };

    EXPECT_FALSE(route.IsEmpty());
    EXPECT_FALSE(route.IsValid());
}

// Protects ordinary movement semantics where the absence of a traversal link
// means the segment can be handled directly by normal steering.
TEST(GameplayRoute, AnnotationWithoutTraversalLinkIsValid)
{
    const GameplayRouteSegmentAnnotation annotation{};

    EXPECT_TRUE(annotation.IsValid());
    EXPECT_FALSE(annotation.traversalLink.has_value());
}

// Protects the generic traversal boundary so a valid link can mark a segment
// for resolution by an external traversal system.
TEST(GameplayRoute, AnnotationWithMakeTraversalLinkIsValid)
{
    const GameplayRouteSegmentAnnotation annotation{
        .traversalLink = MakeTraversalLink(42u)
    };

    EXPECT_TRUE(annotation.IsValid());
    ASSERT_TRUE(annotation.traversalLink.has_value());
    EXPECT_TRUE(annotation.traversalLink->IsValid());
}

// Protects annotation validation from accepting a present but invalid traversal
// link, which is different from an intentionally absent traversal link.
TEST(GameplayRoute, AnnotationWithInMakeTraversalLinkIsInvalid)
{
    const GameplayRouteSegmentAnnotation annotation{
        .traversalLink = GameplayTraversalLinkHandle{}
    };

    EXPECT_FALSE(annotation.IsValid());
    ASSERT_TRUE(annotation.traversalLink.has_value());
    EXPECT_FALSE(annotation.traversalLink->IsValid());
}

// Protects default handle construction so an uninitialized traversal reference
// cannot accidentally resolve to a real traversal entry.
TEST(GameplayRoute, DefaultTraversalLinkHandleIsInvalid)
{
    const GameplayTraversalLinkHandle handle{};

    EXPECT_FALSE(handle.IsValid());
    EXPECT_EQ(
        handle.value,
        GameplayTraversalLinkHandle::InvalidValue);
}

// Protects explicit handle construction so valid registry identifiers remain
// distinguishable from the reserved invalid sentinel value.
TEST(GameplayRoute, ExplicitTraversalLinkHandleIsValid)
{
    const GameplayTraversalLinkHandle handle{ 42u };

    EXPECT_TRUE(handle.IsValid());
    EXPECT_EQ(handle.value, 42u);
}

// Protects value semantics required for route storage, lookup, and comparison
// without introducing pointer ownership into the route model.
TEST(GameplayRoute, TraversalLinkHandlesCompareByValue)
{
    const GameplayTraversalLinkHandle first{ 42u };
    const GameplayTraversalLinkHandle same{ 42u };
    const GameplayTraversalLinkHandle different{ 7u };

    EXPECT_EQ(first, same);
    EXPECT_NE(first, different);
}

// Protects route-level validation so a malformed traversal annotation cannot
// reach a future follower even when point and segment counts are correct.
TEST(GameplayRoute, InMakeTraversalLinkInvalidatesRoute)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{
                .traversalLink = GameplayTraversalLinkHandle{}
            }
        }
    };

    EXPECT_FALSE(route.IsValid());
}

// Protects point validation against NaN coordinates in every axis while keeping
// the route model independent of scene, physics, and runtime state.
TEST(GameplayRoute, NaNInAnyPointCoordinateInvalidatesRoute)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(nan, 0.0f, 0.0f)
        }
    }.IsValid());

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(0.0f, nan, 0.0f)
        }
    }.IsValid());

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, nan)
        }
    }.IsValid());
}

// Protects point validation against infinite coordinates so route consumers do
// not need their own guards against invalid world-space input.
TEST(GameplayRoute, InfinityInAnyPointCoordinateInvalidatesRoute)
{
    const float infinity = std::numeric_limits<float>::infinity();

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(infinity, 0.0f, 0.0f)
        }
    }.IsValid());

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(0.0f, -infinity, 0.0f)
        }
    }.IsValid());

    EXPECT_FALSE(GameplayRoute{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, infinity)
        }
    }.IsValid());
}

// Protects the data-model boundary by allowing zero-length segments that may be
// handled or filtered later by route-following or route-search logic.
TEST(GameplayRoute, RepeatedConsecutivePointsRemainStructurallyValid)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(1.0f, 2.0f, 3.0f),
            MakeRoutePoint(1.0f, 2.0f, 3.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{}
        }
    };

    EXPECT_TRUE(route.IsValid());
}

// Protects route composition with a valid generic traversal link without
// coupling the route model to doors, jumps, ladders, or other action types.
TEST(GameplayRoute, RouteWithMakeTraversalLinkIsValid)
{
    const GameplayRoute route{
        .points = {
            MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(5.0f, 0.0f, 0.0f)
        },
        .segmentAnnotations = {
            GameplayRouteSegmentAnnotation{
                .traversalLink = MakeTraversalLink(42u)
            }
        }
    };

    EXPECT_TRUE(route.IsValid());
}

TEST(GameplayNavigationDebug, ReplacingAndClearingRouteUsesPerOwnerState)
{
    GameplayNavigationDebugRegistry debug{};
    constexpr EntityHandle firstOwner = static_cast<EntityHandle>(1u);
    constexpr EntityHandle secondOwner = static_cast<EntityHandle>(2u);
    const GameplayRoute firstRoute{
        .points = {MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f), MakeRoutePoint(2.0f, 0.0f, 1.0f)},
        .segmentAnnotations = {{}, {}}};
    const GameplayRoute replacementRoute{
        .points = {MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(3.0f, 0.0f, 2.0f)},
        .segmentAnnotations = {{}}};

    debug.Publish(firstOwner, firstRoute);
    debug.Publish(secondOwner, firstRoute);
    debug.Publish(firstOwner, replacementRoute);

    ASSERT_NE(debug.Find(firstOwner), nullptr);
    EXPECT_EQ(debug.Find(firstOwner)->points.size(), 2u);
    EXPECT_EQ(debug.Routes().size(), 2u);

    debug.Clear(firstOwner);
    EXPECT_EQ(debug.Find(firstOwner), nullptr);
    EXPECT_NE(debug.Find(secondOwner), nullptr);
}

TEST(GameplayNavigationDebug, InvalidOrDegenerateRouteClearsOwner)
{
    GameplayNavigationDebugRegistry debug{};
    constexpr EntityHandle owner = static_cast<EntityHandle>(1u);
    const GameplayRoute route{
        .points = {MakeRoutePoint(0.0f, 0.0f, 0.0f),
            MakeRoutePoint(1.0f, 0.0f, 0.0f)},
        .segmentAnnotations = {{}}};

    debug.Publish(owner, route);
    ASSERT_NE(debug.Find(owner), nullptr);

    debug.Publish(owner, GameplayRoute{});

    EXPECT_EQ(debug.Find(owner), nullptr);
    EXPECT_TRUE(debug.Routes().empty());
}