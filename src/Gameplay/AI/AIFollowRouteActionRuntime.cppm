module;

#include <utility>

export module core:ai_follow_route_action_runtime;

import :gameplay;
import :ai_action_runtime;
import :gameplay_route;
import :gameplay_route_follower;
import :gameplay_steering;
import :character_controller;

export namespace rendern 
{
    inline constexpr AIActionId kAIFollowRouteActionId{1u};
    
    class AIFollowRouteActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFollowRouteActionRuntime(
            GameplayWorld& world,
            GameplayRoute route,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
        : world_(world)
        , route_(std::move(route))
        , steeringSettings_(steeringSettings)
        {
        }
        
        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            if (!ValidateContextAndAgent_(context) || !route_.IsValid())
            {
                ClearMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }
            
            const GameplayRouteFollowerStatus followerStatus = follower_.Start(std::move(route_), steeringSettings_);
            return MapFollowerStatus_(context.agentEntity, followerStatus);
        }
        
        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            [[maybe_unused]] const float deltaSeconds) override
        {
            if (!ValidateContextAndAgent_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                follower_.Reset();
                return AIActionRuntimeResult::Failed;
            }
            
            const GameplayTransformComponent* transform = world_.TryGetTransform(context.agentEntity);
            const GameplayRouteFollowerOutput output = follower_.Tick(transform->position);
            if (output.status == GameplayRouteFollowerStatus::Following)
            {
                GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(context.agentEntity);
                GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(context.agentEntity);
                GameplayCharacterMovementStateComponent* movementState = world_.TryGetCharacterMovementState(context.agentEntity);
                ApplyGameplayMovementIntent(output.movement, *command);
                motor->desiredMoveWorld = command->moveWorld;
                if (output.movement.IsMoving())
                {
                    movementState->desiredFacingYawDegrees = ExtractGameplayYawDegreesFromDirection(command->moveWorld);
                }
                return AIActionRuntimeResult::Running;
            }
            
            return MapFollowerStatus_(context.agentEntity, output.status);
        }
        
        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            ClearMovementIfAccessible_(context.agentEntity);
            follower_.Reset();
        }
        
    private:
        
        [[nodiscard]] bool ValidateContextAndAgent_(const AIActionRuntimeContext& context) const noexcept
        {
            const bool bHasContext = context.IsValid();
            const bool bHasEntity = world_.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasEntity && world_.HasAI(context.agentEntity);
            const bool bHasTransform = bHasEntity && world_.HasTransform(context.agentEntity);
            const bool bHasCommand = bHasEntity && world_.HasCharacterCommand(context.agentEntity);
            const bool bHasMotor = bHasEntity && world_.HasCharacterMotor(context.agentEntity);
            const bool bHasMovementState = bHasEntity && world_.HasCharacterMovementState(context.agentEntity);
            
            return bHasContext && bHasEntity && bIsAIAgent && 
                bHasTransform && bHasCommand && bHasMotor && bHasMovementState;
        }
        
        void ClearMovementIfAccessible_(const EntityHandle entity) const noexcept
        {
            if (!world_.IsEntityValid(entity))
            {
                return;
            }

            if (GameplayCharacterCommandComponent* command =
                    world_.TryGetCharacterCommand(entity))
            {
                ApplyGameplayMovementIntent(
                    GameplayMovementIntent{},
                    *command);
            }

            if (GameplayCharacterMotorComponent* motor =
                    world_.TryGetCharacterMotor(entity))
            {
                motor->desiredMoveWorld = {};

                // Route movement is planar. Clear the remaining planar velocity so a
                // completed or cancelled action cannot leave locomotion active.
                motor->velocity.x = 0.0f;
                motor->velocity.z = 0.0f;
            }
        }
        
        [[nodiscard]] AIActionRuntimeResult MapFollowerStatus_(
            const EntityHandle entity,
            const GameplayRouteFollowerStatus status) const noexcept
        {
            switch (status)
            {
            case GameplayRouteFollowerStatus::Following:
                return AIActionRuntimeResult::Running;
            case GameplayRouteFollowerStatus::Succeeded:
                ClearMovementIfAccessible_(entity);
                return AIActionRuntimeResult::Succeeded;
            case GameplayRouteFollowerStatus::TraversalRequired:
            case GameplayRouteFollowerStatus::InvalidRoute:
            case GameplayRouteFollowerStatus::NotStarted:
            default:
                ClearMovementIfAccessible_(entity);
                return AIActionRuntimeResult::Failed;
            }
        }
        
        GameplayWorld& world_;
        GameplayRoute route_{};
        GameplayArrivalSteeringSettings steeringSettings_{};
        GameplayRouteFollower follower_{};
    };
}