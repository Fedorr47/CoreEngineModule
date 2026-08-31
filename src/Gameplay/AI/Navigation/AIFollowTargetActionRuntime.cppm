module;

#include <algorithm>
#include <cmath>

export module core:ai_follow_target_action_runtime;

import :gameplay;
import :ai_action_contracts;
import :ai_action_runtime;
import :gameplay_steering;
import :gameplay_obstacle_avoidance;
import :gameplay_steering_debug;

export namespace rendern
{
    inline constexpr AIActionId kAIFollowTargetActionId{4u};

    struct AIFollowTargetSettings
    {
        GameplaySeekSteeringSettings steering{};
        float steeringUpdateIntervalSeconds{0.05f};
    };

    class AIFollowTargetActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFollowTargetActionRuntime(
            GameplayWorld& world,
            const EntityHandle targetEntity,
            AIFollowTargetSettings settings = {},
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
            settings_.steeringUpdateIntervalSeconds =
                std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
        }

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            elapsedSinceSteeringUpdate_ = 0.0f;
            obstacleAvoidanceState_ = {};
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
                elapsedSinceSteeringUpdate_ =
                    settings_.steeringUpdateIntervalSeconds > 0.0f
                    ? std::fmod(
                        elapsedSinceSteeringUpdate_,
                        settings_.steeringUpdateIntervalSeconds)
                    : 0.0f;
            }
            // GameplayCharacterCommandComponent is frame-local and is cleared
            // by GameplayRuntime::BeginFrame(). Steering is intentionally
            // sampled at a lower cadence, so re-issue the last sampled intent
            // every AI tick without re-running steering or obstacle probes.
            ApplyCurrentMovement_(context.agentEntity);
            return AIActionRuntimeResult::Running;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            ClearMovementIfAccessible_(context.agentEntity);
            elapsedSinceSteeringUpdate_ = 0.0f;
            if (debugRegistry_ != nullptr)
            {
                debugRegistry_->Clear(context.agentEntity);
            }
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
                world_.HasCharacterMovementState(context.agentEntity) && bHasTarget &&
                world_.HasTransform(targetEntity_);
        }

        void RefreshSteering_(const EntityHandle agentEntity) noexcept
        {
            const GameplaySteeringOutput output = BuildGameplaySeekSteering(
                world_.TryGetTransform(agentEntity)->position,
                world_.TryGetTransform(targetEntity_)->position,
                settings_.steering);
            GameplayMovementIntent movement = output.movement;
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
                    const mathUtils::Vec3 origin = world_.TryGetTransform(agentEntity)->position +
                        mathUtils::Vec3{0.0f, physical->GetTotalHeight() * 0.5f, 0.0f};
                    GameplayObstacleAvoidanceSettings settings = obstacleSettings_;
                    settings.characterRadius = physical->radius;
                    settings.supportOriginVerticalOffset =
                        physical->GetTotalHeight() * 0.5f;
                    movement = ApplyGameplayObstacleAvoidance(
                        movement, origin, *obstacleQuery_, settings,
                        obstacleAvoidanceState_,
                        debugEnabled ? &debug : nullptr);
                }
            }
            if (debugEnabled)
            {
                debugRegistry_->Publish(agentEntity, GameplaySteeringDebugMode::Follow, debug);
            }
            ApplyGameplayMovementIntent(movement, *world_.TryGetCharacterCommand(agentEntity));
            
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
        AIFollowTargetSettings settings_{};
        const IGameplayObstacleQuery* obstacleQuery_{nullptr};
        GameplayObstacleAvoidanceSettings obstacleSettings_{};
        GameplaySteeringDebugRegistry* debugRegistry_{nullptr};
        GameplayMovementIntent currentMovement_{};
        GameplayObstacleAvoidanceState obstacleAvoidanceState_{};
        float elapsedSinceSteeringUpdate_{0.0f};
    };
}