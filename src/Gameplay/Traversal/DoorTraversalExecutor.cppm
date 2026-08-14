module;

#include <unordered_map>

export module core:door_traversal_executor;

import :gameplay;
import :gameplay_interaction_point;
import :gameplay_object_reservation_system;
import :gameplay_steering;
import :gameplay_traversal_executor;
import :gameplay_traversal_link;
import :character_controller;

export namespace rendern
{
    class DoorTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        static constexpr GameplayTraversalTypeId kTypeId{kDoorTraversalTypeId};

        DoorTraversalExecutor(
            GameplayWorld& world,
            GameplayObjectReservationSystem& reservationSystem,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
            : world_(world)
            , reservationSystem_(reservationSystem)
            , steeringSettings_(steeringSettings)
        {
        }

        [[nodiscard]] GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) override
        {
            const bool bHasValidContext = context.IsValid();
            const bool bIsDoorTraversal = context.traversalTypeId == kDoorTraversalTypeId;
            const bool bHasValidAgent = bHasValidContext && world_.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasValidAgent && world_.HasAI(context.agentEntity);
            const bool bHasMovementComponents = bHasValidAgent && world_.HasTransform(context.agentEntity) &&
                world_.HasCharacterCommand(context.agentEntity) && world_.HasCharacterMotor(context.agentEntity) &&
                world_.HasCharacterMovementState(context.agentEntity);
            const bool bHasValidDoor = bHasValidContext && world_.IsEntityValid(context.targetEntity);
            const bool bIsDoor = bHasValidDoor && world_.HasDoor(context.targetEntity);
            const bool bHasInteractionPoint = bHasValidDoor && world_.HasInteractionPoint(context.targetEntity);
            const bool bHasNoActiveTraversal = !activeByAgent_.contains(context.agentEntity);
            if (!bHasValidContext || !bIsDoorTraversal || !bIsAIAgent || !bHasMovementComponents ||
                !bIsDoor || !bHasInteractionPoint || !bHasNoActiveTraversal)
            {
                return GameplayTraversalExecutionResult::Failed;
            }

            const bool bIsAlreadyReserved = reservationSystem_.IsReserved(context.targetEntity);
            if (bIsAlreadyReserved)
            {
                return GameplayTraversalExecutionResult::Failed;
            }
            if (!reservationSystem_.TryReserve(world_, context.targetEntity, context.agentEntity))
            {
                return GameplayTraversalExecutionResult::Failed;
            }

            const auto interactionPoint = ResolveGameplayInteractionPoint(world_, context.targetEntity);
            if (!interactionPoint)
            {
                (void)reservationSystem_.Release(context.targetEntity, context.agentEntity);
                return GameplayTraversalExecutionResult::Failed;
            }

            activeByAgent_.emplace(context.agentEntity, ActiveDoorTraversal{context, *interactionPoint});
            ClearMovementIfAccessible_(context.agentEntity);
            return GameplayTraversalExecutionResult::Running;
        }

        [[nodiscard]] GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, float) override
        {
            auto it = activeByAgent_.find(context.agentEntity);
            if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context))
            {
                return GameplayTraversalExecutionResult::Failed;
            }

            const bool bHasValidAgent = world_.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasValidAgent && world_.HasAI(context.agentEntity);
            const bool bHasMovementComponents = bHasValidAgent && world_.HasTransform(context.agentEntity) &&
                world_.HasCharacterCommand(context.agentEntity) && world_.HasCharacterMotor(context.agentEntity) &&
                world_.HasCharacterMovementState(context.agentEntity);
            const bool bHasValidDoor = world_.IsEntityValid(context.targetEntity);
            const bool bIsDoor = bHasValidDoor && world_.HasDoor(context.targetEntity);
            const bool bOwnsReservation = reservationSystem_.IsReservedBy(context.targetEntity, context.agentEntity);
            const bool bHasDoorTransform =  bHasValidDoor && world_.HasTransform(context.targetEntity);
            const bool bHasInteractionPoint = bHasValidDoor &&  world_.HasInteractionPoint(context.targetEntity);
            if (!bIsAIAgent
                || !bHasMovementComponents
                || !bIsDoor
                || !bHasDoorTransform
                || !bHasInteractionPoint
                || !bOwnsReservation)   
            {
                ClearMovementIfAccessible_(context.agentEntity);
                if (bOwnsReservation)
                {
                    (void)reservationSystem_.Release(context.targetEntity, context.agentEntity);
                }
                activeByAgent_.erase(it);
                return GameplayTraversalExecutionResult::Failed;
            }

            const GameplayTransformComponent* transform = world_.TryGetTransform(context.agentEntity);
            const GameplaySteeringOutput steering = BuildGameplayArrivalSteering(
                transform->position, it->second.interactionPoint.worldPosition, steeringSettings_);
            if (steering.status == GameplaySteeringStatus::Moving)
            {
                ApplyMovement_(context.agentEntity, steering.movement);
                return GameplayTraversalExecutionResult::Running;
            }

            ClearMovementIfAccessible_(context.agentEntity);
            world_.TryGetCharacterMovementState(context.agentEntity)->desiredFacingYawDegrees =
                it->second.interactionPoint.worldFacingYawDegrees;
            GameplayDoorComponent* door = world_.TryGetDoor(context.targetEntity);
            if (!door->isOpen)
            {
                // This is the authoritative logical open transition for this vertical slice.
                // Visuals, physics, collision, and navigation synchronization remain out of scope.
                door->isOpen = true;
            }
            const bool bReleased = reservationSystem_.Release(context.targetEntity, context.agentEntity);
            activeByAgent_.erase(it);
            return bReleased ? GameplayTraversalExecutionResult::Succeeded : GameplayTraversalExecutionResult::Failed;
        }

        void Cancel(const GameplayTraversalExecutionContext& context) noexcept override
        {
            const auto it = activeByAgent_.find(context.agentEntity);
            if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context))
            {
                return;
            }
            ClearMovementIfAccessible_(context.agentEntity);
            if (reservationSystem_.IsReservedBy(it->second.context.targetEntity, context.agentEntity))
            {
                (void)reservationSystem_.Release(it->second.context.targetEntity, context.agentEntity);
            }
            activeByAgent_.erase(it);
        }

    private:
        struct ActiveDoorTraversal
        {
            GameplayTraversalExecutionContext context{};
            GameplayResolvedInteractionPoint interactionPoint{};
        };

        [[nodiscard]] static bool ContextsMatch_(const GameplayTraversalExecutionContext& left,
            const GameplayTraversalExecutionContext& right) noexcept
        {
            return left.agentEntity == right.agentEntity && left.traversalLink == right.traversalLink &&
                left.traversalTypeId == right.traversalTypeId && left.targetEntity == right.targetEntity;
        }

        void ApplyMovement_(const EntityHandle agent, const GameplayMovementIntent& movement) const noexcept
        {
            GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(agent);
            ApplyGameplayMovementIntent(movement, *command);
            world_.TryGetCharacterMotor(agent)->desiredMoveWorld = command->moveWorld;
            if (movement.IsMoving())
            {
                world_.TryGetCharacterMovementState(agent)->desiredFacingYawDegrees =
                    ExtractGameplayYawDegreesFromDirection(command->moveWorld);
            }
        }

        void ClearMovementIfAccessible_(const EntityHandle agent) const noexcept
        {
            if (!world_.IsEntityValid(agent))
            {
                return;
            }
            if (GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(agent))
            {
                ApplyGameplayMovementIntent(GameplayMovementIntent{}, *command);
            }
            if (GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(agent))
            {
                motor->desiredMoveWorld = {};
                motor->velocity.x = 0.0f;
                motor->velocity.z = 0.0f;
            }
        }

        GameplayWorld& world_;
        GameplayObjectReservationSystem& reservationSystem_;
        GameplayArrivalSteeringSettings steeringSettings_{};
        std::unordered_map<EntityHandle, ActiveDoorTraversal> activeByAgent_{};
    };
}