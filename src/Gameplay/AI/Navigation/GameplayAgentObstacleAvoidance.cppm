export module core:gameplay_agent_obstacle_avoidance;

import :gameplay;
import :gameplay_obstacle_avoidance;
import :gameplay_steering;
import :math_utils;

export namespace rendern
{
    [[nodiscard]] GameplayMovementIntent ApplyGameplayAgentObstacleAvoidance(
        const GameplayWorld& world,
        const EntityHandle entity,
        const GameplayMovementIntent& baseMovement,
        const IGameplayObstacleQuery& query,
        const GameplayObstacleAvoidanceSettings& settings,
        GameplayObstacleAvoidanceState& state,
        GameplayObstacleAvoidanceDebugSnapshot* debugOut = nullptr) noexcept
    {
        const GameplayTransformComponent* transform = world.TryGetTransform(entity);
        const GameplayCharacterPhysicalSettingsComponent* physical =
            world.TryGetCharacterPhysicalSettings(entity);
        const GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity);
        if (transform == nullptr || physical == nullptr || motor == nullptr)
        {
            return baseMovement;
        }

        const float supportOriginVerticalOffset = physical->GetTotalHeight() * 0.5f;
        const mathUtils::Vec3 probeOrigin = transform->position +
            mathUtils::Vec3{0.0f, supportOriginVerticalOffset, 0.0f};
        const mathUtils::Vec3 planarVelocity{
            motor->velocity.x, 0.0f, motor->velocity.z};
        const GameplayObstacleAvoidanceInput input{
            .baseMovement = baseMovement,
            .probeOrigin = probeOrigin,
            .characterRadius = physical->radius,
            .supportOriginVerticalOffset = supportOriginVerticalOffset,
            .currentPlanarSpeed = mathUtils::Length(planarVelocity),
            .maximumWalkableSlopeAngleDegrees = physical->maximumSlopeAngleDegrees
        };
        return ApplyGameplayObstacleAvoidance(input, query, settings, state, debugOut);
    }
}