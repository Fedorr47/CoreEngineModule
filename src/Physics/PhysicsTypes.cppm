module;

#include <cstdint>
#include <limits>

export module core:physics_types;

import :math_utils;

export namespace physics
{
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