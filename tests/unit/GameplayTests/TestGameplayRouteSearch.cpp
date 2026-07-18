#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "TestSupport/MathTestHelper.h"
#include "TestSupport/GameplayRouteTestHelper.h"

import core;

using namespace rendern;
using namespace MathTestHelper;
using namespace GameplayRouteTestHelper;

namespace
{
    [[nodiscard]] constexpr GameplayRouteNodeId Id(const std::uint64_t value) noexcept
    {
        return GameplayRouteNodeId{ value };
    }

    [[nodiscard]] constexpr GameplayRouteGraphNode MakeRoutePoint(
        const std::uint64_t id,
        const float x,
        const float y = 0.0f,
        const float z = 0.0f) noexcept
    {
        return GameplayRouteGraphNode{
            .nodeId = Id(id),
            .worldPosition = mathUtils::Vec3{ x, y, z }
        };
    }

    [[nodiscard]] constexpr GameplayRouteSegmentAnnotation Ordinary() noexcept
    {
        return GameplayRouteSegmentAnnotation{};
    }

    [[nodiscard]] constexpr GameplayRouteSegmentAnnotation Link(const std::uint64_t value) noexcept
    {
        return GameplayRouteSegmentAnnotation{ .traversalLink = MakeTraversalLink(value) };
    }

    [[nodiscard]] constexpr GameplayRouteGraphEdge Edge(
        const std::uint64_t from,
        const std::uint64_t to,
        const float cost,
        const GameplayRouteSegmentAnnotation annotation = Ordinary()) noexcept
    {
        return GameplayRouteGraphEdge{
            .fromNodeId = Id(from),
            .toNodeId = Id(to),
            .cost = cost,
            .annotation = annotation
        };
    }

    [[nodiscard]] GameplayRouteGraph TwoNodeGraph()
    {
        return GameplayRouteGraph{
            .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) },
            .edges = { Edge(1u, 2u, 1.0f) }
        };
    }

    void ExpectAnnotationEqual(
        const GameplayRouteSegmentAnnotation& actual,
        const GameplayRouteSegmentAnnotation& expected)
    {
        EXPECT_EQ(actual.traversalLink.has_value(), expected.traversalLink.has_value());

        if (actual.traversalLink.has_value() || expected.traversalLink.has_value())
        {
            ASSERT_TRUE(actual.traversalLink.has_value());
            ASSERT_TRUE(expected.traversalLink.has_value());
            EXPECT_EQ(*actual.traversalLink, *expected.traversalLink);
        }
    }

    void ExpectGraphEqual(const GameplayRouteGraph& actual, const GameplayRouteGraph& expected)
    {
        ASSERT_EQ(actual.nodes.size(), expected.nodes.size());
        for (std::size_t nodeIndex = 0u; nodeIndex < actual.nodes.size(); ++nodeIndex)
        {
            EXPECT_EQ(actual.nodes[nodeIndex].nodeId, expected.nodes[nodeIndex].nodeId);
            EXPECT_FLOAT_EQ(actual.nodes[nodeIndex].worldPosition.x, expected.nodes[nodeIndex].worldPosition.x);
            EXPECT_FLOAT_EQ(actual.nodes[nodeIndex].worldPosition.y, expected.nodes[nodeIndex].worldPosition.y);
            EXPECT_FLOAT_EQ(actual.nodes[nodeIndex].worldPosition.z, expected.nodes[nodeIndex].worldPosition.z);
        }

        ASSERT_EQ(actual.edges.size(), expected.edges.size());
        for (std::size_t edgeIndex = 0u; edgeIndex < actual.edges.size(); ++edgeIndex)
        {
            EXPECT_EQ(actual.edges[edgeIndex].fromNodeId, expected.edges[edgeIndex].fromNodeId);
            EXPECT_EQ(actual.edges[edgeIndex].toNodeId, expected.edges[edgeIndex].toNodeId);
            EXPECT_FLOAT_EQ(actual.edges[edgeIndex].cost, expected.edges[edgeIndex].cost);
            ExpectAnnotationEqual(actual.edges[edgeIndex].annotation, expected.edges[edgeIndex].annotation);
        }
    }

    void ExpectSearchResultEqual(
        const GameplayRouteSearchResult& actual,
        const GameplayRouteSearchResult& expected)
    {
        EXPECT_EQ(actual.status, expected.status);
        EXPECT_EQ(actual.Succeeded(), expected.Succeeded());
        EXPECT_EQ(actual.totalCost.has_value(), expected.totalCost.has_value());
        if (actual.totalCost.has_value() || expected.totalCost.has_value())
        {
            ASSERT_TRUE(actual.totalCost.has_value());
            ASSERT_TRUE(expected.totalCost.has_value());
            EXPECT_FLOAT_EQ(*actual.totalCost, *expected.totalCost);
        }

        ASSERT_EQ(actual.route.points.size(), expected.route.points.size());
        for (std::size_t pointIndex = 0u; pointIndex < actual.route.points.size(); ++pointIndex)
        {
            EXPECT_FLOAT_EQ(actual.route.points[pointIndex].worldPosition.x, expected.route.points[pointIndex].worldPosition.x);
            EXPECT_FLOAT_EQ(actual.route.points[pointIndex].worldPosition.y, expected.route.points[pointIndex].worldPosition.y);
            EXPECT_FLOAT_EQ(actual.route.points[pointIndex].worldPosition.z, expected.route.points[pointIndex].worldPosition.z);
        }

        ASSERT_EQ(actual.route.segmentAnnotations.size(), expected.route.segmentAnnotations.size());
        for (std::size_t annotationIndex = 0u; annotationIndex < actual.route.segmentAnnotations.size(); ++annotationIndex)
        {
            ExpectAnnotationEqual(
                actual.route.segmentAnnotations[annotationIndex],
                expected.route.segmentAnnotations[annotationIndex]);
        }
    }
}

// Empty graph data is structurally valid so callers can distinguish malformed graphs from invalid node requests.
TEST(GameplayRouteSearch, EmptyGraphIsStructurallyValid)
{
    EXPECT_TRUE(GameplayRouteGraph{}.IsValid());
}

// MakeRoutePoint identity must use explicit valid IDs rather than implicit vector positions.
TEST(GameplayRouteSearch, NodeWithInvalidIdInvalidatesGraph)
{
    GameplayRouteGraph graph{ .nodes = { GameplayRouteGraphNode{} } };
    EXPECT_FALSE(graph.IsValid());
}

// Duplicate IDs would make deterministic predecessor reconstruction ambiguous and are rejected.
TEST(GameplayRouteSearch, DuplicateNodeIdsInvalidateGraph)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(1u, 1.0f) } };
    EXPECT_FALSE(graph.IsValid());
}

// NaN coordinates cannot be consumed safely by route followers and invalidate graph nodes.
TEST(GameplayRouteSearch, NanNodePositionInvalidatesGraph)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, std::numeric_limits<float>::quiet_NaN()) } };
    EXPECT_FALSE(graph.IsValid());
}

// Infinite coordinates cannot form valid route points and invalidate graph nodes.
TEST(GameplayRouteSearch, InfiniteNodePositionInvalidatesGraph)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, std::numeric_limits<float>::infinity()) } };
    EXPECT_FALSE(graph.IsValid());
}

// Directed edges must explicitly name a valid source endpoint.
TEST(GameplayRouteSearch, EdgeWithInvalidSourceIdInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].fromNodeId = GameplayRouteNodeId{};
    EXPECT_FALSE(graph.IsValid());
}

// Directed edges must explicitly name a valid destination endpoint.
TEST(GameplayRouteSearch, EdgeWithInvalidDestinationIdInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].toNodeId = GameplayRouteNodeId{};
    EXPECT_FALSE(graph.IsValid());
}

// Edges cannot reference source IDs absent from the graph node list.
TEST(GameplayRouteSearch, EdgeReferencingMissingSourceNodeInvalidatesGraph)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f) }, .edges = { Edge(2u, 1u, 1.0f) } };
    EXPECT_FALSE(graph.IsValid());
}

// Edges cannot reference destination IDs absent from the graph node list.
TEST(GameplayRouteSearch, EdgeReferencingMissingDestinationNodeInvalidatesGraph)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f) }, .edges = { Edge(1u, 2u, 1.0f) } };
    EXPECT_FALSE(graph.IsValid());
}

// Dijkstra requires non-negative effective edge weights to preserve shortest-path correctness.
TEST(GameplayRouteSearch, NegativeEdgeCostInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = -1.0f;
    EXPECT_FALSE(graph.IsValid());
}

// NaN edge costs cannot be ordered deterministically by the weighted search.
TEST(GameplayRouteSearch, NanEdgeCostInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(graph.IsValid());
}

// Infinite edge costs are rejected instead of becoming sentinel values for missing paths.
TEST(GameplayRouteSearch, InfiniteEdgeCostInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(graph.IsValid());
}

// Invalid traversal links are rejected while the graph still treats valid links generically.
TEST(GameplayRouteSearch, InvalidEdgeAnnotationInvalidatesGraph)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].annotation.traversalLink = GameplayTraversalLinkHandle{};
    EXPECT_FALSE(graph.IsValid());
}

// Zero-cost edges are valid because effective costs may intentionally encode free transitions.
TEST(GameplayRouteSearch, ZeroCostEdgeRemainsValid)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = 0.0f;
    EXPECT_TRUE(graph.IsValid());
}

// Self-edges are valid weighted graph constructs and must not be rejected speculatively.
TEST(GameplayRouteSearch, SelfEdgeRemainsValid)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f) }, .edges = { Edge(1u, 1u, 0.0f) } };
    EXPECT_TRUE(graph.IsValid());
}

// Parallel edges preserve authoring order and may differ in cost or annotation.
TEST(GameplayRouteSearch, ParallelEdgesRemainValid)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) }, .edges = { Edge(1u, 2u, 2.0f), Edge(1u, 2u, 1.0f, Link(4u)) } };
    EXPECT_TRUE(graph.IsValid());
}

// Empty valid graphs still cannot satisfy requests because neither endpoint exists.
TEST(GameplayRouteSearch, EmptyGraphSearchReturnsInvalidRequest)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(GameplayRouteGraph{}, Id(1u), Id(2u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidRequest);
    EXPECT_FALSE(result.totalCost.has_value());
}

// Invalid start IDs are reported as request errors after graph validation succeeds.
TEST(GameplayRouteSearch, InvalidStartIdReturnsInvalidRequest)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), GameplayRouteNodeId{}, Id(2u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidRequest);
}

// Invalid goal IDs are reported as request errors after graph validation succeeds.
TEST(GameplayRouteSearch, InvalidGoalIdReturnsInvalidRequest)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(1u), GameplayRouteNodeId{});
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidRequest);
}

// Missing start nodes are invalid requests, not graph failures.
TEST(GameplayRouteSearch, MissingStartNodeReturnsInvalidRequest)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(9u), Id(2u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidRequest);
}

// Missing goal nodes are invalid requests, not no-route results.
TEST(GameplayRouteSearch, MissingGoalNodeReturnsInvalidRequest)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(1u), Id(9u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidRequest);
}

// Graph validation has precedence so malformed graph data is never hidden by a bad request.
TEST(GameplayRouteSearch, InvalidGraphTakesPrecedenceOverInvalidRequest)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = -1.0f;
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, GameplayRouteNodeId{}, GameplayRouteNodeId{});
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::InvalidGraph);
}

// Disconnected valid endpoints are an explicit no-route outcome rather than an invalid request.
TEST(GameplayRouteSearch, DisconnectedValidNodesReturnNoRoute)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::NoRoute);
    EXPECT_TRUE(result.route.IsEmpty());
    EXPECT_FALSE(result.totalCost.has_value());
}

// Failure results keep route data empty and omit total cost for every failure status.
TEST(GameplayRouteSearch, FailureResultsContainEmptyRouteAndNoTotalCost)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = -1.0f;
    const std::vector<GameplayRouteSearchResult> results{
        FindWeightedGameplayRoute(graph, Id(1u), Id(2u)),
        FindWeightedGameplayRoute(TwoNodeGraph(), Id(9u), Id(2u)),
        FindWeightedGameplayRoute(GameplayRouteGraph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) } }, Id(1u), Id(2u))
    };

    for (const GameplayRouteSearchResult& result : results)
    {
        EXPECT_FALSE(result.Succeeded());
        EXPECT_TRUE(result.route.IsEmpty());
        EXPECT_FALSE(result.totalCost.has_value());
    }
}

// A request already at the destination still produces a valid one-point route anchor.
TEST(GameplayRouteSearch, StartEqualsGoalReturnsOnePointAndZeroCost)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(1u), Id(1u));
    EXPECT_TRUE(result.Succeeded());
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 0.0f);
    ASSERT_EQ(result.route.points.size(), 1u);
    EXPECT_TRUE(result.route.segmentAnnotations.empty());
    EXPECT_TRUE(result.route.IsValid());
    ExpectVec3Near(result.route.points[0].worldPosition, mathUtils::Vec3{ 0.0f, 0.0f, 0.0f }, kEpsVec);
}

// A direct edge reconstructs its two endpoint positions in start-to-goal order.
TEST(GameplayRouteSearch, DirectEdgeReturnsTwoPoints)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(1u), Id(2u));
    EXPECT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.points.size(), 2u);
    ExpectVec3Near(result.route.points[0].worldPosition, mathUtils::Vec3{ 0.0f, 0.0f, 0.0f });
    ExpectVec3Near(result.route.points[1].worldPosition, mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });
}

// Segment annotations are copied from the selected direct edge without reinterpretation.
TEST(GameplayRouteSearch, DirectEdgeCopiesItsAnnotation)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) }, .edges = { Edge(1u, 2u, 1.0f, Link(7u)) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.segmentAnnotations.size(), 1u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(7u));
}

// Weighted search prefers the lower total cost even when it requires more route segments.
TEST(GameplayRouteSearch, LowerCostMultiEdgeRouteBeatsHigherCostDirectEdge)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f), MakeRoutePoint(3u, 2.0f) }, .edges = { Edge(1u, 3u, 10.0f), Edge(1u, 2u, 1.0f), Edge(2u, 3u, 1.5f) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(3u));
    ASSERT_TRUE(result.Succeeded());
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 2.5f);
    ASSERT_EQ(result.route.points.size(), 3u);
    ExpectVec3Near(result.route.points[1].worldPosition, mathUtils::Vec3{ 1.0f, 0.0f, 0.0f });
}

// Reported total cost is the exact sum of the selected edge weights.
TEST(GameplayRouteSearch, RouteCostEqualsSelectedEdgeCostSum)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f), MakeRoutePoint(3u, 2.0f) }, .edges = { Edge(1u, 2u, 2.25f), Edge(2u, 3u, 3.5f) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(3u));
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 5.75f);
}

// Zero-cost paths can succeed and preserve a meaningful zero total cost.
TEST(GameplayRouteSearch, ZeroCostRouteSucceeds)
{
    GameplayRouteGraph graph = TwoNodeGraph();
    graph.edges[0].cost = 0.0f;
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    EXPECT_TRUE(result.Succeeded());
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 0.0f);
}

// Directed edges are not traversable backward unless a reverse edge is authored.
TEST(GameplayRouteSearch, DirectedEdgeCannotBeTraversedBackwardWithoutReverseEdge)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(2u), Id(1u));
    EXPECT_EQ(result.status, GameplayRouteSearchStatus::NoRoute);
}

// Cycles terminate because stale queue entries are ignored and equal-cost revisits do not replace predecessors.
TEST(GameplayRouteSearch, CycleDoesNotStallSearch)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f), MakeRoutePoint(3u, 2.0f) }, .edges = { Edge(1u, 2u, 1.0f), Edge(2u, 1u, 1.0f), Edge(2u, 3u, 1.0f) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(3u));
    EXPECT_TRUE(result.Succeeded());
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 2.0f);
}

// Self-edges do not cause infinite processing and are ignored when they do not improve distance.
TEST(GameplayRouteSearch, SelfEdgeDoesNotStallSearch)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) }, .edges = { Edge(1u, 1u, 0.0f), Edge(1u, 2u, 1.0f) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    EXPECT_TRUE(result.Succeeded());
    ASSERT_TRUE(result.totalCost.has_value());
    EXPECT_FLOAT_EQ(*result.totalCost, 1.0f);
}

// Parallel edges between the same endpoints choose the lower-cost annotation-bearing edge.
TEST(GameplayRouteSearch, LowerCostParallelEdgeIsSelected)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) }, .edges = { Edge(1u, 2u, 5.0f, Link(10u)), Edge(1u, 2u, 1.0f, Link(20u)) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.segmentAnnotations.size(), 1u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(20u));
}

// Selected traversal links remain opaque handles owned by the route annotation contract.
TEST(GameplayRouteSearch, SelectedTraversalLinkIsPreserved)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f) }, .edges = { Edge(1u, 2u, 1.0f, Link(99u)) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(2u));
    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.segmentAnnotations.size(), 1u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(99u));
}

// Multiple selected traversal links preserve edge order during predecessor reconstruction.
TEST(GameplayRouteSearch, MultipleSelectedTraversalLinksRemainInCorrectOrder)
{
    GameplayRouteGraph graph{ .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f), MakeRoutePoint(3u, 2.0f) }, .edges = { Edge(1u, 2u, 1.0f, Link(11u)), Edge(2u, 3u, 1.0f, Link(22u)) } };
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(3u));
    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.segmentAnnotations.size(), 2u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    ASSERT_TRUE(result.route.segmentAnnotations[1].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(11u));
    EXPECT_EQ(*result.route.segmentAnnotations[1].traversalLink, MakeTraversalLink(22u));
}

// Successful search results must be directly consumable by route followers without repair.
TEST(GameplayRouteSearch, ResultingRoutePassesIsValid)
{
    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(TwoNodeGraph(), Id(1u), Id(2u));
    ASSERT_TRUE(result.Succeeded());
    EXPECT_TRUE(result.route.IsValid());
}

// Search owns all mutable state locally and must leave every authored node, edge, cost, and annotation unchanged.
TEST(GameplayRouteSearch, SourceGraphRemainsUnchanged)
{
    GameplayRouteGraph graph{
        .nodes = { MakeRoutePoint(1u, 0.0f, 0.0f, 0.0f), MakeRoutePoint(2u, 1.0f, 2.0f, 3.0f), MakeRoutePoint(3u, -4.0f, 5.0f, -6.0f) },
        .edges = { Edge(1u, 2u, 1.0f, Link(1u)), Edge(2u, 3u, 2.0f), Edge(1u, 3u, 5.0f, Link(3u)) }
    };
    const GameplayRouteGraph original = graph;

    (void)FindWeightedGameplayRoute(graph, Id(1u), Id(3u));

    ExpectGraphEqual(graph, original);
}

// Equal-cost alternatives must follow stable edge discovery order even when node vector order prefers another branch.
TEST(GameplayRouteSearch, EqualCostAlternativesProduceDocumentedDeterministicRoute)
{
    GameplayRouteGraph graph{
        .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 20.0f), MakeRoutePoint(3u, 10.0f), MakeRoutePoint(4u, 30.0f) },
        .edges = { Edge(1u, 3u, 1.0f, Link(13u)), Edge(1u, 2u, 1.0f, Link(12u)), Edge(3u, 4u, 1.0f, Link(34u)), Edge(2u, 4u, 1.0f, Link(24u)) }
    };

    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(4u));

    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.points.size(), 3u);
    ExpectVec3Near(result.route.points[1].worldPosition, mathUtils::Vec3{ 10.0f, 0.0f, 0.0f });
    ASSERT_EQ(result.route.segmentAnnotations.size(), 2u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    ASSERT_TRUE(result.route.segmentAnnotations[1].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(13u));
    EXPECT_EQ(*result.route.segmentAnnotations[1].traversalLink, MakeTraversalLink(34u));
}

// Equal candidate costs preserve the first predecessor instead of replacing it with a later equivalent path.
TEST(GameplayRouteSearch, EqualCostCandidateDoesNotReplaceFirstPredecessor)
{
    GameplayRouteGraph graph{
        .nodes = { MakeRoutePoint(1u, 0.0f), MakeRoutePoint(2u, 1.0f), MakeRoutePoint(3u, 2.0f), MakeRoutePoint(4u, 3.0f) },
        .edges = { Edge(1u, 2u, 1.0f, Link(12u)), Edge(1u, 3u, 1.0f, Link(13u)), Edge(2u, 4u, 1.0f, Link(24u)), Edge(3u, 4u, 1.0f, Link(34u)) }
    };

    const GameplayRouteSearchResult result = FindWeightedGameplayRoute(graph, Id(1u), Id(4u));

    ASSERT_TRUE(result.Succeeded());
    ASSERT_EQ(result.route.segmentAnnotations.size(), 2u);
    ASSERT_TRUE(result.route.segmentAnnotations[0].traversalLink.has_value());
    ASSERT_TRUE(result.route.segmentAnnotations[1].traversalLink.has_value());
    EXPECT_EQ(*result.route.segmentAnnotations[0].traversalLink, MakeTraversalLink(12u));
    EXPECT_EQ(*result.route.segmentAnnotations[1].traversalLink, MakeTraversalLink(24u));
}

// Repeating search over unchanged inputs must produce identical statuses, costs, points, and annotations.
TEST(GameplayRouteSearch, RepeatedSearchesOnSameGraphProduceIdenticalResults)
{
    GameplayRouteGraph graph{
        .nodes = { MakeRoutePoint(1u, 0.0f, 0.0f, 0.0f), MakeRoutePoint(2u, 1.0f, 2.0f, 3.0f), MakeRoutePoint(3u, 4.0f, 5.0f, 6.0f), MakeRoutePoint(4u, 7.0f, 8.0f, 9.0f) },
        .edges = { Edge(1u, 2u, 1.0f, Link(12u)), Edge(1u, 3u, 1.0f, Link(13u)), Edge(2u, 4u, 1.0f, Link(24u)), Edge(3u, 4u, 1.0f, Link(34u)) }
    };

    const GameplayRouteSearchResult first = FindWeightedGameplayRoute(graph, Id(1u), Id(4u));
    const GameplayRouteSearchResult second = FindWeightedGameplayRoute(graph, Id(1u), Id(4u));

    ExpectSearchResultEqual(first, second);
}
