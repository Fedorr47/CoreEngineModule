module;

#include <cmath>
#include <optional>
#include <unordered_map>

export module core:jump_traversal_executor;

import :gameplay;
import :gameplay_steering;
import :gameplay_traversal_executor;
import :gameplay_traversal_link;
import :gameplay_traversal_link_registry;
import :character_controller;

export namespace rendern
{
    class JumpTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        static constexpr GameplayTraversalTypeId kTypeId{kJumpTraversalTypeId};

        explicit JumpTraversalExecutor(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& linkRegistry,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
            : world_(world), linkRegistry_(linkRegistry), steeringSettings_(steeringSettings)
        {
        }

        [[nodiscard]] GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) override
        {
            const auto link = ResolveExpectedLink_(context);
            if (!link || !HasRequiredAgentState_(context.agentEntity) ||
                activeByAgent_.contains(context.agentEntity))
            {
                return GameplayTraversalExecutionResult::Failed;
            }
            const GameplayCharacterCommandComponent* command =
                world_.TryGetCharacterCommand(context.agentEntity);
            activeByAgent_.emplace(context.agentEntity, ActiveJump{
                .context = context,
                .data = link->jump,
                .wantsRun = command->wantsRun});
            return GameplayTraversalExecutionResult::Running;
        }

        [[nodiscard]] GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, const float deltaSeconds) override
        {
            auto it = activeByAgent_.find(context.agentEntity);
            if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context) ||
                !ResolveExpectedLink_(context) || !HasRequiredAgentState_(context.agentEntity))
            {
                return Fail_(it, context.agentEntity);
            }

            ActiveJump& active = it->second;
            if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f)
            {
                return Fail_(it, context.agentEntity);
            }
            active.elapsedSeconds += deltaSeconds;
            if (active.elapsedSeconds > kTraversalTimeoutSeconds)
            {
                return Fail_(it, context.agentEntity);
            }

            const GameplayTransformComponent* transform = world_.TryGetTransform(context.agentEntity);
            GameplayCharacterMovementStateComponent* movementState =
                world_.TryGetCharacterMovementState(context.agentEntity);
            GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(context.agentEntity);
            GameplayActionComponent* action = world_.TryGetAction(context.agentEntity);

            if (!active.requestIssued)
            {
                GameplayArrivalSteeringSettings settings = steeringSettings_;
                settings.wantsRun = active.wantsRun;
                settings.acceptanceRadius = active.data.takeoffTolerance;
                // The takeoff is a trigger to cross with momentum, not a destination to stop at.
                // Collapsing the slowing interval keeps full movement intent until acceptance.
                settings.slowingRadius = active.data.takeoffTolerance;
                const GameplaySteeringOutput steering = BuildGameplayArrivalSteering(
                    transform->position, active.data.takeoffPosition, settings);
                if (steering.status == GameplaySteeringStatus::Moving)
                {
                    ApplyMovement_(context.agentEntity, steering.movement);
                    return GameplayTraversalExecutionResult::Running;
                }

                ApplyLandingMovement_(context.agentEntity, transform->position, active);
                if (!movementState->grounded || movementState->jumpPhase != GameplayJumpPhase::None)
                {
                    return Fail_(it, context.agentEntity);
                }
                const GameplayActionRequest request{
                    .id = kGameplayActionJump,
                    .source = GameplayActionRequestSource::Script,
                    .priority = 200};
                if (!QueueGameplayActionRequest(*action, request))
                {
                    return Fail_(it, context.agentEntity);
                }
                active.previousVerticalSpeed = motor->jumpVerticalSpeed;
                motor->jumpVerticalSpeed = active.data.verticalSpeed;
                movementState->jumpRequestResult = GameplayJumpRequestResult::Pending;
                active.requestIssued = true;
                return GameplayTraversalExecutionResult::Running;
            }

            if (!active.takeoffObserved)
            {
                ApplyLandingMovement_(context.agentEntity, transform->position, active);
                if (movementState->jumpRequestResult == GameplayJumpRequestResult::Rejected)
                {
                    return Fail_(it, context.agentEntity);
                }
                if (movementState->jumpPhase == GameplayJumpPhase::Airborne &&
                    !movementState->grounded)
                {
                    active.takeoffObserved = true;
                    motor->jumpVerticalSpeed = active.previousVerticalSpeed;
                    return GameplayTraversalExecutionResult::Running;
                }
                return GameplayTraversalExecutionResult::Running;
            }

            if (!movementState->grounded || movementState->jumpPhase == GameplayJumpPhase::Airborne)
            {
                return GameplayTraversalExecutionResult::Running;
            }

            const bool validHorizontalLanding = PlanarDistanceSquared_(
                transform->position, active.data.landingPosition) <=
                active.data.landingHorizontalTolerance * active.data.landingHorizontalTolerance;
            const bool validVerticalLanding = std::abs(
                transform->position.y - active.data.landingPosition.y) <=
                active.data.landingVerticalTolerance;
            ClearMovement_(context.agentEntity);
            activeByAgent_.erase(it);
            return validHorizontalLanding && validVerticalLanding
                ? GameplayTraversalExecutionResult::Succeeded
                : GameplayTraversalExecutionResult::Failed;
        }

        void Cancel(const GameplayTraversalExecutionContext& context) noexcept override
        {
            const auto it = activeByAgent_.find(context.agentEntity);
            if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context))
            {
                return;
            }
            RestoreVerticalSpeed_(context.agentEntity, it->second);
            CleanupIssuedRequest_(context.agentEntity, it->second);
            ClearMovement_(context.agentEntity);
            activeByAgent_.erase(it);
        }

    private:
        struct ActiveJump
        {
            GameplayTraversalExecutionContext context{};
            GameplayJumpTraversalData data{};
            float elapsedSeconds{0.0f};
            float previousVerticalSpeed{0.0f};
            bool requestIssued{false};
            bool takeoffObserved{false};
            bool wantsRun{false};
        };
        using ActiveIterator = std::unordered_map<EntityHandle, ActiveJump>::iterator;
        static constexpr float kTraversalTimeoutSeconds = 10.0f;

        [[nodiscard]] std::optional<GameplayTraversalLink> ResolveExpectedLink_(
            const GameplayTraversalExecutionContext& context) const noexcept
        {
            const auto link = linkRegistry_.Find(context.traversalLink);
            if (!link || context.traversalTypeId != kJumpTraversalTypeId ||
                link->traversalTypeId != kJumpTraversalTypeId ||
                link->targetEntity != context.targetEntity || !link->jump.IsValid())
            {
                return std::nullopt;
            }
            return link;
        }

        [[nodiscard]] bool HasRequiredAgentState_(const EntityHandle agent) const noexcept
        {
            const GameplayPhysicsCharacterComponent* physicsCharacter = world_.TryGetPhysicsCharacter(agent);
            return world_.IsEntityValid(agent) && world_.HasAI(agent) && world_.HasTransform(agent) &&
                world_.HasCharacterCommand(agent) && world_.HasCharacterMotor(agent) &&
                world_.HasCharacterMovementState(agent) && world_.HasAction(agent) &&
                physicsCharacter != nullptr && physicsCharacter->character.IsValid();
        }

        [[nodiscard]] GameplayTraversalExecutionResult Fail_(
            const ActiveIterator it, const EntityHandle agent) noexcept
        {
            if (it != activeByAgent_.end())
            {
                RestoreVerticalSpeed_(agent, it->second);
                CleanupIssuedRequest_(agent, it->second);
                activeByAgent_.erase(it);
            }
            ClearMovement_(agent);
            return GameplayTraversalExecutionResult::Failed;
        }

        static bool ContextsMatch_(const GameplayTraversalExecutionContext& left,
            const GameplayTraversalExecutionContext& right) noexcept
        {
            return left.agentEntity == right.agentEntity && left.traversalLink == right.traversalLink &&
                left.traversalTypeId == right.traversalTypeId && left.targetEntity == right.targetEntity;
        }

        static float PlanarDistanceSquared_(const mathUtils::Vec3& a, const mathUtils::Vec3& b) noexcept
        {
            const float x = a.x - b.x;
            const float z = a.z - b.z;
            return x * x + z * z;
        }

        void ApplyMovement_(const EntityHandle agent, const GameplayMovementIntent& movement) noexcept
        {
            GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(agent);
            ApplyGameplayMovementIntent(movement, *command);
            world_.TryGetCharacterMotor(agent)->desiredMoveWorld = command->moveWorld;
        }

        void ApplyLandingMovement_(const EntityHandle agent, const mathUtils::Vec3& position,
            const ActiveJump& active) noexcept
        {
            GameplayArrivalSteeringSettings settings = steeringSettings_;
            settings.acceptanceRadius = 0.0f;
            const GameplaySteeringOutput steering = BuildGameplayArrivalSteering(
            position, active.data.landingPosition, settings);
            if (steering.status == GameplaySteeringStatus::Moving)
            {
                ApplyMovement_(agent, steering.movement);
            }
        }

        void ClearMovement_(const EntityHandle agent) noexcept
        {
            if (GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(agent))
            {
                ApplyGameplayMovementIntent({}, *command);
            }
            if (GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(agent))
            {
                motor->desiredMoveWorld = {};
            }
        }

        void RestoreVerticalSpeed_(const EntityHandle agent, const ActiveJump& active) noexcept
        {
            if (active.requestIssued && !active.takeoffObserved)
            {
                if (GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(agent))
                {
                    motor->jumpVerticalSpeed = active.previousVerticalSpeed;
                }
            }
        }

        void CleanupIssuedRequest_(const EntityHandle agent, const ActiveJump& active) noexcept
        {
            if (!active.requestIssued)
            {
                return;
            }
            if (GameplayActionComponent* action = world_.TryGetAction(agent);
                action != nullptr && action->pending.id == kGameplayActionJump &&
                action->pending.source == GameplayActionRequestSource::Script)
            {
                ClearGameplayActionRequest(action->pending);
                action->pendingDispatched = false;
            }
            if (GameplayCharacterMovementStateComponent* movementState =
                    world_.TryGetCharacterMovementState(agent);
                movementState != nullptr &&
                movementState->jumpRequestResult == GameplayJumpRequestResult::Pending)
            {
                movementState->jumpRequestResult = GameplayJumpRequestResult::None;
            }
        }

        GameplayWorld& world_;
        const GameplayTraversalLinkRegistry& linkRegistry_;
        GameplayArrivalSteeringSettings steeringSettings_{};
        std::unordered_map<EntityHandle, ActiveJump> activeByAgent_{};
    };
}
