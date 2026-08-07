#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

#include "Physics/Jolt/JoltCollisionLayers.h"
#include "Physics/Jolt/JoltRuntime.h"

import core;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace
{
    constexpr JPH::uint MaximumBodies = 16'384;
    constexpr JPH::uint BodyMutexCount = 0;
    constexpr JPH::uint MaximumBodyPairs = 65'536;
    constexpr JPH::uint MaximumContactConstraints = 10'240;
    constexpr double FixedPhysicsDeltaSeconds = FixedDeltaSec60;
    constexpr int MaximumPhysicsStepsPerFrame = 4;
    constexpr int CollisionSteps = 1;
    constexpr double MaximumAcceptedFrameDeltaSeconds =
        FixedPhysicsDeltaSeconds * static_cast<double>(MaximumPhysicsStepsPerFrame);
    constexpr float MinimumQuaternionLengthSquared = 1.0e-12f;
}

struct ConvertedTransform
{
    JPH::RVec3 position;
    JPH::Quat rotation;
};

[[nodiscard]] std::optional<ConvertedTransform> ConvertTransform(
    const physics::PhysicsTransform& transform) noexcept
{
    const auto& position = transform.position;
    const auto& rotation = transform.rotationQuaternion;
    if ( !mathUtils::IsFinite(position) || !mathUtils::IsFinite(rotation))
    {
        return std::nullopt;
    }
    
    const float lengthSquared = mathUtils::LengthSquared(rotation);
    
    if (!std::isfinite(lengthSquared) || lengthSquared <= MinimumQuaternionLengthSquared)
    {
        return std::nullopt;
    }
    
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return ConvertedTransform{
        .position = JPH::RVec3(position.x, position.y, position.z),
        .rotation = JPH::Quat(
            rotation.x * inverseLength,
            rotation.y * inverseLength,
            rotation.z * inverseLength,
            rotation.w * inverseLength)
            };
}

namespace physics
{
    struct JoltPhysicsWorld::Implementation
    {
        struct KinematicTarget
        {
            PhysicsBodyHandle handle{};
            PhysicsTransform transform{};
        };
        
        jolt::JoltBroadPhaseLayerInterface broadPhaseLayerInterface;
        jolt::JoltObjectLayerPairFilter objectLayerPairFilter;
        jolt::JoltObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter;
        jolt::JoltBodyRegistry bodyRegistry;
        JPH::PhysicsSystem physicsSystem;
        FixedStepScheduler fixedStepScheduler{ FixedPhysicsDeltaSeconds };
        std::vector<KinematicTarget> kinematicTargets;
        bool initialized{ false };
    };
    
    JoltPhysicsWorld::JoltPhysicsWorld(JoltRuntime& runtime) noexcept : runtime_(runtime) {}
    JoltPhysicsWorld::~JoltPhysicsWorld() { Shutdown(); }

    bool JoltPhysicsWorld::Initialize()
    {
        if (impl_ != nullptr && impl_->initialized)
        {
            return true;
        }
        if (!runtime_.IsInitialized())
        {
            return false;
        }
        if (impl_ == nullptr)
        {
            auto implementation = std::make_unique<Implementation>();
            implementation->physicsSystem.Init(
                MaximumBodies,
                BodyMutexCount,
                MaximumBodyPairs,
                MaximumContactConstraints,
                implementation->broadPhaseLayerInterface,
                implementation->objectVsBroadPhaseLayerFilter,
                implementation->objectLayerPairFilter);
            impl_ = std::move(implementation);
        }
        impl_->initialized = true;
        impl_->fixedStepScheduler.SetMaxCatchupTicks(MaximumPhysicsStepsPerFrame);
        ResetSimulationClock();
        return true;
    }

    void JoltPhysicsWorld::Shutdown() noexcept
    {
        if (impl_ == nullptr || !impl_->initialized)
        {
            return;
        }

        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        impl_->bodyRegistry.VisitActiveBodyIDs([&bodyInterface](const JPH::BodyID bodyID)
        {
            bodyInterface.RemoveBody(bodyID);
            bodyInterface.DestroyBody(bodyID);
        });
        impl_->bodyRegistry.Reset();
        impl_->kinematicTargets.clear();
        impl_->fixedStepScheduler.Reset();
        impl_->initialized = false;
    }
    
    bool JoltPhysicsWorld::IsInitialized() const noexcept
    {
        return impl_ != nullptr && impl_->initialized;
    }
    
    std::uint32_t JoltPhysicsWorld::Update(const float deltaSeconds)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return 0u;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
        {
            return 0u;
        }

        // Clamp hitches before accumulation so old wall-clock time is discarded.
        const double acceptedDeltaSeconds = std::min(
            static_cast<double>(deltaSeconds), MaximumAcceptedFrameDeltaSeconds);
        const FixedStepResult fixedStepResult = impl_->fixedStepScheduler.Advance(acceptedDeltaSeconds);
        const std::uint32_t stepCount = static_cast<std::uint32_t>(fixedStepResult.tickToSimulate);

        for (std::uint32_t stepIndex = 0u; stepIndex < stepCount; ++stepIndex)
        {
            JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
            for (const Implementation::KinematicTarget& target : impl_->kinematicTargets)
            {
                const auto bodyID = impl_->bodyRegistry.ResolveBodyID(target.handle);
                const auto converted = ConvertTransform(target.transform);
                if (bodyID.has_value() && converted.has_value())
                {
                    bodyInterface.MoveKinematic(
                        *bodyID, converted->position, converted->rotation,
                        static_cast<float>(FixedPhysicsDeltaSeconds));
                }
            }
            
            impl_->physicsSystem.Update(
                static_cast<float>(FixedPhysicsDeltaSeconds),
                CollisionSteps,
                &runtime_.GetTempAllocator(),
                &runtime_.GetJobSystem());
        }

        return stepCount;
    }

    void JoltPhysicsWorld::ResetSimulationClock() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->fixedStepScheduler.Reset();
        }
    }
    
    PhysicsBodyHandle JoltPhysicsWorld::CreateBody(const PhysicsBodyDescriptor& descriptor)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return InvalidPhysicsBodyHandle;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        const std::optional<ConvertedTransform> transform = ConvertTransform(descriptor.transform);
        if (!transform.has_value())
        {
            return InvalidPhysicsBodyHandle;
        }

        const auto shapeResult = std::visit([](const auto& shapeDescriptor)
        {
            return jolt::CreateShape(shapeDescriptor);
        }, descriptor.shape);
        
        if (!shapeResult.has_value())
        {
            return InvalidPhysicsBodyHandle;
        }

        JPH::EMotionType motionType;
        JPH::ObjectLayer objectLayer;
        JPH::EActivation activation;
        if (descriptor.motionType == PhysicsMotionType::Static)
        {
            motionType = JPH::EMotionType::Static;
            objectLayer = jolt::ObjectLayers::StaticWorld;
            activation = JPH::EActivation::DontActivate;
        }
        else if (descriptor.motionType == PhysicsMotionType::Kinematic)
        {
            motionType = JPH::EMotionType::Kinematic;
            objectLayer = jolt::ObjectLayers::DynamicWorld;
            activation = JPH::EActivation::DontActivate;
        }
        else if (descriptor.motionType == PhysicsMotionType::Dynamic)
        {
            motionType = JPH::EMotionType::Dynamic;
            objectLayer = jolt::ObjectLayers::DynamicWorld;
            activation = JPH::EActivation::Activate;
        }
        else
        {
            return InvalidPhysicsBodyHandle;
        }

        const JPH::BodyCreationSettings settings(
            shapeResult.value(), transform->position, transform->rotation, motionType, objectLayer);
        
        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        const JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(settings, activation);
        if (bodyID.IsInvalid())
        {
            return InvalidPhysicsBodyHandle;
        }

        try
        {
            const PhysicsBodyHandle handle = impl_->bodyRegistry.AllocateHandle(bodyID);
            if (handle.IsValid())
            {
                return handle;
            }
        }
        catch (...)
        {
            bodyInterface.RemoveBody(bodyID);
            bodyInterface.DestroyBody(bodyID);
            throw;
        }

        bodyInterface.RemoveBody(bodyID);
        bodyInterface.DestroyBody(bodyID);
        return InvalidPhysicsBodyHandle;
    }
    
    bool JoltPhysicsWorld::DestroyBody(PhysicsBodyHandle handle)
    {
        if (!IsInitialized() || !handle.IsValid())
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!bodyID.has_value())
        {
            return false;
        }

        // Registry release can grow its free list, so complete it before destroying Jolt storage.
        if (!impl_->bodyRegistry.ReleaseHandle(handle))
        {
            return false;
        }
        
        std::erase_if(impl_->kinematicTargets,
            [handle](const Implementation::KinematicTarget& target)
            {
                return target.handle == handle;
            });
        
        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        bodyInterface.RemoveBody(*bodyID);
        bodyInterface.DestroyBody(*bodyID);
        return true;
    }
    
    bool JoltPhysicsWorld::IsBodyValid(PhysicsBodyHandle handle) const noexcept
    {
        return IsInitialized() && handle.IsValid() && impl_->bodyRegistry.IsValid(handle);
    }
    
    std::optional<PhysicsTransform> JoltPhysicsWorld::GetBodyTransform(
        PhysicsBodyHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !handle.IsValid())
        {
            return std::nullopt;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!bodyID.has_value())
        {
            return std::nullopt;
        }

        JPH::RVec3 position;
        JPH::Quat rotation;
        impl_->physicsSystem.GetBodyInterface().GetPositionAndRotation(
            *bodyID, position, rotation);
        return PhysicsTransform{
            .position = {
                position.GetX(),
                position.GetY(),
                position.GetZ()
            },
            .rotationQuaternion = {
                rotation.GetX(),
                rotation.GetY(),
                rotation.GetZ(),
                rotation.GetW()
            }
        };
    }

    std::optional<mathUtils::Vec3> JoltPhysicsWorld::GetLinearVelocity(
        PhysicsBodyHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !handle.IsValid())
        {
            return std::nullopt;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!bodyID.has_value())
        {
            return std::nullopt;
        }

        const JPH::Vec3 velocity =
            impl_->physicsSystem.GetBodyInterface().GetLinearVelocity(*bodyID);
        return mathUtils::Vec3{
            velocity.GetX(),
            velocity.GetY(),
            velocity.GetZ()
        };
    }
    
    bool JoltPhysicsWorld::SetLinearVelocity(
        const PhysicsBodyHandle handle, 
        const mathUtils::Vec3& velocity)
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !mathUtils::IsFinite(velocity))
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!bodyID.has_value())
        {
            return false;
        }

        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        if (bodyInterface.GetMotionType(*bodyID) != JPH::EMotionType::Dynamic)
        {
            return false;
        }

        bodyInterface.SetLinearVelocity(*bodyID, JPH::Vec3(velocity.x, velocity.y, velocity.z));
        return true;
    }

    bool JoltPhysicsWorld::AddImpulse(
        const PhysicsBodyHandle handle, 
        const mathUtils::Vec3& impulse)
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !mathUtils::IsFinite(impulse))
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!bodyID.has_value())
        {
            return false;
        }

        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        if (bodyInterface.GetMotionType(*bodyID) != JPH::EMotionType::Dynamic)
        {
            return false;
        }

        bodyInterface.AddImpulse(*bodyID, JPH::Vec3(impulse.x, impulse.y, impulse.z));
        return true;
    }

    bool JoltPhysicsWorld::TeleportBody(
        const PhysicsBodyHandle handle, const PhysicsTransform& transform)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<ConvertedTransform> convertedTransform = ConvertTransform(transform);

        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!convertedTransform.has_value() || !bodyID.has_value())
        {
            return false;
        }

        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        const JPH::EMotionType motionType = bodyInterface.GetMotionType(*bodyID);
        if (motionType == JPH::EMotionType::Static)
        {
            return false;
        }
        if (motionType == JPH::EMotionType::Kinematic)
        {
            std::erase_if(
                impl_->kinematicTargets,
                [handle](const Implementation::KinematicTarget& target)
                {
                    return target.handle == handle;
                });
        }

        const JPH::EActivation activation =
            motionType == JPH::EMotionType::Dynamic
                ? JPH::EActivation::Activate
                : JPH::EActivation::DontActivate;

        bodyInterface.SetPositionAndRotation(
            *bodyID,
            convertedTransform->position,
            convertedTransform->rotation,
            activation);

        if (motionType == JPH::EMotionType::Kinematic)
        {
            bodyInterface.SetLinearAndAngularVelocity(
                *bodyID,
                JPH::Vec3::sZero(),
                JPH::Vec3::sZero());
        }

        return true;
    }

    bool JoltPhysicsWorld::MoveKinematic(
        const PhysicsBodyHandle handle,
        const PhysicsTransform& target,
        const float durationSeconds)
    {
        if (!IsInitialized() || !runtime_.IsInitialized()
            || !std::isfinite(durationSeconds) || durationSeconds <= 0.0f)
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const std::optional<ConvertedTransform> convertedTarget = ConvertTransform(target);
        const std::optional<JPH::BodyID> bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        if (!convertedTarget.has_value() || !bodyID.has_value())
        {
            return false;
        }

        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        if (bodyInterface.GetMotionType(*bodyID) != JPH::EMotionType::Kinematic)
        {
            return false;
        }

        bodyInterface.MoveKinematic(
            *bodyID, convertedTarget->position, convertedTarget->rotation, durationSeconds);
        return true;
    }
    
    bool JoltPhysicsWorld::SetKinematicTarget(
        const PhysicsBodyHandle handle, const PhysicsTransform& target)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        const auto bodyID = impl_->bodyRegistry.ResolveBodyID(handle);
        const auto converted = ConvertTransform(target);
        if (!bodyID.has_value() || !converted.has_value()
            || impl_->physicsSystem.GetBodyInterface().GetMotionType(*bodyID) != JPH::EMotionType::Kinematic)
        {
            return false;
        }
        const auto existing = std::ranges::find(
            impl_->kinematicTargets, handle, &Implementation::KinematicTarget::handle);
        if (existing != impl_->kinematicTargets.end())
        {
            existing->transform = target;
        }
        else
        {
            impl_->kinematicTargets.push_back({ .handle = handle, .transform = target });
        }
        return true;
    }
}
