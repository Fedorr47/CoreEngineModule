module;

#include <algorithm>
#include <cmath>

export module core:ai_flee_target_action_runtime;

import :gameplay;
import :ai_action_contracts;
import :ai_action_runtime;
import :gameplay_steering;
import :gameplay_obstacle_avoidance;
import :gameplay_steering_debug;
import :math_utils;

export namespace rendern
{
    inline constexpr AIActionId kAIFleeTargetActionId{5u};

    struct AIFleeTargetSettings
    {
        GameplayFleeSteeringSettings steering{};
        float triggerRadius{2.0f};
        float safeRadius{3.0f};
        float steeringUpdateIntervalSeconds{0.05f};
    };

    class AIFleeTargetActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFleeTargetActionRuntime(
            GameplayWorld& world,
            const EntityHandle targetEntity,
            AIFleeTargetSettings settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            GameplayObstacleAvoidanceSettings obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr) noexcept
        : world_(world)
        , targetEntity_(targetEntity)
        , settings_(settings)
        , obstacleQuery_(obstacleQuery)
        , obstacleSettings_(obstacleSettings)
        , debugRegistry_(debugRegistry)
        {
            settings_.triggerRadius = std::max(settings_.triggerRadius, 0.0f);
            settings_.safeRadius = std::max(settings_.safeRadius, settings_.triggerRadius);
            settings_.steeringUpdateIntervalSeconds =
                std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
        }

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            elapsedSinceSteeringUpdate_ = 0.0f;
            isFleeing_ = false;
            obstacleAvoidanceState_ = {};
            if (debugRegistry_ != nullptr)
            {
                debugRegistry_->Clear(context.agentEntity);
            }
            if (!Validate_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }

            RefreshSteering_(context.agentEntity);
            ApplyCurrentMovement_(context.agentEntity);
            return AIActionRuntimeResult::Running;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            if (!Validate_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                isFleeing_ = false;
                return AIActionRuntimeResult::Failed;
            }

            if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
            {
                elapsedSinceSteeringUpdate_ += deltaSeconds;
            }

            if (settings_.steeringUpdateIntervalSeconds <= 0.0f ||
                elapsedSinceSteeringUpdate_ >= settings_.steeringUpdateIntervalSeconds)
            {
                RefreshSteering_(context.agentEntity);
                elapsedSinceSteeringUpdate_ = settings_.steeringUpdateIntervalSeconds > 0.0f
                    ? std::fmod(elapsedSinceSteeringUpdate_, settings_.steeringUpdateIntervalSeconds)
                    : 0.0f;
            }

            // GameplayCharacterCommandComponent is frame-local and is cleared
            // by GameplayRuntime::BeginFrame(). Keep the steering sample at its
            // configured cadence while re-issuing the cached intent every tick.
            ApplyCurrentMovement_(context.agentEntity);
            return AIActionRuntimeResult::Running;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            ClearMovementIfAccessible_(context.agentEntity);
            elapsedSinceSteeringUpdate_ = 0.0f;
            isFleeing_ = false;
        }

    private:
        [[nodiscard]] bool Validate_(const AIActionRuntimeContext& context) const noexcept
        {
            const bool bHasAgent = world_.IsEntityValid(context.agentEntity);
            const bool bHasTarget = world_.IsEntityValid(targetEntity_);
            return context.IsValid() && bHasAgent && world_.HasAI(context.agentEntity) &&
                world_.HasTransform(context.agentEntity) &&
                world_.HasCharacterCommand(context.agentEntity) &&
                world_.HasCharacterMotor(context.agentEntity) &&
                world_.HasCharacterMovementState(context.agentEntity) && 
                bHasTarget &&
                world_.HasTransform(targetEntity_);
        }

        void RefreshSteering_(const EntityHandle agentEntity) noexcept
        {
            const mathUtils::Vec3 agentPosition = world_.TryGetTransform(agentEntity)->position;
            const mathUtils::Vec3 targetPosition = world_.TryGetTransform(targetEntity_)->position;
            mathUtils::Vec3 planarDelta = targetPosition - agentPosition;
            planarDelta.y = 0.0f;
            const float distance = mathUtils::Length(planarDelta);

            if (isFleeing_)
            {
                isFleeing_ = distance < settings_.safeRadius;
            }
            else
            {
                isFleeing_ = distance <= settings_.triggerRadius;
            }

            GameplayMovementIntent movement = isFleeing_
                ? BuildGameplayFleeSteering(agentPosition, targetPosition, settings_.steering)
                : GameplayMovementIntent{};
            GameplayObstacleAvoidanceDebugSnapshot debug{};
            const bool debugEnabled = debugRegistry_ != nullptr && debugRegistry_->IsEnabled();
            if (debugEnabled)
            {
                debug.baseMovement = movement;
                debug.finalMovement = movement;
            }
            if (obstacleQuery_ != nullptr)
            {
                if (const auto* physical = world_.TryGetCharacterPhysicalSettings(agentEntity))
                {
                    const mathUtils::Vec3 origin = agentPosition +
                        mathUtils::Vec3{0.0f, physical->GetTotalHeight() * 0.5f, 0.0f};
                    const GameplayCharacterMotorComponent* motor =
                        world_.TryGetCharacterMotor(agentEntity);
                    const mathUtils::Vec3 planarVelocity{
                        motor->velocity.x, 0.0f, motor->velocity.z};
                    const GameplayObstacleAvoidanceInput avoidanceInput{
                        .baseMovement = movement,
                        .probeOrigin = origin,
                        .characterRadius = physical->radius,
                        .supportOriginVerticalOffset = physical->GetTotalHeight() * 0.5f,
                        .currentPlanarSpeed = mathUtils::Length(planarVelocity),
                        .maximumWalkableSlopeAngleDegrees = physical->maximumSlopeAngleDegrees
                    };
                    movement = ApplyGameplayObstacleAvoidance(
                        avoidanceInput, *obstacleQuery_, obstacleSettings_,
                        obstacleAvoidanceState_,
                        debugEnabled ? &debug : nullptr);
                }
            }
            if (debugEnabled)
            {
                debugRegistry_->Publish(agentEntity, GameplaySteeringDebugMode::Flee, debug);
            }
            
            currentMovement_ = movement;
        }

        void ApplyCurrentMovement_(const EntityHandle agentEntity) const noexcept
        {
            if (GameplayCharacterCommandComponent* command =
                    world_.TryGetCharacterCommand(agentEntity))
            {
                ApplyGameplayMovementIntent(currentMovement_, *command);
            }
        }
        
        void ClearMovementIfAccessible_(const EntityHandle agentEntity) noexcept
        {
            currentMovement_ = {};
            obstacleAvoidanceState_ = {};
            if (debugRegistry_ != nullptr)
            {
                debugRegistry_->Clear(agentEntity);
            }
            if (world_.IsEntityValid(agentEntity))
            {
                if (GameplayCharacterCommandComponent* command =
                        world_.TryGetCharacterCommand(agentEntity))
                {
                    ApplyGameplayMovementIntent(GameplayMovementIntent{}, *command);
                }
            }
        }

        GameplayWorld& world_;
        EntityHandle targetEntity_{kNullEntity};
        AIFleeTargetSettings settings_{};
        const IGameplayObstacleQuery* obstacleQuery_{nullptr};
        GameplayObstacleAvoidanceSettings obstacleSettings_{};
        GameplaySteeringDebugRegistry* debugRegistry_{nullptr};
        GameplayMovementIntent currentMovement_{};
        GameplayObstacleAvoidanceState obstacleAvoidanceState_{};
        float elapsedSinceSteeringUpdate_{0.0f};
        bool isFleeing_{false};
    };
}