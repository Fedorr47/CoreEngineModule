module;

#include <utility>

export module core:ai_move_to_action;

import :gameplay;
import :ai_action_contracts;
import :ai_system;
import :ai_follow_route_action;
import :ai_action_runtime;
import :gameplay_route_search;
import :gameplay_steering;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor_registry;

export namespace rendern
{
    // MoveTo is the semantic action. FollowRoute remains a distinct, reusable
    // execution primitive used after MoveTo has resolved a route.
    inline constexpr AIActionId kAIMoveToActionId{2u};

    struct AIMoveToActionRequest
    {
        // Non-owning. The graph must remain alive only through synchronous
        // runtime creation; the resolved GameplayRoute is runtime-owned.
        const GameplayRouteGraph* routeGraph{nullptr};
        GameplayRouteNodeId startNodeId{};
        GameplayRouteNodeId goalNodeId{};
        GameplayArrivalSteeringSettings steeringSettings{};
    };
    
    class AIMoveToAction
    {
    public:
        [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const AIMoveToActionRequest& request,
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {})
        {
            if (context.actionId != kAIMoveToActionId || request.routeGraph == nullptr)
            {
                return nullptr;
            }

            GameplayRouteSearchResult searchResult = FindWeightedGameplayRoute(
                *request.routeGraph, request.startNodeId, request.goalNodeId);
            if (!searchResult.Succeeded())
            {
                return nullptr;
            }

            return AIFollowRouteAction::CreateRuntime(
                world,
                traversalLinkRegistry,
                traversalExecutorRegistry,
                context.agentEntity,
                std::move(searchResult.route),
                request.steeringSettings,
                obstacleQuery,
                obstacleSettings);
        }
        
        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const EntityHandle agentEntity,
            const GameplayRouteGraph& routeGraph,
            const GameplayRouteNodeId startNodeId,
            const GameplayRouteNodeId goalNodeId,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {})
        {
            const AIActionRuntimeContext context{
                .agentEntity = agentEntity,
                .actionId = kAIMoveToActionId};
            
            const AIMoveToActionRequest request{
                .routeGraph = &routeGraph,
                .startNodeId = startNodeId,
                .goalNodeId = goalNodeId,
                .steeringSettings = steeringSettings};
            
            std::unique_ptr<IAIActionRuntime> runtime = CreateRuntime(
                context, world, traversalLinkRegistry, traversalExecutorRegistry, request,
                obstacleQuery, obstacleSettings);
            if (runtime == nullptr)
            {
                return AIActionExecutionStatus::Failed;
            }
            
            return aiSystem.StartAction(world, context, std::move(runtime));
        }
    };
}