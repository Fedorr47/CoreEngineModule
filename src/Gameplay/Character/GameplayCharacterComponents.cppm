module;

#include <cstdint>

export module core:gameplay_character_components;

import :math_utils;
export import :physics_types;

export namespace rendern
{
    enum class GameplayJumpPhase : std::uint8_t
    {
        None,
        Preparing,
        Airborne
    };
    
    struct GameplayCharacterCommandComponent
    {
        float moveInputX{ 0.0f };
        float moveInputY{ 0.0f };
        mathUtils::Vec3 moveWorld{ 0.0f, 0.0f, 0.0f };
        float moveMagnitude{ 0.0f };
        bool wantsRun{ false };
        std::uint32_t actionIntentMask{ 0u };
    };

    struct GameplayCharacterMotorComponent
    {
        mathUtils::Vec3 velocity{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 desiredVelocity{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 desiredMoveWorld{ 0.0f, 0.0f, 0.0f };
        float maxWalkSpeed{ 2.0f };
        float maxRunSpeed{ 4.5f };
        float acceleration{ 12.0f };
        float deceleration{ 16.0f };
        float backwardSpeedScale{ 0.72f };
        float airDeceleration{ 2.5f };
        float jumpVerticalSpeed{ 5.5f };
    };

    struct GameplayPhysicsCharacterComponent
    {
        physics::PhysicsCharacterHandle character{};
        // Added to the capsule center to obtain the gameplay/model root position.
        mathUtils::Vec3 visualRootOffset{};
    };
    
    // Gameplay-owned physical settings consumed by subsystem integration boundaries.
    struct GameplayCharacterPhysicalSettingsComponent
    {
        float radius{ 0.3f };
        float cylinderHeight{ 1.0f };
        float maximumSlopeAngleDegrees{ 45.0f };
        float maximumStepHeight{ 0.25f };
        float mass{ 70.0f };

        [[nodiscard]] constexpr float GetTotalHeight() const noexcept
        {
            return cylinderHeight + 2.0f * radius;
        }
        
        [[nodiscard]] constexpr physics::PhysicsCharacterDescriptor BuildPhysicsCharacterDescriptor(
            const mathUtils::Vec3 position,
            const float maximumSpeed) const noexcept
        {
            return {
                .collider = { .radius = radius, .cylinderHeight = cylinderHeight },
                .position = position,
                .maximumSlopeAngleDegrees = maximumSlopeAngleDegrees,
                .maximumStepHeight = maximumStepHeight,
                .mass = mass,
                .maximumSpeed = maximumSpeed
            };
        }
    };
    
    struct GameplayCharacterMovementStateComponent
    {
        bool grounded{ true };
        bool jumping{ false };
        bool falling{ false };
        // AI feedback derived from desired versus observed fixed-step movement.
        bool physicallyBlocked{ false };
        float physicalBlockedSeconds{ 0.0f };
        GameplayJumpPhase jumpPhase{ GameplayJumpPhase::None };
        bool jumpRequestConsumed{ false };
        bool jumpAirbornePhysicallyObserved{ false };
        bool turningInPlace{ false };
        float facingYawDegrees{ 0.0f };
        // Character body-facing target consumed by movement and locomotion.
        float desiredFacingYawDegrees{ 0.0f };
        float previousFacingYawDegrees{ 0.0f };
        // View-facing yaw supplied by player or camera systems, not ordinary AI movement.
        float cameraFacingYawDegrees{ 0.0f };
        mathUtils::Vec3 jumpLockedVelocity{ 0.0f, 0.0f, 0.0f };
    };

    struct GameplayLocomotionComponent
    {
        float moveX{ 0.0f };
        float moveY{ 0.0f };
        float forwardSpeed{ 0.0f };
        float rightSpeed{ 0.0f };
        float planarSpeed{ 0.0f };
        float turnDeltaYawDegrees{ 0.0f };
        bool isMoving{ false };
        bool isRunning{ false };
        bool wantsTurnInPlaceLeft{ false };
        bool wantsTurnInPlaceRight{ false };
    };
}
