module;

#include <utility>

export module core:ai_move_to_action;

import :gameplay;
import :ai_system;
import :ai_follow_route_action;
import :gameplay_route_search;
import :gameplay_steering;

export namespace rendern
{
    class AIMoveToAction
    {
    public:
        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const GameplayRouteGraph& routeGraph,
            const GameplayRouteNodeId startNodeId,
            const GameplayRouteNodeId goalNodeId,
            const GameplayArrivalSteeringSettings& steeringSettings = {})
        {
            GameplayRouteSearchResult searchResult = FindWeightedGameplayRoute(routeGraph, startNodeId, goalNodeId);  
            if (!searchResult.Succeeded())
            {
                return AIActionExecutionStatus::Failed;
            }
            
            return AIFollowRouteAction::Start(
                aiSystem, 
                world, 
                agentEntity, 
                std::move(searchResult.route), 
                steeringSettings);
        }
    };
}