module;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <expected>
#include <string>
#include <utility>

export module core:jolt_shape_factory;

import :physics_types;

export namespace physics::jolt
{
    using JoltShapeCreationResult = std::expected<JPH::ShapeRefC, std::string>;

    [[nodiscard]] JoltShapeCreationResult CreateShape(const BoxShapeDescriptor& descriptor);
    [[nodiscard]] JoltShapeCreationResult CreateShape(const SphereShapeDescriptor& descriptor);
    [[nodiscard]] JoltShapeCreationResult CreateShape(const CapsuleShapeDescriptor& descriptor);
}

namespace
{
    [[nodiscard]] JPH::Vec3 ToJoltVector(const mathUtils::Vec3& vector) noexcept
    {
        return { vector.x, vector.y, vector.z };
    }

    [[nodiscard]] physics::jolt::JoltShapeCreationResult ToCreationResult(
        JPH::ShapeSettings::ShapeResult&& result,
        const char* shapeKind)
    {
        if (result.HasError())
        {
            return std::unexpected(std::string(shapeKind) + " shape creation failed: " + result.GetError().c_str());
        }

        JPH::ShapeRefC shape = result.Get();
        if (shape == nullptr)
        {
            return std::unexpected(std::string(shapeKind) + " shape creation returned a null shape.");
        }
        return shape;
    }
}

namespace physics::jolt
{
    JoltShapeCreationResult CreateShape(const BoxShapeDescriptor& descriptor)
    {
        if (!descriptor.IsValid())
        {
            return std::unexpected("Box shape half extents must be finite and greater than zero.");
        }

        JPH::BoxShapeSettings settings(ToJoltVector(descriptor.halfExtents), 0.0f);
        return ToCreationResult(settings.Create(), "Box");
    }

    JoltShapeCreationResult CreateShape(const SphereShapeDescriptor& descriptor)
    {
        if (!descriptor.IsValid())
        {
            return std::unexpected("Sphere shape radius must be finite and greater than zero.");
        }

        JPH::SphereShapeSettings settings(descriptor.radius);
        return ToCreationResult(settings.Create(), "Sphere");
    }

    JoltShapeCreationResult CreateShape(const CapsuleShapeDescriptor& descriptor)
    {
        if (!descriptor.IsValid())
        {
            return std::unexpected(
                "Capsule shape radius and cylinder height must be finite and greater than zero.");
        }

        // Jolt measures only half of the straight cylindrical section.
        const float halfCylinderHeight = descriptor.cylinderHeight * 0.5f;
        JPH::CapsuleShapeSettings settings(halfCylinderHeight, descriptor.radius);
        return ToCreationResult(settings.Create(), "Capsule");
    }
}