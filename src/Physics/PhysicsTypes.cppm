module;

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <variant>

export module core:physics_types;

import :math_utils;

namespace
{
    [[nodiscard]] bool IsPositiveFinite(const float value) noexcept
    {
        return value > 0.0f && std::isfinite(value);
    }
}


export namespace physics
{
    inline constexpr std::uint32_t MaximumPhysicsStepsPerUpdate = 4u;
    
    struct PhysicsMaterialDescriptor
    {
        float friction{ 0.2f };
        float restitution{ 0.0f };

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct SurfaceTypeId
    {
        using ValueType = std::uint32_t;
        static constexpr ValueType InvalidValue = 0u;

        ValueType value{ InvalidValue };

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        friend constexpr bool operator==(const SurfaceTypeId&, const SurfaceTypeId&) noexcept = default;
    };

    inline constexpr SurfaceTypeId InvalidSurfaceType{};
    inline constexpr SurfaceTypeId DefaultSurfaceType{ 1u };
    
    struct BoxShapeDescriptor
    {
        mathUtils::Vec3 halfExtents{};

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct SphereShapeDescriptor
    {
        float radius{ 0.0f };

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct CapsuleShapeDescriptor
    {
        float radius{ 0.0f };
        float cylinderHeight{ 0.0f };

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] constexpr float GetTotalHeight() const noexcept
        {
            return cylinderHeight + 2.0f * radius;
        }
    };
    
    using PhysicsShapeDescriptor = std::variant<
        BoxShapeDescriptor,
        SphereShapeDescriptor,
        CapsuleShapeDescriptor>;
    
    struct CharacterColliderDescriptor
    {
        // Radius of each hemispherical cap. cylinderHeight excludes both caps.
        float radius{ 0.0f };
        float cylinderHeight{ 0.0f };

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] constexpr float GetTotalHeight() const noexcept
        {
            return cylinderHeight + 2.0f * radius;
        }
    };
    
    struct PhysicsCharacterDescriptor
    {
        CharacterColliderDescriptor collider{};

        // World-space center of the complete capsule, independent of backend center-of-mass details.
        mathUtils::Vec3 position{};

        // Degrees from the horizontal support plane. Valid values are in [0, 90).
        float maximumSlopeAngleDegrees{ 0.0f };

        // Zero disables automatic stepping for this character.
        float maximumStepHeight{ 0.0f };
        float mass{ 0.0f };

        // Zero represents a valid stationary or movement-disabled character.
        float maximumSpeed{ 0.0f };

        [[nodiscard]] bool IsValid() const noexcept;
    };
    
    struct PhysicsBodyHandle
    {
        using IndexType = std::uint32_t;
        using GenerationType = std::uint32_t;

        static constexpr IndexType InvalidIndex = std::numeric_limits<IndexType>::max();
        static constexpr GenerationType InvalidGeneration = 0u;

        IndexType index{ InvalidIndex };
        GenerationType generation{ InvalidGeneration };

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return index != InvalidIndex && generation != InvalidGeneration;
        }

        friend constexpr bool operator==(const PhysicsBodyHandle&, const PhysicsBodyHandle&) noexcept = default;
    };

    inline constexpr PhysicsBodyHandle InvalidPhysicsBodyHandle{};

    struct PhysicsCharacterHandle
    {
        using IndexType = std::uint32_t;
        using GenerationType = std::uint32_t;

        static constexpr IndexType InvalidIndex = std::numeric_limits<IndexType>::max();
        static constexpr GenerationType InvalidGeneration = 0u;

        IndexType index{ InvalidIndex };
        GenerationType generation{ InvalidGeneration };

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return index != InvalidIndex && generation != InvalidGeneration;
        }

        friend constexpr bool operator==(
            const PhysicsCharacterHandle&, const PhysicsCharacterHandle&) noexcept = default;
    };

    inline constexpr PhysicsCharacterHandle InvalidPhysicsCharacterHandle{};

    struct CharacterGroundState
    {
        bool bIsSupported{ false };
        bool bIsWalkable{ false };
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};

        // Velocity of the supporting ground at the support point.
        mathUtils::Vec3 velocity{};
        PhysicsBodyHandle body{ InvalidPhysicsBodyHandle };
        SurfaceTypeId surface{ InvalidSurfaceType };
    };
    
    // Read-only, engine-facing state used by runtime diagnostics.
    struct PhysicsCharacterDebugState
    {
        CharacterColliderDescriptor collider{};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 desiredVelocity{};
        mathUtils::Vec3 actualVelocity{};
        CharacterGroundState ground{};
    };
    
    struct CharacterMotionStepObservation
    {
        mathUtils::Vec3 displacement{};
        bool bIsSupported{ false };
    };

    struct CharacterMotionObservation
    {
        std::array<
        CharacterMotionStepObservation,
        MaximumPhysicsStepsPerUpdate> steps{};

        std::uint32_t stepCount{0u};
    };
    
    enum class PhysicsQueryLayerMask : std::uint8_t
    {
        None = 0u,
        StaticWorld = 1u << 0u,
        DynamicWorld = 1u << 1u,
        Character = 1u << 2u,
        Trigger = 1u << 3u,
        All = (1u << 4u) - 1u
    };

    [[nodiscard]] constexpr PhysicsQueryLayerMask operator|(
        const PhysicsQueryLayerMask first, const PhysicsQueryLayerMask second) noexcept
    {
        return static_cast<PhysicsQueryLayerMask>(static_cast<std::uint8_t>(first) | static_cast<std::uint8_t>(second));
    }

    [[nodiscard]] constexpr bool HasQueryLayer(
        const PhysicsQueryLayerMask mask, const PhysicsQueryLayerMask layer) noexcept
    {
        return (static_cast<std::uint8_t>(mask) & static_cast<std::uint8_t>(layer)) != 0u;
    }

    struct PhysicsRayCastRequest
    {
        mathUtils::Vec3 origin{};
        mathUtils::Vec3 direction{};
        float maxDistance{ 0.0f };
        PhysicsQueryLayerMask layerMask{ PhysicsQueryLayerMask::All };
        PhysicsBodyHandle ignoredBody{ InvalidPhysicsBodyHandle };
    };

    struct PhysicsHit
    {
        PhysicsBodyHandle body{ InvalidPhysicsBodyHandle };
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
        float distance{ 0.0f };
        SurfaceTypeId surface{ InvalidSurfaceType };
    };

    enum class PhysicsMotionType : std::uint8_t
    {
        Static,
        Kinematic,
        Dynamic
    };

    struct PhysicsTransform
    {
        mathUtils::Vec3 position{ 0.0f, 0.0f, 0.0f };

        // Quaternion components are stored in x, y, z, w order.
        mathUtils::Vec4 rotationQuaternion{ 0.0f, 0.0f, 0.0f, 1.0f };

        friend constexpr bool operator==(const PhysicsTransform&, const PhysicsTransform&) noexcept = default;
    };
    
    struct PhysicsShapeCastRequest
    {
        PhysicsShapeDescriptor shape{};
        PhysicsTransform startTransform{};
        mathUtils::Vec3 direction{};
        float maxDistance{ 0.0f };
        PhysicsQueryLayerMask layerMask{ PhysicsQueryLayerMask::All };
        PhysicsBodyHandle ignoredBody{ InvalidPhysicsBodyHandle };
    };

    struct PhysicsOverlapRequest
    {
        PhysicsShapeDescriptor shape{};
        PhysicsTransform transform{};
        PhysicsQueryLayerMask layerMask{ PhysicsQueryLayerMask::All };
        PhysicsBodyHandle ignoredBody{ InvalidPhysicsBodyHandle };
    };

    struct PhysicsBodyDescriptor
    {
        PhysicsShapeDescriptor shape{};
        PhysicsTransform transform{};
        PhysicsMotionType motionType{ PhysicsMotionType::Static };
        PhysicsMaterialDescriptor material{};
        SurfaceTypeId surface{ DefaultSurfaceType };
    };
}

bool physics::PhysicsMaterialDescriptor::IsValid() const noexcept
{
    // CoreEngine material coefficients must be finite and non-negative.
    return std::isfinite(friction) && friction >= 0.0f
        && std::isfinite(restitution) && restitution >= 0.0f;
}

bool physics::BoxShapeDescriptor::IsValid() const noexcept
{
    return IsPositiveFinite(halfExtents.x)
        && IsPositiveFinite(halfExtents.y)
        && IsPositiveFinite(halfExtents.z);
}

bool physics::SphereShapeDescriptor::IsValid() const noexcept
{
    return IsPositiveFinite(radius);
}

bool physics::CapsuleShapeDescriptor::IsValid() const noexcept
{
    return IsPositiveFinite(radius) && IsPositiveFinite(cylinderHeight);
}

bool physics::CharacterColliderDescriptor::IsValid() const noexcept
{
    return IsPositiveFinite(radius) && IsPositiveFinite(cylinderHeight);
}

bool physics::PhysicsCharacterDescriptor::IsValid() const noexcept
{
    return collider.IsValid()
        && mathUtils::IsFinite(position)
        && std::isfinite(maximumSlopeAngleDegrees)
        && maximumSlopeAngleDegrees >= 0.0f
        && maximumSlopeAngleDegrees < 90.0f
        && std::isfinite(maximumStepHeight)
        && maximumStepHeight >= 0.0f
        && IsPositiveFinite(mass)
        && std::isfinite(maximumSpeed)
        && maximumSpeed >= 0.0f;
}