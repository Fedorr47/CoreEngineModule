module;

#include <memory>
#include <utility>

export module core:ai_follow_route_action;

import :gameplay;
import :ai_system;
import :ai_follow_route_action_runtime;
import :gameplay_route;
import :gameplay_steering;

export namespace rendern
{
	class AIFollowRouteAction
	{
	public:
		[[nodiscard]] static AIActionExecutionStatus Start(
			AISystem& aiSystem,
			GameplayWorld& world,
			const EntityHandle agentEntity,
			GameplayRoute route,
			const GameplayArrivalSteeringSettings& steeringSettings = {})
		{
			if (!CanStart_(world, agentEntity, route))
			{
				return AIActionExecutionStatus::Failed;
			}
			
			const AIActionRuntimeContext context
			{
				.agentEntity = agentEntity,
				.actionId =  kAIFollowRouteActionId
			};
			if (!context.IsValid())
			{
				return AIActionExecutionStatus::Failed;
			}
			
			auto runtime = std::make_unique<AIFollowRouteActionRuntime>(
				world,
				std::move(route),
				steeringSettings);
			
			return aiSystem.StartAction(world, context, std::move(runtime));
		}
	private:
		[[nodiscard]] static bool CanStart_(
		   const GameplayWorld& world,
		   const EntityHandle agentEntity,
		   const GameplayRoute& route) noexcept
		{
			const bool bHasValidEntity = world.IsEntityValid(agentEntity);
			const bool bIsAIAgent  = bHasValidEntity && world.HasAI(agentEntity);
			const bool bHasValidRoute = route.IsValid();
			const bool bHasTransform = bHasValidEntity && world.HasTransform(agentEntity);
			const bool bHasCommand = bHasValidEntity && world.HasCharacterCommand(agentEntity);
			const bool bHasMotor = bHasValidEntity && world.HasCharacterMotor(agentEntity);
			const bool bHasMovementState = bHasValidEntity && world.HasCharacterMovementState(agentEntity);
			const bool bHasRequiredMovementComponents = 
				bHasTransform && bHasCommand && bHasMotor && bHasMovementState;
            
			return bHasValidEntity && bIsAIAgent && bHasValidRoute && bHasRequiredMovementComponents;
		}
	};
}