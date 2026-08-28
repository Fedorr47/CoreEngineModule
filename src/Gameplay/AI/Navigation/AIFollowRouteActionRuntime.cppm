module;

#include <optional>
#include <utility>

export module core:ai_follow_route_action_runtime;

import :gameplay;
import :ai_action_contracts;
import :ai_action_runtime;
import :gameplay_route;
import :gameplay_route_follower;
import :gameplay_steering;
import :character_controller;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor;
import :gameplay_traversal_executor_registry;

export namespace rendern
{
    inline constexpr AIActionId kAIFollowRouteActionId{1u};
    
    class AIFollowRouteActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFollowRouteActionRuntime(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            GameplayRoute route,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
        : world_(world)
        , traversalLinkRegistry_(traversalLinkRegistry)
        , traversalExecutorRegistry_(traversalExecutorRegistry)
        , route_(std::move(route))
        , steeringSettings_(steeringSettings)
        {
        }
        
        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            physicallyBlocked_ = false;
            if (!ValidateContextAndAgent_(context) || !route_.IsValid())
            {
                StopMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }
            
            const GameplayRouteFollowerStatus followerStatus = follower_.Start(std::move(route_), steeringSettings_);
            if (followerStatus == GameplayRouteFollowerStatus::Following)
            {
                return AIActionRuntimeResult::Running;
            }
            StopMovementIfAccessible_(context.agentEntity);
            return followerStatus == GameplayRouteFollowerStatus::Succeeded
                ? AIActionRuntimeResult::Succeeded
                : AIActionRuntimeResult::Failed;
        }
        
        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            if (!ValidateContextAndAgent_(context))
            {
                StopMovementIfAccessible_(context.agentEntity);
                CancelActiveTraversal_();
                follower_.Reset();
                physicallyBlocked_ = false;
                return AIActionRuntimeResult::Failed;
            }
            
            if (activeTraversal_)
            {
                physicallyBlocked_ = false;
                const GameplayTraversalLinkHandle completedTraversal = activeTraversal_->context.traversalLink;
                const GameplayTraversalExecutionResult result = activeTraversal_->executor->Tick(activeTraversal_->context, deltaSeconds);
                if (result == GameplayTraversalExecutionResult::Running)
                {
                    return AIActionRuntimeResult::Running;
                }
                
                activeTraversal_.reset();
                if (result == GameplayTraversalExecutionResult::Failed 
                    || !follower_.CompleteTraversal(completedTraversal))
                {
                    StopMovementIfAccessible_(context.agentEntity);
                    physicallyBlocked_ = false;
                    return AIActionRuntimeResult::Failed;
                }
            }
            
            return AdvanceFollower_(context.agentEntity);
        }
        
        [[nodiscard]] bool IsPhysicallyBlocked() const noexcept
        {
            return physicallyBlocked_;
        }
        
        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            CancelActiveTraversal_();
            StopMovementIfAccessible_(context.agentEntity);
            follower_.Reset();
            physicallyBlocked_ = false;
        }
        
    private:
        struct ActiveTraversalState
        {
            GameplayTraversalExecutionContext context{};
            IGameplayTraversalExecutor* executor{nullptr};

            [[nodiscard]] bool IsValid() const noexcept
            {
                return context.IsValid() && executor != nullptr;
            }
        };

        [[nodiscard]] AIActionRuntimeResult AdvanceFollower_(const EntityHandle entity)
        {
            const GameplayTransformComponent* transform = world_.TryGetTransform(entity);
            const GameplayCharacterMovementStateComponent* movementState =
                world_.TryGetCharacterMovementState(entity);
            physicallyBlocked_ = movementState->physicallyBlocked;
            const GameplayRouteFollowerOutput output = follower_.Tick(transform->position);
            switch (output.status)
            {
            case GameplayRouteFollowerStatus::Following:
                ApplyFollowerMovement_(entity, output.movement);
                return AIActionRuntimeResult::Running;
            case GameplayRouteFollowerStatus::TraversalRequired:
                return StartTraversal_(entity, output.requiredTraversalLink);
            case GameplayRouteFollowerStatus::Succeeded:
                StopMovementIfAccessible_(entity);
                physicallyBlocked_ = false;
                return AIActionRuntimeResult::Succeeded;
            case GameplayRouteFollowerStatus::InvalidRoute:
            case GameplayRouteFollowerStatus::NotStarted:
            default:
                StopMovementIfAccessible_(entity);
                physicallyBlocked_ = false;
                return AIActionRuntimeResult::Failed;
            }
        }

        void ApplyFollowerMovement_(
            const EntityHandle entity,
            const GameplayMovementIntent& movement) const noexcept
        {
            GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(entity);
            GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(entity);
            GameplayCharacterMovementStateComponent* movementState =
                world_.TryGetCharacterMovementState(entity);
            ApplyGameplayMovementIntent(movement, *command);
            motor->desiredMoveWorld = command->moveWorld;
            if (movement.IsMoving())
            {
                movementState->desiredFacingYawDegrees =
                    ExtractGameplayYawDegreesFromDirection(command->moveWorld);
            }
        }

        [[nodiscard]] AIActionRuntimeResult StartTraversal_(
            const EntityHandle entity,
            const std::optional<GameplayTraversalLinkHandle>& requiredLink)
        {
            if (!requiredLink || !requiredLink->IsValid())
            {
                return AIActionRuntimeResult::Failed;
            }
            const std::optional<GameplayTraversalLink> link = traversalLinkRegistry_.Find(*requiredLink);
            if (!link || !link->IsValid() || !world_.IsEntityValid(link->targetEntity))
            {
                return AIActionRuntimeResult::Failed;
            }
            IGameplayTraversalExecutor* executor = traversalExecutorRegistry_.Find(link->traversalTypeId);
            if (executor == nullptr)
            {
                return AIActionRuntimeResult::Failed;
            }
            const GameplayTraversalExecutionContext traversalContext{
                .agentEntity = entity,
                .traversalLink = link->handle,
                .traversalTypeId = link->traversalTypeId,
                .targetEntity = link->targetEntity
            };
            if (!traversalContext.IsValid())
            {
                return AIActionRuntimeResult::Failed;
            }
            
            const GameplayTraversalExecutionResult result = executor->Start(traversalContext);
            if (result == GameplayTraversalExecutionResult::Running)
            {
                activeTraversal_ = ActiveTraversalState{
                    .context = traversalContext,
                    .executor = executor
                };
                return AIActionRuntimeResult::Running;
            }
            if (result == GameplayTraversalExecutionResult::Failed ||
                !follower_.CompleteTraversal(traversalContext.traversalLink))
            {
                return AIActionRuntimeResult::Failed;
            }

            // Defer progression after synchronous completion to the next action
            // tick so arbitrarily long traversal chains cannot spin in one frame.
            return AIActionRuntimeResult::Running;
        }

        void CancelActiveTraversal_() noexcept
        {
            if (activeTraversal_)
            {
                activeTraversal_->executor->Cancel(activeTraversal_->context);
                activeTraversal_.reset();
            }
        }
        
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
        
        void ClearMovementIntentIfAccessible_(const EntityHandle entity) const noexcept
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
            }
        }
        
        void StopMovementIfAccessible_(const EntityHandle entity) const noexcept
        {
            ClearMovementIntentIfAccessible_(entity);
            if (GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(entity))
            {
                // Route movement is planar. Preserve vertical motion on terminal cleanup.
                motor->velocity.x = 0.0f;
                motor->velocity.z = 0.0f;
            }
        }
        
        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& traversalLinkRegistry_;
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry_;
        GameplayRoute route_{};
        GameplayArrivalSteeringSettings steeringSettings_{};
        bool physicallyBlocked_{ false };
        GameplayRouteFollower follower_{};
        std::optional<ActiveTraversalState> activeTraversal_{};
    };
}