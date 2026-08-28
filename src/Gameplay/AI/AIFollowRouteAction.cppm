module;

#include <memory>
#include <utility>

export module core:ai_follow_route_action;

import :gameplay;
import :ai_action_contracts;
import :ai_system;
import :ai_action_runtime;
import :ai_follow_route_action_runtime;
import :gameplay_route;
import :gameplay_steering;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor_registry;

export namespace rendern
{
	class AIFollowRouteAction
	{
	public:
		[[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
			GameplayWorld& world,
			const GameplayTraversalLinkRegistry& traversalLinkRegistry,
			const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
			const EntityHandle agentEntity,
			GameplayRoute route,
			const GameplayArrivalSteeringSettings& steeringSettings = {})
		{
			if (!CanCreateRuntime_(world, agentEntity, route))
			{
				return nullptr;
			}
			
			return std::make_unique<AIFollowRouteActionRuntime>(
				world,
				traversalLinkRegistry,
				traversalExecutorRegistry,
				std::move(route),
				steeringSettings);
		}

		[[nodiscard]] static AIActionExecutionStatus Start(
			AISystem& aiSystem,
			GameplayWorld& world,
			const GameplayTraversalLinkRegistry& traversalLinkRegistry,
			const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
			const EntityHandle agentEntity,
			GameplayRoute route,
			const GameplayArrivalSteeringSettings& steeringSettings = {})
		{
			
			const AIActionRuntimeContext context
			{
				.agentEntity = agentEntity,
				.actionId =  kAIFollowRouteActionId
			};
			if (!context.IsValid())
			{
				return AIActionExecutionStatus::Failed;
			}
			
			std::unique_ptr<IAIActionRuntime> runtime = CreateRuntime(
				world,
				traversalLinkRegistry,
				traversalExecutorRegistry,
				agentEntity,
				std::move(route),
				steeringSettings);
			if (runtime == nullptr)
			{
				return AIActionExecutionStatus::Failed;
			}
			
			return aiSystem.StartAction(world, context, std::move(runtime));
		}
	private:
		[[nodiscard]] static bool CanCreateRuntime_(
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