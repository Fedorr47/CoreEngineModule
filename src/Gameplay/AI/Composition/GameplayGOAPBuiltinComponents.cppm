module;

#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_goap_builtin_components;

export import :gameplay_goap_composition_registry;
import :gameplay;
import :ai_move_to_action_binding;
import :ai_move_to_action;
import :gameplay_route;
import :gameplay_steering;

namespace rendern::goap_components_detail
{
    class SpatialObservation final : public IGameplayGOAPObservation
    {
    public:
        SpatialObservation(GameplayAIResolvedRole target, AIWorldFactId fact,
            bool requireInteraction, std::optional<float> radius)
            : target_(std::move(target)), fact_(fact), requireInteraction_(requireInteraction),
              radius_(radius)
        {
        }

        void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent>, AIAgentWorldState& facts) override
        {
            bool value = world.IsEntityValid(target_.entity)
                && (!requireInteraction_ || world.HasInteractionPoint(target_.entity));
            if (radius_)
            {
                const auto* transform = world.TryGetTransform(agent);
                if (value && transform != nullptr)
                {
                    const auto offset = transform->position - target_.position;
                    value = mathUtils::Dot(offset, offset) <= *radius_ * *radius_;
                }
                else
                {
                    value = false;
                }
            }
            facts.SetFact(fact_, value);
        }

    private:
        GameplayAIResolvedRole target_;
        AIWorldFactId fact_;
        bool requireInteraction_{};
        std::optional<float> radius_;
    };

    std::unique_ptr<IGameplayGOAPObservation> CompileSpatial(
        const GameplayAIObservationAsset& asset, const GameplayGOAPCompositionContext& context,
        bool distance)
    {
        const auto& target = context.Role(asset.target);
        if (!context.services.world.IsEntityValid(target.entity))
        {
            context.Fail(asset.type, "target role '" + asset.target + "' has no gameplay entity");
        }
        if (asset.requireInteractionPoint && !context.services.world.HasInteractionPoint(target.entity))
        {
            context.Fail(asset.type, "target role '" + asset.target + "' has no interaction point");
        }
        if (distance && (!(asset.radius > 0.0f) || !std::isfinite(asset.radius * asset.radius)))
        {
            context.Fail(asset.type, "radius must be positive and its square finite");
        }
        if (!distance && asset.radius != 0.0f)
        {
            context.Fail(asset.type, "radius is only supported by within_distance");
        }
        return std::make_unique<SpatialObservation>(target, context.BooleanFact(asset.fact),
            asset.requireInteractionPoint, distance ? std::optional{asset.radius} : std::nullopt);
    }

    class MoveToCapability final : public IGameplayGOAPCapability,
        public IAIMoveToActionRequestProvider
    {
    public:
        MoveToCapability(std::span<const GameplayAICapabilityAsset> assets,
            const GameplayGOAPCompositionContext& context)
            : world_(context.services.world), links_(context.services.traversalLinkRegistry),
              executors_(context.services.traversalExecutorRegistry)
        {
            for (const auto& asset : assets)
            {
                if (!(asset.acceptanceRadius > 0.0f)
                    || !std::isfinite(asset.acceptanceRadius * asset.acceptanceRadius)
                    || !std::isfinite(asset.slowingRadius)
                    || asset.slowingRadius < asset.acceptanceRadius)
                {
                    context.Fail(asset.context, "require finite slowingRadius >= acceptanceRadius > 0");
                }
                const auto id = context.definition.FindActionContext(asset.context);
                if (!id)
                {
                    context.Fail(asset.context, "unknown action context");
                }
                Route route{.context = *id};
                route.graph.nodes = {{GameplayRouteNodeId{1u}, context.Role(asset.source).position},
                    {GameplayRouteNodeId{2u}, context.Role(asset.target).position}};
                route.graph.edges = {{GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, 1.0f, {}}};
                route.steering.acceptanceRadius = asset.acceptanceRadius;
                route.steering.slowingRadius = asset.slowingRadius;
                route.steering.wantsRun = asset.wantsRun;
                routes_.push_back(std::move(route));
            }
        }

        std::unique_ptr<IAIActionBinding> CreateBinding(
            AIAgentWorldState&, std::vector<GameplayWorldEvent>&) override
        {
            return std::make_unique<AIMoveToActionBinding>(world_, links_, executors_, *this);
        }

        std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override
        {
            for (const auto& route : routes_)
            {
                if (route.context == context.contextId)
                {
                    return AIMoveToActionRequest{&route.graph,
                        GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, route.steering};
                }
            }
            return std::nullopt;
        }

    private:
        struct Route
        {
            AIActionContextId context;
            GameplayRouteGraph graph;
            GameplayArrivalSteeringSettings steering;
        };
        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& links_;
        const GameplayTraversalExecutorRegistry& executors_;
        std::vector<Route> routes_;
    };
}

export namespace rendern
{
    [[nodiscard]] GameplayGOAPCompositionRegistry MakeDefaultGameplayGOAPComponents()
    {
        GameplayGOAPCompositionRegistry registry;
        const bool available = registry.RegisterObservation("target_available",
            [](const auto& asset, const auto& context)
            {
                return goap_components_detail::CompileSpatial(asset, context, false);
            });
        const bool distance = registry.RegisterObservation("within_distance",
            [](const auto& asset, const auto& context)
            {
                return goap_components_detail::CompileSpatial(asset, context, true);
            });
        const bool move = registry.RegisterCapability("move_to", kAIMoveToActionId,
            [](auto assets, const auto& context)
            {
                return std::make_unique<goap_components_detail::MoveToCapability>(assets, context);
            });
        if (!available || !distance || !move)
        {
            throw std::logic_error("Duplicate built-in GOAP component registration");
        }
        return registry;
    }
}
