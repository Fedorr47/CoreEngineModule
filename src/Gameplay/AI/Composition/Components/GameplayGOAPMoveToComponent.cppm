module;

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

export module core:gameplay_goap_move_to_component;

import :gameplay_goap_composition_registry;
import :gameplay;
import :ai_move_to_action;
import :ai_move_to_action_binding;
import :gameplay_route_search;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_link;
import :gameplay_steering;
import :gameplay_goap_path_inspection;

namespace rendern::goap_move_detail
{
    struct AuthoredGraph
    {
        GameplayRouteGraph graph;
        std::map<std::string, GameplayRouteNodeId, std::less<>> nodes;
    };
    AuthoredGraph CompileGraph(const GameplayGOAPCompositionContext& context)
    {
        AuthoredGraph result;
        const auto& asset = *context.routeGraph;
        for (const auto& name : asset.nodes)
        {
            const GameplayRouteNodeId id{result.nodes.size() + 1u};
            if (!result.nodes.emplace(name, id).second)
            {
                context.Fail(asset.source, "duplicate route node '" + name + "'");
            }
            result.graph.nodes.push_back({id, context.Role(name).position});
        }
        for (const auto& edge : asset.edges)
        {
            const auto from = result.nodes.find(edge.from), to = result.nodes.find(edge.to);
            if (from == result.nodes.end() || to == result.nodes.end())
            {
                context.Fail(asset.source, "route edge references an unknown node");
            }
            const float cost = edge.cost.value_or(mathUtils::Length(
                context.Role(edge.to).position - context.Role(edge.from).position));
            GameplayRouteSegmentAnnotation annotation{};
            if (!edge.traversal.empty())
            {
                if (context.levelBindings == nullptr)
                {
                    context.Fail(asset.source, "missing traversal bindings");
                }
                const auto& entries = context.levelBindings->traversals;
                const auto binding = std::ranges::find(entries, edge.traversal, &GameplayAITraversalBindingAsset::name);
                if (binding == entries.end()
                    || std::ranges::count(entries, edge.traversal, &GameplayAITraversalBindingAsset::name) != 1)
                {
                    context.Fail(edge.traversal, "missing or ambiguous traversal binding");
                }
                const GameplayTraversalTypeId type = binding->type == "jump" ? kJumpTraversalTypeId
                    : binding->type == "door" ? kDoorTraversalTypeId : GameplayTraversalTypeId{};
                const GameplayTraversalLinkHandle handle{binding->handle};
                const auto link = context.services.traversalLinkRegistry.Find(handle);
                if (!type.IsValid() || !link || !link->IsValid() || link->traversalTypeId != type
                    || link->targetEntity != context.Role(binding->target).entity)
                {
                    context.Fail(edge.traversal, "unresolved traversal link or target/type mismatch");
                }
                annotation.traversalLink = handle;
            }
            result.graph.edges.push_back({from->second, to->second, cost, annotation});
        }
        if (!result.graph.IsValid())
        {
            context.Fail(asset.source, "invalid route graph");
        }
        return result;
    }

    class MoveToCapability final : public IGameplayGOAPCapability,
        public IAIMoveToActionRequestProvider, public IAIActionReservationTargetProvider,
        public IGameplayGOAPActionPathProvider
    {
    public:
        MoveToCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
            : world_(context.services.world), links_(context.services.traversalLinkRegistry),
              executors_(context.services.traversalExecutorRegistry), reservations_(context.services.reservationSystem)
        {
            std::optional<AuthoredGraph> authored;
            if (context.routeGraph != nullptr)
            {
                authored = CompileGraph(context);
                graphs_.push_back(std::move(authored->graph));
            }
            for (const auto& asset : assets)
            {
                const auto& parameters = context.Parameters<GameplayAIMoveToAsset>(asset);
                if (!(parameters.acceptanceRadius > 0)
                    || !std::isfinite(parameters.acceptanceRadius * parameters.acceptanceRadius)
                    || !std::isfinite(parameters.slowingRadius) || parameters.slowingRadius < parameters.acceptanceRadius
                    || parameters.sourceRadius < 0 || !std::isfinite(parameters.sourceRadius * parameters.sourceRadius))
                {
                    context.Fail(asset.context, "require finite slowingRadius >= acceptanceRadius > 0 and sourceRadius >= 0");
                }
                const auto id = context.definition.FindActionContext(asset.context);
                if (!id)
                {
                    context.Fail(asset.context, "unknown action context");
                }
                Route route{.id = *id, .sourceRadius = parameters.sourceRadius};
                if (authored)
                {
                    const auto source = authored->nodes.find(parameters.source), target = authored->nodes.find(parameters.target);
                    if (source == authored->nodes.end() || target == authored->nodes.end())
                    {
                        context.Fail(asset.context, "move source/target is not in the route graph");
                    }
                    route.start = source->second;
                    route.goal = target->second;
                }
                else
                {
                    route.graphIndex = graphs_.size();
                    route.start = GameplayRouteNodeId{1u};
                    route.goal = GameplayRouteNodeId{2u};
                    GameplayRouteGraph graph;
                    graph.nodes = {{route.start, context.Role(parameters.source).position},
                        {route.goal, context.Role(parameters.target).position}};
                    graph.edges = {{route.start, route.goal, 1.0f, {}}};
                    graphs_.push_back(std::move(graph));
                    if (parameters.routeCost)
                    {
                        context.Fail(asset.context, "routeCost requires an authored route graph");
                    }
                }
                const auto planned = FindWeightedGameplayRoute(graphs_[route.graphIndex], route.start, route.goal);
                if (!planned.Succeeded() || !planned.totalCost || !std::isfinite(*planned.totalCost))
                {
                    context.Fail(asset.context, "no valid route between action endpoints");
                }
                if (parameters.routeCost)
                {
                    costs_.push_back({asset.type, asset.context, *planned.totalCost});
                }
                if (parameters.reserveTarget)
                {
                    route.reservedTarget = context.Role(parameters.target).entity;
                    if (!world_.IsEntityValid(route.reservedTarget) || !world_.HasInteractionPoint(route.reservedTarget))
                    {
                        context.Fail(asset.context, "reservation target requires a live interaction point");
                    }
                }
                route.steering.acceptanceRadius = parameters.acceptanceRadius;
                route.steering.slowingRadius = parameters.slowingRadius;
                route.steering.wantsRun = parameters.wantsRun;
                routes_.push_back(route);
            }
        }
        std::unique_ptr<IAIActionBinding> CreateBinding(AIAgentWorldState&, std::vector<GameplayWorldEvent>&) override
        {
            return std::make_unique<AIMoveToActionBinding>(world_, links_, executors_, *this, reservations_, *this);
        }
        std::vector<GameplayGOAPActionCostOverride> CostOverrides() const override { return costs_; }
        std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId id) const override
        {
            const auto* route = Find(id);
            if (route == nullptr)
            {
                return {};
            }
            auto planned = FindWeightedGameplayRoute(graphs_[route->graphIndex], route->start, route->goal);
            return planned.Succeeded() ? std::optional{std::move(planned.route)} : std::nullopt;
        }
        EntityHandle ResolveReservationTarget(const AIActionRuntimeContext& context) override
        {
            const auto* route = Find(context.contextId);
            return route == nullptr ? kNullEntity : route->reservedTarget;
        }
        std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override
        {
            const auto* route = Find(context.contextId);
            if (route == nullptr)
            {
                return {};
            }
            const auto& graph = graphs_[route->graphIndex];
            if (route->sourceRadius > 0)
            {
                const auto* transform = world_.TryGetTransform(context.agentEntity);
                if (transform == nullptr)
                {
                    return {};
                }
                GameplayRouteNodeId nearest;
                float distance = std::numeric_limits<float>::max();
                for (const auto& node : graph.nodes)
                {
                    const auto delta = transform->position - node.worldPosition;
                    const float candidate = mathUtils::Dot(delta, delta);
                    if (candidate < distance)
                    {
                        nearest = node.nodeId;
                        distance = candidate;
                    }
                }
                if (nearest != route->start || distance > route->sourceRadius * route->sourceRadius)
                {
                    return {};
                }
            }
            return AIMoveToActionRequest{&graph, route->start, route->goal, route->steering};
        }
    private:
        struct Route
        {
            AIActionContextId id;
            std::size_t graphIndex{};
            GameplayRouteNodeId start, goal;
            GameplayArrivalSteeringSettings steering;
            float sourceRadius{};
            EntityHandle reservedTarget{kNullEntity};
        };
        const Route* Find(AIActionContextId id) const
        {
            const auto found = std::ranges::find(routes_, id, &Route::id);
            return found == routes_.end() ? nullptr : &*found;
        }
        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& links_;
        const GameplayTraversalExecutorRegistry& executors_;
        GameplayObjectReservationSystem& reservations_;
        std::vector<GameplayRouteGraph> graphs_;
        std::vector<Route> routes_;
        std::vector<GameplayGOAPActionCostOverride> costs_;
    };
}

export namespace rendern
{
    void RegisterGameplayGOAPMoveToComponent(GameplayGOAPCompositionRegistry& registry)
    {
        if (!registry.RegisterCapability("move_to", kAIMoveToActionId, [](auto assets, const auto& context)
            {
                return std::make_unique<goap_move_detail::MoveToCapability>(assets, context);
            }))
        {
            throw std::logic_error("Duplicate move_to component registration");
        }
    }
}
