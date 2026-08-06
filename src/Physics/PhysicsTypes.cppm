module;

#include <cmath>
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

    struct PhysicsBodyDescriptor
    {
        PhysicsTransform transform{};
        PhysicsMotionType motionType{ PhysicsMotionType::Static };
    };
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