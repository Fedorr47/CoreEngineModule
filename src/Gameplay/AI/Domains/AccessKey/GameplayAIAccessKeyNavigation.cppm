module;

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_ai_access_key_navigation;

import :gameplay;
import :ai_move_to_action_binding;
import :gameplay_ai_access_key_contracts;
import :gameplay_ai_access_key_observation;
import :gameplay_goap_definition_compiler;
import :gameplay_goap_path_inspection;
import :gameplay_route_search;
import :gameplay_traversal_link;

export namespace rendern
{
    inline constexpr GameplayTraversalLinkHandle kAccessKeyGoalJumpTraversalLink{9470001u};
}

export namespace rendern::ai_access_key_detail
{
        inline constexpr GameplayRouteNodeId kStartNode{1u};
        inline constexpr GameplayRouteNodeId kCoinANode{2u};
        inline constexpr GameplayRouteNodeId kCoinBNode{3u};
        inline constexpr GameplayRouteNodeId kCoinCNode{4u};
        inline constexpr GameplayRouteNodeId kAccessKeyNode{5u};
        inline constexpr GameplayRouteNodeId kFinalGoalNode{6u};
        inline constexpr GameplayRouteNodeId kJumpTakeoffNode{7u};
        inline constexpr GameplayRouteNodeId kJumpLandingNode{8u};

        [[nodiscard]] constexpr GameplayRouteNodeId SpatialNode(
            const SpatialLocation location) noexcept
        {
            constexpr std::array nodes{kStartNode, kCoinANode, kCoinBNode, kCoinCNode,
                kAccessKeyNode, kFinalGoalNode};
            return nodes[static_cast<std::size_t>(location)];
        }

        struct MoveTransition
        {
            std::string_view contextName{};
            SpatialLocation source{};
            SpatialLocation target{};
            GameplayRouteNodeId startNodeId{};
            GameplayRouteNodeId goalNodeId{};
        };

        [[nodiscard]] constexpr MoveTransition Transition(
        const std::string_view contextName, const SpatialLocation source, const SpatialLocation target) noexcept
        {
            return MoveTransition{
                .contextName = contextName,
                .source = source,
                .target = target,
                .startNodeId = SpatialNode(source),
                .goalNodeId = SpatialNode(target)
            };
        }

        inline constexpr std::array kMoveTransitions{
            Transition("start_to_coin_a", SpatialLocation::Start, SpatialLocation::CoinA),
            Transition("start_to_coin_b", SpatialLocation::Start, SpatialLocation::CoinB),
            Transition("start_to_coin_c", SpatialLocation::Start, SpatialLocation::CoinC),
            Transition("coin_a_to_coin_b", SpatialLocation::CoinA, SpatialLocation::CoinB),
            Transition("coin_a_to_coin_c", SpatialLocation::CoinA, SpatialLocation::CoinC),
            Transition("coin_a_to_access_key_shop", SpatialLocation::CoinA, SpatialLocation::AccessKeyShop),
            Transition("coin_b_to_coin_a", SpatialLocation::CoinB, SpatialLocation::CoinA),
            Transition("coin_b_to_coin_c", SpatialLocation::CoinB, SpatialLocation::CoinC),
            Transition("coin_b_to_access_key_shop", SpatialLocation::CoinB, SpatialLocation::AccessKeyShop),
            Transition("coin_c_to_coin_a", SpatialLocation::CoinC, SpatialLocation::CoinA),
            Transition("coin_c_to_coin_b", SpatialLocation::CoinC, SpatialLocation::CoinB),
            Transition("coin_c_to_access_key_shop", SpatialLocation::CoinC, SpatialLocation::AccessKeyShop),
            Transition("access_key_shop_to_goal", SpatialLocation::AccessKeyShop, SpatialLocation::Goal)};

        struct ResolvedMoveTransition
        {
            AIActionContextId contextId{};
            SpatialLocation source{};
            SpatialLocation target{};
            GameplayRouteNodeId startNodeId{};
            GameplayRouteNodeId goalNodeId{};
        };

        [[nodiscard]] std::optional<std::vector<ResolvedMoveTransition>>
            ResolveMoveTransitions(const GameplayGOAPCompiledDefinition& compiled)
        {
            std::vector<ResolvedMoveTransition> resolved;
            resolved.reserve(kMoveTransitions.size());
            for (const MoveTransition& transition : kMoveTransitions)
            {
                const std::optional<AIActionContextId> context =
                    compiled.FindActionContext(transition.contextName);
                if (!context.has_value())
                {
                    return std::nullopt;
                }
                resolved.push_back({*context, transition.source, transition.target,
                    transition.startNodeId, transition.goalNodeId});
            }
            return resolved;
        }

        [[nodiscard]] std::optional<AIActionContextId> FindMoveContext(
            const GameplayGOAPCompiledDefinition& compiled,
            const SpatialLocation source, const SpatialLocation target)
        {
            const auto transition = std::ranges::find_if(kMoveTransitions,
                [&](const MoveTransition& candidate)
                {
                    return candidate.source == source && candidate.target == target;
                });
            return transition == kMoveTransitions.end()
                ? std::nullopt : compiled.FindActionContext(transition->contextName);
        }

        class AccessKeyMoveToRequestProvider final :
            public IAIMoveToActionRequestProvider,
            public IAIActionReservationTargetProvider
        {
        public:
            AccessKeyMoveToRequestProvider(GameplayWorld& world, GameplayRouteGraph routeGraph,
                std::vector<ResolvedMoveTransition> transitions)
                 : world_(world), routeGraph_(std::move(routeGraph)),
                transitions_(std::move(transitions)),
                goalRequiresJump_(std::ranges::any_of(routeGraph_.edges,
                [](const GameplayRouteGraphEdge& edge)
                    {
                        return edge.annotation.traversalLink ==
                            kAccessKeyGoalJumpTraversalLink;
                    }))
            {
            }

            void SetCoinEntities(const EntityHandle coinA, const EntityHandle coinB,
                const EntityHandle coinC) noexcept
            {
                coinEntities_ = {coinA, coinB, coinC};
            }

            EntityHandle ResolveReservationTarget(
                const AIActionRuntimeContext& context) override
            {
                const auto transition = std::ranges::find_if(transitions_,
                    [&](const ResolvedMoveTransition& candidate)
                    {
                        return candidate.contextId == context.contextId;
                    });
                if (transition == transitions_.end() ||
                    transition->target < SpatialLocation::CoinA ||
                    transition->target > SpatialLocation::CoinC)
                {
                    return kNullEntity;
                }
                return coinEntities_[static_cast<std::size_t>(transition->target) - 1u];
            }

            std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                const auto transition = std::ranges::find_if(transitions_,
                    [&](const ResolvedMoveTransition& candidate)
                     {
                         return candidate.contextId == context.contextId;
                     });
                if (transition == transitions_.end())
                {
                    return std::nullopt;
                }
                const auto* transform = world_.TryGetTransform(context.agentEntity);
                if (transform == nullptr)
                {
                    return std::nullopt;
                }
                GameplayRouteNodeId nearestNode{};
                float nearestDistanceSquared = std::numeric_limits<float>::max();
                for (const GameplayRouteGraphNode& node : routeGraph_.nodes)
                {
                    const mathUtils::Vec3 delta = transform->position - node.worldPosition;
                    const float distanceSquared = mathUtils::Dot(delta, delta);
                    if (distanceSquared < nearestDistanceSquared)
                    {
                        nearestDistanceSquared = distanceSquared;
                        nearestNode = node.nodeId;
                    }
                }
                if (nearestNode != transition->startNodeId)
                {
                    return std::nullopt;
                }

                const auto sourceNode = std::ranges::find_if(
                    routeGraph_.nodes,
                    [&](const GameplayRouteGraphNode& node)
                    {
                        return node.nodeId == transition->startNodeId;
                    });
                if (sourceNode == routeGraph_.nodes.end())
                {
                    return std::nullopt;
                }

                const mathUtils::Vec3 sourceDelta =
                    transform->position - sourceNode->worldPosition;
                if (mathUtils::Dot(sourceDelta, sourceDelta) >
                    kSpatialArrivalRadiusSquared)
                {
                    return std::nullopt;
                }

                GameplayArrivalSteeringSettings steering{};
                steering.acceptanceRadius = 0.35f;
                steering.slowingRadius = 1.25f;
                steering.wantsRun = goalRequiresJump_ &&
                    transition->source == SpatialLocation::AccessKeyShop &&
                    transition->target == SpatialLocation::Goal;
                //steering.wantsRun = true;
                return AIMoveToActionRequest{&routeGraph_, transition->startNodeId,
                    transition->goalNodeId, steering};
            }

            [[nodiscard]] std::optional<GameplayRoute> BuildDebugRoute(
                const AIActionContextId contextId) const
            {
                const auto transition = std::ranges::find_if(transitions_,
                    [&](const ResolvedMoveTransition& candidate)
                    {
                        return candidate.contextId == contextId;
                    });
                if (transition == transitions_.end())
                {
                    return std::nullopt;
                }
                GameplayRouteSearchResult result = FindWeightedGameplayRoute(
                    routeGraph_, transition->startNodeId, transition->goalNodeId);
                if (!result.Succeeded())
                {
                    return std::nullopt;
                }
                return std::move(result.route);
            }

        private:
            GameplayWorld& world_;
            GameplayRouteGraph routeGraph_{};
            std::vector<ResolvedMoveTransition> transitions_{};
            std::array<EntityHandle, 3> coinEntities_{};
            bool goalRequiresJump_{};
        };

        [[nodiscard]] GameplayAIDebugPlannedPathView BuildPlannedPathDebugView(
            const std::span<const AIPlanStep> selectedPlan,
            const std::optional<std::size_t> currentStepIndex,
            const AccessKeyMoveToRequestProvider& moveToRequests)
        {
            GameplayAIDebugPlannedPathView result{};
            const std::size_t firstStep = currentStepIndex.value_or(selectedPlan.size());
            for (std::size_t index = firstStep; index < selectedPlan.size(); ++index)
            {
                const AIPlanStep& step = selectedPlan[index];
                if (step.actionId != kAIMoveToActionId)
                {
                    continue;
                }
                std::optional<GameplayRoute> route =
                    moveToRequests.BuildDebugRoute(step.contextId);
                if (!route.has_value())
                {
                    result.complete = false;
                }
                result.routeSteps.push_back({index, step.actionId, step.contextId,
                    std::move(route)});
            }
            return result;
        }


        [[nodiscard]] GameplayRouteGraph BuildRouteGraph(
            const mathUtils::Vec3 start, const mathUtils::Vec3 coinA,
            const mathUtils::Vec3 coinB, const mathUtils::Vec3 coinC,
            const mathUtils::Vec3 key, const mathUtils::Vec3 goal,
            const std::optional<mathUtils::Vec3> jumpTakeoff = std::nullopt,
            const std::optional<mathUtils::Vec3> jumpLanding = std::nullopt)
        {
            GameplayRouteGraph graph{};
            graph.nodes = {{kStartNode, start}, {kCoinANode, coinA}, {kCoinBNode, coinB},
                {kCoinCNode, coinC}, {kAccessKeyNode, key}, {kFinalGoalNode, goal}};
            const bool goalRequiresJump = jumpTakeoff.has_value() && jumpLanding.has_value();
            if (goalRequiresJump)
            {
                graph.nodes.push_back({kJumpTakeoffNode, *jumpTakeoff});
                graph.nodes.push_back({kJumpLandingNode, *jumpLanding});
            }
            for (const MoveTransition& transition : kMoveTransitions)
            {
                if (goalRequiresJump && transition.source == SpatialLocation::AccessKeyShop &&
                    transition.target == SpatialLocation::Goal)
                {
                    graph.edges.push_back({kAccessKeyNode, kJumpTakeoffNode,
                        mathUtils::Length(*jumpTakeoff - key)});
                    graph.edges.push_back({kJumpTakeoffNode, kJumpLandingNode,
                        mathUtils::Length(*jumpLanding - *jumpTakeoff),
                        {.traversalLink = kAccessKeyGoalJumpTraversalLink}});
                    graph.edges.push_back({kJumpLandingNode, kFinalGoalNode,
                        mathUtils::Length(goal - *jumpLanding)});
                    continue;
                }
                const mathUtils::Vec3 delta =
                    graph.nodes[static_cast<std::size_t>(transition.target)].worldPosition -
                    graph.nodes[static_cast<std::size_t>(transition.source)].worldPosition;
                graph.edges.push_back({transition.startNodeId, transition.goalNodeId,
                    std::sqrt(mathUtils::Dot(delta, delta))});
            }
            return graph;
        }

        [[nodiscard]] AccessKeyMoveToRequestProvider BuildMoveToProvider(
            GameplayWorld& world, GameplayRouteGraph routeGraph,
            std::vector<ResolvedMoveTransition> transitions)
        {
            return AccessKeyMoveToRequestProvider{
                world, std::move(routeGraph), std::move(transitions)};
        }

}
