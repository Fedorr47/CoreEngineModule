module;

#include <memory>
#include <utility>

export module core:ai_flee_target_action;

import :gameplay;
import :ai_action_contracts;
import :ai_system;
import :ai_action_runtime;
import :ai_flee_target_action_runtime;
import :gameplay_obstacle_avoidance;

export namespace rendern
{
    class AIFleeTargetAction
    {
    public:
        [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity,
            const AIFleeTargetSettings& settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {})
        {
            if (!CanCreateRuntime_(world, agentEntity, targetEntity))
            {
                return nullptr;
            }
            return std::make_unique<AIFleeTargetActionRuntime>(
                world, targetEntity, settings, obstacleQuery, obstacleSettings);
        }

        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity,
            const AIFleeTargetSettings& settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {})
        {
            const AIActionRuntimeContext context{
                .agentEntity = agentEntity,
                .actionId = kAIFleeTargetActionId};
            std::unique_ptr<IAIActionRuntime> runtime = CreateRuntime(
            world, agentEntity, targetEntity, settings, obstacleQuery, obstacleSettings);
            if (!context.IsValid() || runtime == nullptr)
            {
                return AIActionExecutionStatus::Failed;
            }
            return aiSystem.StartAction(world, context, std::move(runtime));
        }

    private:
        [[nodiscard]] static bool CanCreateRuntime_(
            const GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity) noexcept
        {
            const bool bHasAgent = world.IsEntityValid(agentEntity);
            const bool bHasTarget = world.IsEntityValid(targetEntity);
            return bHasAgent && world.HasAI(agentEntity) &&
                world.HasTransform(agentEntity) &&
                world.HasCharacterCommand(agentEntity) &&
				world.HasCharacterMotor(agentEntity) &&
   				world.HasCharacterMovementState(agentEntity) && 
				bHasTarget &&
                world.HasTransform(targetEntity);
        }
    };
}