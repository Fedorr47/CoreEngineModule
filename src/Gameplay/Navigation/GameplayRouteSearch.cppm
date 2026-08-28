module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

export module core:gameplay_route_search;

import :math_utils;
import :gameplay_route;

export namespace rendern
{
    struct GameplayRouteNodeId
    {
        using ValueType = std::uint64_t;
        
        static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();
        
        ValueType value{InvalidValue};
        
        constexpr GameplayRouteNodeId() noexcept = default;
        
        explicit constexpr GameplayRouteNodeId(const ValueType inValue) noexcept : value(inValue)
        {}
        
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }
        
        friend constexpr bool operator==(
            const GameplayRouteNodeId&, 
            const GameplayRouteNodeId&) noexcept = default;
    };
    
    struct GameplayRouteGraphNode
    {
        // Stable graph identity; repeated positions are allowed for distinct IDs.
        GameplayRouteNodeId nodeId{};
        mathUtils::Vec3 worldPosition{};
    };
    
    struct GameplayRouteGraphEdge
    {
        // Directed endpoint IDs. Reverse traversal requires a separate edge.
        GameplayRouteNodeId fromNodeId{};
        GameplayRouteNodeId toNodeId{};
        
        // Complete effective non-negative cost used by weighted search.
        float cost{1.0f};
        
        // Copied unchanged into the reconstructed GameplayRoute segment.
        GameplayRouteSegmentAnnotation annotation{};
    };
    
    struct GameplayRouteGraph
    {
        std::vector<GameplayRouteGraphNode> nodes{};
        std::vector<GameplayRouteGraphEdge> edges{};
        
        [[nodiscard]] bool IsValid() const;
    };
    
    enum class GameplayRouteSearchStatus : std::uint8_t
    {
        Succeeded,
        NoRoute,
        InvalidGraph,
        InvalidRequest
    };
    
    struct GameplayRouteSearchResult
    {
        GameplayRouteSearchStatus status{GameplayRouteSearchStatus::InvalidRequest};
        GameplayRoute route{};
        
        // Present only when status is Succeeded.
        std::optional<float> totalCost{};
        
        [[nodiscard]] bool Succeeded() const noexcept
        {
            return status == GameplayRouteSearchStatus::Succeeded;
        }
    };
    
    [[nodiscard]] GameplayRouteSearchResult FindWeightedGameplayRoute(
        const GameplayRouteGraph& graph,
        const GameplayRouteNodeId startNodeId,
        const GameplayRouteNodeId goalNodeId);
}

namespace rendern
{
    namespace
    {
        using NodeIndexById = std::unordered_map<GameplayRouteNodeId::ValueType, std::size_t>;
        
        inline constexpr std::size_t kInvalidNodeIndex = std::numeric_limits<std::size_t>::max();
        
        struct GameplayRouteSearchQueueEntry
        {
            float accumulatedCost{};
            std::size_t nodeIndex{};
            std::size_t insertionOrder{};
        };
        
        struct GameplayRouteSearchQueueEntryGreater
        {
            [[nodiscard]] bool operator()(
                const GameplayRouteSearchQueueEntry& lhs,
                const GameplayRouteSearchQueueEntry& rhs) const noexcept
            {
                if (lhs.accumulatedCost != rhs.accumulatedCost)
                {
                    return lhs.accumulatedCost > rhs.accumulatedCost;
                }
                
                return lhs.insertionOrder > rhs.insertionOrder;
            }
        };
        
        using GameplayRouteSearchQueue = std::priority_queue<
            GameplayRouteSearchQueueEntry,
            std::vector<GameplayRouteSearchQueueEntry>,
            GameplayRouteSearchQueueEntryGreater>;
        
        [[nodiscard]] bool IsFinitePosition(const mathUtils::Vec3& position) noexcept
        {
            return std::isfinite(position.x) && 
                   std::isfinite(position.y) && 
                   std::isfinite(position.z);
        }
        
        [[nodiscard]] NodeIndexById BuildNodeIndexById(const GameplayRouteGraph& graph)
        {
            NodeIndexById nodeIndexById{};
            nodeIndexById.reserve(graph.nodes.size());
            
            for (std::size_t nodeIndex = 0u; nodeIndex < graph.nodes.size(); ++nodeIndex)
            {
                nodeIndexById.emplace(graph.nodes[nodeIndex].nodeId.value, nodeIndex);
            }
            
            return nodeIndexById;
        }
        
        [[nodiscard]] std::optional<std::size_t> FindNodeIndex(
            const NodeIndexById& nodeIndexById,
            const GameplayRouteNodeId nodeId)
        {
            const auto nodeIt = nodeIndexById.find(nodeId.value);
            if (nodeIt == nodeIndexById.end())
            {
                return std::nullopt;
            }
            
            return nodeIt->second;
        }
    }
    
    bool GameplayRouteGraph::IsValid() const
    {
        NodeIndexById nodeIndexById{};
        nodeIndexById.reserve(nodes.size());
        
        for (std::size_t nodeIndex = 0u; nodeIndex < nodes.size(); ++nodeIndex)
        {
            const GameplayRouteGraphNode& node = nodes[nodeIndex];
            const bool bHasValidNodeIdentity = node.nodeId.IsValid();
            const bool bIsFiniteWorldPosition = IsFinitePosition(node.worldPosition);
            
            if (!bHasValidNodeIdentity || !bIsFiniteWorldPosition)
            {
                return false;
            }
            
            const bool bInserted = nodeIndexById.emplace(node.nodeId.value, nodeIndex).second;
            if (!bInserted)
            {
                return false;
            }
        }
        
        for (const GameplayRouteGraphEdge& edge : edges)
        {
            const bool bHasValidEndpoints = edge.fromNodeId.IsValid() && edge.toNodeId.IsValid();
            const bool bIsFiniteNonNegativeCost = std::isfinite(edge.cost) && edge.cost >= 0.0f;
            const bool bHasValidAnnotation = edge.annotation.IsValid();
            
            if (!bHasValidEndpoints || !bHasValidAnnotation || !bIsFiniteNonNegativeCost)
            {
                return false;
            }
            
            const bool bSourceExists = nodeIndexById.contains(edge.fromNodeId.value);
            const bool bDestinationExists = nodeIndexById.contains(edge.toNodeId.value);
            
            if (!bSourceExists || !bDestinationExists)
            {
                return false;
            }
        }
        
        return true;
    }
    
    GameplayRouteSearchResult FindWeightedGameplayRoute(
        const GameplayRouteGraph& graph,
        const GameplayRouteNodeId startNodeId,
        const GameplayRouteNodeId goalNodeId)
    {
        if (!graph.IsValid())
        {
            return GameplayRouteSearchResult{.status = GameplayRouteSearchStatus::InvalidGraph};
        }
        
        const NodeIndexById nodeIndexById = BuildNodeIndexById(graph);
        
        const std::optional<std::size_t> startNodeIndex = FindNodeIndex(nodeIndexById, startNodeId);
        const std::optional<std::size_t> goalNodeIndex = FindNodeIndex(nodeIndexById, goalNodeId);
        
        const bool bHasValidStartNode = startNodeId.IsValid() && startNodeIndex.has_value();
        const bool bHasValidGoalNode = goalNodeId.IsValid() && goalNodeIndex.has_value();
        const bool bHasValidRequest = bHasValidStartNode && bHasValidGoalNode;
        
        if (!bHasValidRequest)
        {
            return GameplayRouteSearchResult{.status = GameplayRouteSearchStatus::InvalidRequest};
        }
        
        const std::size_t resolvedStartNodeIndex = *startNodeIndex;
        const std::size_t resolvedGoalNodeIndex = *goalNodeIndex;
        
        if (resolvedStartNodeIndex == resolvedGoalNodeIndex)
        {
            GameplayRoute route{ .points = 
                {GameplayRoutePoint
                    {.worldPosition = graph.nodes[resolvedStartNodeIndex].worldPosition}
                }
            };
            
            return GameplayRouteSearchResult{
                .status = GameplayRouteSearchStatus::Succeeded,
                .route = std::move(route),
                .totalCost = 0.0f
            };
        }
        
        std::vector<std::vector<std::size_t>> outgoingEdgeIndices(graph.nodes.size());
        
        for (std::size_t edgeIndex = 0u; edgeIndex  < graph.edges.size(); ++edgeIndex)
        {
            const GameplayRouteGraphEdge& edge = graph.edges[edgeIndex];
            const std::size_t fromNodeIndex = nodeIndexById.at(edge.fromNodeId.value);
            outgoingEdgeIndices[fromNodeIndex].push_back(edgeIndex);
        }
        
        std::vector<float> accumulatedCosts(graph.nodes.size(), std::numeric_limits<float>::infinity());
        std::vector<std::size_t> predecessorNodeIndices(graph.nodes.size(), kInvalidNodeIndex);
        std::vector<std::size_t> predecessorEdgeIndices(graph.nodes.size(), kInvalidNodeIndex);
        
        GameplayRouteSearchQueue queue{};
        std::size_t nextInsertionOrder = 0u;
        
        accumulatedCosts[resolvedStartNodeIndex] = 0.0f;
        
        queue.push(GameplayRouteSearchQueueEntry{
            .accumulatedCost = 0.0f, 
            .nodeIndex =  resolvedStartNodeIndex, 
            .insertionOrder = nextInsertionOrder++});
        
        while (!queue.empty())
        {
            const GameplayRouteSearchQueueEntry currentEntry = queue.top();
            queue.pop();
            
            const bool bIsStaleEntry = currentEntry.accumulatedCost != accumulatedCosts[currentEntry.nodeIndex];
            
            if (bIsStaleEntry)
            {
                continue;
            }
            
            if (currentEntry.nodeIndex == resolvedGoalNodeIndex)
            {
                break;
            }
            
            for (const std::size_t edgeIndex : outgoingEdgeIndices[currentEntry.nodeIndex])
            {
                const GameplayRouteGraphEdge& edge = graph.edges[edgeIndex];
                const std::size_t destinationNodeIndex = nodeIndexById.at(edge.toNodeId.value);
                const float candidateCost = currentEntry.accumulatedCost + edge.cost;
                const bool bHasLowerCost = candidateCost < accumulatedCosts[destinationNodeIndex];
                
                if (!bHasLowerCost)
                {
                    continue;
                }
                
                accumulatedCosts[destinationNodeIndex] = candidateCost;
                predecessorNodeIndices[destinationNodeIndex] = currentEntry.nodeIndex;
                predecessorEdgeIndices[destinationNodeIndex] = edgeIndex;
                
                queue.push(GameplayRouteSearchQueueEntry{
                .accumulatedCost = candidateCost,
                .nodeIndex = destinationNodeIndex,
                .insertionOrder = nextInsertionOrder++});
            }
        }
        
        const float goalCost = accumulatedCosts[resolvedGoalNodeIndex];
            
        if (!std::isfinite(goalCost))
        {
            return GameplayRouteSearchResult{ .status = GameplayRouteSearchStatus::NoRoute };
        }
            
        std::vector<std::size_t> pathNodeIndices{};
        std::vector<std::size_t> pathEdgeIndices{};
            
        for (std::size_t nodeIndex = resolvedGoalNodeIndex; nodeIndex != kInvalidNodeIndex; nodeIndex = predecessorNodeIndices[nodeIndex])
        {
            pathNodeIndices.push_back(nodeIndex);
                
            const std::size_t predecessorEdgeIndex  = predecessorEdgeIndices[nodeIndex];
            if (predecessorEdgeIndex != kInvalidNodeIndex)
            {
                pathEdgeIndices.push_back(predecessorEdgeIndex);
            }
        }
            
        std::reverse(pathNodeIndices.begin(), pathNodeIndices.end());
        std::reverse(pathEdgeIndices.begin(), pathEdgeIndices.end());
        
        GameplayRoute route{};
        route.points.reserve(pathNodeIndices.size());
        route.segmentAnnotations.reserve(pathEdgeIndices.size());
        
        for (const std::size_t nodeIndex : pathNodeIndices)
        {
            route.points.push_back(GameplayRoutePoint
                {
                .worldPosition = graph.nodes[nodeIndex].worldPosition
                });
        }
        
        for (const std::size_t edgeIndex : pathEdgeIndices)
        {
            route.segmentAnnotations.push_back(graph.edges[edgeIndex].annotation);
        }
        
        if (!route.IsValid())
        {
            return GameplayRouteSearchResult{ .status = GameplayRouteSearchStatus::InvalidGraph };
        }
        
        return GameplayRouteSearchResult{ 
            .status = GameplayRouteSearchStatus::Succeeded,
            .route = std::move(route),
            .totalCost = goalCost};
    }
}