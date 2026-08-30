module;

#include <algorithm>
#include <cmath>

export module core:ai_flee_target_action_runtime;

import :gameplay;
import :ai_action_contracts;
import :ai_action_runtime;
import :gameplay_steering;
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
            AIFleeTargetSettings settings = {}) noexcept
        : world_(world)
        , targetEntity_(targetEntity)
        , settings_(settings)
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

            const GameplayMovementIntent movement = isFleeing_
                ? BuildGameplayFleeSteering(agentPosition, targetPosition, settings_.steering)
                : GameplayMovementIntent{};
            ApplyGameplayMovementIntent(
                movement, *world_.TryGetCharacterCommand(agentEntity));
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
        AIFleeTargetSettings settings_{};
        float elapsedSinceSteeringUpdate_{0.0f};
        bool isFleeing_{false};
    };
}