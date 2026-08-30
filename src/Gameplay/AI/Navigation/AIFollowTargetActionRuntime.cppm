module;

#include <algorithm>
#include <cmath>

export module core:ai_follow_target_action_runtime;

import :gameplay;
import :ai_action_contracts;
import :ai_action_runtime;
import :gameplay_steering;
import :gameplay_obstacle_avoidance;

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
            GameplayObstacleAvoidanceSettings obstacleSettings = {}) noexcept
        : world_(world)
        , targetEntity_(targetEntity)
        , settings_(settings)
        , obstacleQuery_(obstacleQuery)
        , obstacleSettings_(obstacleSettings)
        {
            settings_.steeringUpdateIntervalSeconds =
                std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
        }

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            elapsedSinceSteeringUpdate_ = 0.0f;
            if (!Validate_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }

            RefreshSteering_(context.agentEntity);
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

            return AIActionRuntimeResult::Running;
        }

        void Cancel(const AIActionRuntimeContext& context) noexcept override
        {
            ClearMovementIfAccessible_(context.agentEntity);
            elapsedSinceSteeringUpdate_ = 0.0f;
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

        void RefreshSteering_(const EntityHandle agentEntity) const noexcept
        {
            const GameplaySteeringOutput output = BuildGameplaySeekSteering(
                world_.TryGetTransform(agentEntity)->position,
                world_.TryGetTransform(targetEntity_)->position,
                settings_.steering);
            GameplayMovementIntent movement = output.movement;
            if (obstacleQuery_ != nullptr)
            {
                if (const auto* physical = world_.TryGetCharacterPhysicalSettings(agentEntity))
                {
                    const mathUtils::Vec3 origin = world_.TryGetTransform(agentEntity)->position +
                        mathUtils::Vec3{0.0f, physical->GetTotalHeight() * 0.5f, 0.0f};
                    movement = ApplyGameplayObstacleAvoidance(
                        movement, origin, *obstacleQuery_, obstacleSettings_);
                }
            }
            ApplyGameplayMovementIntent(movement, *world_.TryGetCharacterCommand(agentEntity));
        }

        void ClearMovementIfAccessible_(const EntityHandle agentEntity) const noexcept
        {
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
        float elapsedSinceSteeringUpdate_{0.0f};
    };
}