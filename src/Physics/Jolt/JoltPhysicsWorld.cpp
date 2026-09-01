#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

#include "Physics/Jolt/JoltCollisionLayers.h"
#include "Physics/Jolt/JoltRuntime.h"

import core;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Geometry/Plane.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "Jolt/Physics/Collision/CastResult.h"

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
    constexpr float CharacterPadding = 0.02f;
    constexpr float PredictiveContactDistance = 0.1f;
    // Keeps contact across shallow downward changes without increasing the permitted step height.
    constexpr float CharacterStickToFloorDistance = 0.1f;
    // Airborne intent converges gradually while grounded intent is immediate.
    constexpr float AirControlPerSecond = 3.0f;
    
    class QueryObjectLayerFilter final : public JPH::ObjectLayerFilter
    {
    public:
        explicit QueryObjectLayerFilter(const physics::PhysicsQueryLayerMask mask) noexcept : mask_(mask) {}

        [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer layer) const override
        {
            using enum physics::PhysicsQueryLayerMask;
            if (layer == physics::jolt::ObjectLayers::StaticWorld)
            {
                return HasQueryLayer(mask_, StaticWorld);
            }
            if (layer == physics::jolt::ObjectLayers::DynamicWorld)
            {
                return HasQueryLayer(mask_, DynamicWorld);
            }
            if (layer == physics::jolt::ObjectLayers::Character)
            {
                return HasQueryLayer(mask_, Character);
            }
            if (layer == physics::jolt::ObjectLayers::Trigger)
            {
                return HasQueryLayer(mask_, Trigger);
            }
            return false;
        }

    private:
        physics::PhysicsQueryLayerMask mask_;
    };

    class IgnoredBodyFilter final : public JPH::BodyFilter
    {
    public:
        explicit IgnoredBodyFilter(const std::optional<JPH::BodyID> ignoredBody) noexcept
            : ignoredBody_(ignoredBody) {}

        [[nodiscard]] bool ShouldCollide(const JPH::BodyID& bodyID) const override
        {
            return !ignoredBody_.has_value() || bodyID != *ignoredBody_;
        }

    private:
        std::optional<JPH::BodyID> ignoredBody_;
    };
    
    [[nodiscard]] bool IsShapeValid(const physics::PhysicsShapeDescriptor& descriptor) noexcept
    {
        return std::visit([](const auto& shape) { return shape.IsValid(); }, descriptor);
    }
    
    [[nodiscard]] JPH::RVec3 CharacterCenterToBasePosition(
        const mathUtils::Vec3& centerPosition,
        const JPH::Quat& rotation,
        const JPH::Vec3& shapeOffset) noexcept
    {
        const JPH::Vec3 worldOffset = rotation * shapeOffset;
        return JPH::RVec3(centerPosition.x, centerPosition.y, centerPosition.z) - worldOffset;
    }

    [[nodiscard]] mathUtils::Vec3 CharacterBaseToCenterPosition(
        const JPH::CharacterVirtual& character) noexcept
    {
        const JPH::Vec3 worldOffset = character.GetRotation() * character.GetShapeOffset();
        const JPH::RVec3 centerPosition = character.GetPosition() + worldOffset;
        return {
            static_cast<float>(centerPosition.GetX()),
            static_cast<float>(centerPosition.GetY()),
            static_cast<float>(centerPosition.GetZ())
        };
    }
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

[[nodiscard]] JPH::RMat44 ToCenterOfMassTransform(
    const ConvertedTransform& transform, const JPH::Shape& shape) noexcept
{
    return JPH::RMat44::sRotationTranslation(
        transform.rotation,
        transform.position + transform.rotation * shape.GetCenterOfMass());
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
        jolt::JoltCharacterRegistry characterRegistry;
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
        impl_->fixedStepScheduler.SetMaxCatchupTicks(MaximumPhysicsStepsPerUpdate);
        ResetSimulationClock();
        return true;
    }

    void JoltPhysicsWorld::Shutdown() noexcept
    {
        if (impl_ == nullptr || !impl_->initialized)
        {
            return;
        }

        impl_->characterRegistry.Reset();
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
        
        impl_->characterRegistry.ResetMotionObservations();

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
            
            // Kinematic velocities are established first, characters then consume them as
            // ground velocity, and rigid bodies finally advance for this fixed tick.
            const float fixedDelta = static_cast<float>(FixedPhysicsDeltaSeconds);
            const JPH::Vec3 gravity = impl_->physicsSystem.GetGravity();
            impl_->characterRegistry.VisitActiveCharacters(
                [this, fixedDelta, gravity](
                    JPH::CharacterVirtual& character,
                    const mathUtils::Vec3& desiredVelocity,
                    const float maximumStepHeight,
                    mathUtils::Vec3& observedVelocity,
                    CharacterMotionObservation& motionObservation)
                {
                    const JPH::RVec3 positionBefore = character.GetPosition();
                    JPH::Vec3 velocity = character.GetLinearVelocity();
                    character.UpdateGroundVelocity();
                    const JPH::Vec3 groundVelocity = character.GetGroundVelocity();
                    const auto groundState = character.GetGroundState();

                    const bool bIsWalkable =
                        groundState == JPH::CharacterBase::EGroundState::OnGround;
                    
                    const bool bIsSupported =
                        bIsWalkable ||
                        groundState == JPH::CharacterBase::EGroundState::OnSteepGround;

                    if (bIsSupported && (velocity - groundVelocity).Dot(character.GetUp()) < 0.0f)
                    {
                        velocity -= character.GetUp() * (velocity - groundVelocity).Dot(character.GetUp());
                    }
                    else
                    {
                        velocity += gravity * fixedDelta;
                    }

                    const float response = bIsSupported ? 1.0f
                        : std::min(1.0f, AirControlPerSecond * fixedDelta);
                    const JPH::Vec3 target{
                        desiredVelocity.x + (bIsSupported ? groundVelocity.GetX() : 0.0f),
                        velocity.GetY(),
                        desiredVelocity.z + (bIsSupported ? groundVelocity.GetZ() : 0.0f)
                    };
                    velocity.SetX(velocity.GetX() + (target.GetX() - velocity.GetX()) * response);
                    velocity.SetZ(velocity.GetZ() + (target.GetZ() - velocity.GetZ()) * response);
                    character.SetLinearVelocity(velocity);

                    JPH::CharacterVirtual::ExtendedUpdateSettings settings;
                    settings.mStickToFloorStepDown =
                        -character.GetUp() * CharacterStickToFloorDistance;
                    settings.mWalkStairsStepUp = maximumStepHeight > 0.0f
                        ? character.GetUp() * maximumStepHeight : JPH::Vec3::sZero();
                    character.ExtendedUpdate(
                        fixedDelta,
                        gravity,
                        settings,
                        impl_->physicsSystem.GetDefaultBroadPhaseLayerFilter(jolt::ObjectLayers::Character),
                        impl_->physicsSystem.GetDefaultLayerFilter(jolt::ObjectLayers::Character),
                        JPH::BodyFilter{},
                        JPH::ShapeFilter{},
                        runtime_.GetTempAllocator());
                    
                    const JPH::RVec3 displacement = character.GetPosition() - positionBefore;
                    observedVelocity = {
                        static_cast<float>(displacement.GetX()) / fixedDelta,
                        static_cast<float>(displacement.GetY()) / fixedDelta,
                        static_cast<float>(displacement.GetZ()) / fixedDelta
                    };
                    
                    assert(motionObservation.stepCount < motionObservation.steps.size());
                    
                    if (motionObservation.stepCount < motionObservation.steps.size())
                    {
                        const auto postUpdateGroundState = character.GetGroundState();
                        motionObservation.steps[motionObservation.stepCount++] = {
                            .displacement = {
                                static_cast<float>(displacement.GetX()),
                                static_cast<float>(displacement.GetY()),
                                static_cast<float>(displacement.GetZ())
                            },
                            .bIsSupported =
                                postUpdateGroundState == JPH::CharacterBase::EGroundState::OnGround ||
                                postUpdateGroundState == JPH::CharacterBase::EGroundState::OnSteepGround
                        };
                    }
                });
            
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
        
        if (!descriptor.material.IsValid() || !descriptor.surface.IsValid())
        {
            return InvalidPhysicsBodyHandle;
        }
        
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

        JPH::BodyCreationSettings settings(
            shapeResult.value(), transform->position, transform->rotation, motionType, objectLayer);
        settings.mFriction = descriptor.material.friction;
        settings.mRestitution = descriptor.material.restitution;
        
        JPH::BodyInterface& bodyInterface = impl_->physicsSystem.GetBodyInterface();
        const JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(settings, activation);
        if (bodyID.IsInvalid())
        {
            return InvalidPhysicsBodyHandle;
        }

        try
        {
            const PhysicsBodyHandle handle = impl_->bodyRegistry.AllocateHandle(bodyID, descriptor.surface);
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
    
    std::optional<PhysicsHit> JoltPhysicsWorld::RayCastClosest(
        const PhysicsRayCastRequest& request) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const float directionLengthSquared = mathUtils::LengthSquared(request.direction);
        if (!mathUtils::IsFinite(request.origin)
            || !mathUtils::IsFinite(request.direction)
            || !std::isfinite(directionLengthSquared)
            || directionLengthSquared <= mathUtils::kLengthEpsilonSq
            || !std::isfinite(request.maxDistance)
            || request.maxDistance <= 0.0f
            || request.layerMask == PhysicsQueryLayerMask::None)
        {
            return std::nullopt;
        }

        const float inverseDirectionLength = 1.0f / std::sqrt(directionLengthSquared);
        const mathUtils::Vec3 direction{
            request.direction.x * inverseDirectionLength,
            request.direction.y * inverseDirectionLength,
            request.direction.z * inverseDirectionLength
        };
        const JPH::RRayCast ray{
            JPH::RVec3(request.origin.x, request.origin.y, request.origin.z),
            JPH::Vec3(direction.x, direction.y, direction.z) * request.maxDistance
        };
        const QueryObjectLayerFilter layerFilter{ request.layerMask };
        const IgnoredBodyFilter bodyFilter{ impl_->bodyRegistry.ResolveBodyID(request.ignoredBody) };
        JPH::RayCastResult result;
        if (!impl_->physicsSystem.GetNarrowPhaseQuery().CastRay(
            ray,
            result,
            JPH::BroadPhaseLayerFilter{},
            layerFilter,
            bodyFilter))
        {
            return std::nullopt;
        }

        const std::optional<PhysicsBodyHandle> handle = impl_->bodyRegistry.ResolveHandle(result.mBodyID);
        if (!handle.has_value())
        {
            return std::nullopt;
        }
        const auto surface = impl_->bodyRegistry.ResolveSurface(*handle);
        if (!surface.has_value())
        {
            return std::nullopt;
        }

        const float distance = result.mFraction * request.maxDistance;
        const JPH::RVec3 hitPosition = ray.GetPointOnRay(result.mFraction);
        const JPH::BodyLockRead lock{ impl_->physicsSystem.GetBodyLockInterface(), result.mBodyID };
        if (!lock.Succeeded())
        {
            return std::nullopt;
        }
        const JPH::Vec3 normal = lock.GetBody().GetWorldSpaceSurfaceNormal(
            result.mSubShapeID2, hitPosition).Normalized();
        return PhysicsHit{
            .body = *handle,
            .position = {
                static_cast<float>(hitPosition.GetX()),
                static_cast<float>(hitPosition.GetY()),
                static_cast<float>(hitPosition.GetZ())
            },
            .normal = { normal.GetX(), normal.GetY(), normal.GetZ() },
            .distance = distance,
            .surface = *surface
        };
    }
    
    std::optional<PhysicsHit> JoltPhysicsWorld::ShapeCastClosest(
        const PhysicsShapeCastRequest& request) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }

        CORE_ASSERT_PHYSICS_THREAD();

        const auto transform = ConvertTransform(request.startTransform);
        const float directionLengthSquared = mathUtils::LengthSquared(request.direction);
        if (!transform.has_value()
            || !IsShapeValid(request.shape)
            || !mathUtils::IsFinite(request.direction)
            || !std::isfinite(directionLengthSquared)
            || directionLengthSquared <= mathUtils::kLengthEpsilonSq
            || !std::isfinite(request.maxDistance)
            || request.maxDistance <= 0.0f
            || request.layerMask == PhysicsQueryLayerMask::None)
        {
            return std::nullopt;
        }
        
        const float inverseLength = 1.0f / std::sqrt(directionLengthSquared);
        const JPH::Vec3 displacement{
            request.direction.x * inverseLength * request.maxDistance,
            request.direction.y * inverseLength * request.maxDistance,
            request.direction.z * inverseLength * request.maxDistance
        };
        
        const auto castShape = [&](const JPH::Shape& shape) -> std::optional<PhysicsHit>
        {
            const JPH::RShapeCast cast{
                &shape,
                JPH::Vec3::sReplicate(1.0f),
                ToCenterOfMassTransform(*transform, shape),
                displacement
            };
            const QueryObjectLayerFilter layerFilter{ request.layerMask };
            const IgnoredBodyFilter bodyFilter{
                impl_->bodyRegistry.ResolveBodyID(request.ignoredBody) };
            JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
            impl_->physicsSystem.GetNarrowPhaseQuery().CastShape(
                cast,
                JPH::ShapeCastSettings{},
                JPH::RVec3::sZero(),
                collector,
                JPH::BroadPhaseLayerFilter{},
                layerFilter,
                bodyFilter);
            if (!collector.HadHit())
            {
                return std::nullopt;
            }
            
            const JPH::ShapeCastResult& result = collector.mHit;
            const auto handle = impl_->bodyRegistry.ResolveHandle(result.mBodyID2);
            if (!handle.has_value())
            {
                return std::nullopt;
            }
            const auto surface = impl_->bodyRegistry.ResolveSurface(*handle);
            if (!surface.has_value())
            {
                return std::nullopt;
            }

            const JPH::Vec3 normal =
                (-result.mPenetrationAxis).NormalizedOr(JPH::Vec3::sZero());
            return PhysicsHit{
                .body = *handle,
                .position = {
                    result.mContactPointOn2.GetX(),
                    result.mContactPointOn2.GetY(),
                    result.mContactPointOn2.GetZ()
                },
                .normal = { normal.GetX(), normal.GetY(), normal.GetZ() },
                .distance = result.mFraction * request.maxDistance,
                .surface = *surface
            };
        };

        if (const auto* sphere = std::get_if<SphereShapeDescriptor>(&request.shape))
        {
            // Jolt's direct SphereShape is stack-resident for this synchronous cast.
            JPH::SphereShape sphereShape{sphere->radius};
            return castShape(sphereShape);
        }

        const auto shapeResult = std::visit([](const auto& descriptor)
        {
            return jolt::CreateShape(descriptor);
        }, request.shape);
        return shapeResult.has_value()
            ? castShape(*shapeResult.value())
            : std::nullopt;
    }

    std::vector<PhysicsBodyHandle> JoltPhysicsWorld::OverlapShape(
        const PhysicsOverlapRequest& request) const
    {
        std::vector<PhysicsBodyHandle> handles;
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return handles;
        }

        CORE_ASSERT_PHYSICS_THREAD();
        const auto transform = ConvertTransform(request.transform);
        if (!transform.has_value() || !IsShapeValid(request.shape)
            || request.layerMask == PhysicsQueryLayerMask::None)
        {
            return handles;
        }
        const auto shapeResult = std::visit([](const auto& descriptor)
        {
            return jolt::CreateShape(descriptor);
        }, request.shape);
        if (!shapeResult.has_value())
        {
            return handles;
        }

        const JPH::ShapeRefC& shape = shapeResult.value();
        const QueryObjectLayerFilter layerFilter{ request.layerMask };
        const IgnoredBodyFilter bodyFilter{ impl_->bodyRegistry.ResolveBodyID(request.ignoredBody) };
        JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
        impl_->physicsSystem.GetNarrowPhaseQuery().CollideShape(
            shape.GetPtr(),
            JPH::Vec3::sReplicate(1.0f),
            ToCenterOfMassTransform(*transform, *shape),
            JPH::CollideShapeSettings{},
            JPH::RVec3::sZero(),
            collector,
            JPH::BroadPhaseLayerFilter{},
            layerFilter,
            bodyFilter);
        for (const JPH::CollideShapeResult& hit : collector.mHits)
        {
            const auto handle = impl_->bodyRegistry.ResolveHandle(hit.mBodyID2);
            if (handle.has_value() && std::ranges::find(handles, *handle) == handles.end())
            {
                handles.push_back(*handle);
            }
        }
        return handles;
    }

    bool JoltPhysicsWorld::CanPlaceShape(const PhysicsOverlapRequest& request) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }

        CORE_ASSERT_PHYSICS_THREAD();
        const auto transform = ConvertTransform(request.transform);
        if (!transform.has_value() || !IsShapeValid(request.shape)
            || request.layerMask == PhysicsQueryLayerMask::None)
        {
            return false;
        }
        const auto shapeResult = std::visit([](const auto& descriptor)
        {
            return jolt::CreateShape(descriptor);
        }, request.shape);
        if (!shapeResult.has_value())
        {
            return false;
        }

        const JPH::ShapeRefC& shape = shapeResult.value();
        const QueryObjectLayerFilter layerFilter{ request.layerMask };
        const IgnoredBodyFilter bodyFilter{ impl_->bodyRegistry.ResolveBodyID(request.ignoredBody) };
        JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
        impl_->physicsSystem.GetNarrowPhaseQuery().CollideShape(
            shape.GetPtr(),
            JPH::Vec3::sReplicate(1.0f),
            ToCenterOfMassTransform(*transform, *shape),
            JPH::CollideShapeSettings{},
            JPH::RVec3::sZero(),
            collector,
            JPH::BroadPhaseLayerFilter{},
            layerFilter,
            bodyFilter);
        return !collector.HadHit();
    }
    
    PhysicsCharacterHandle JoltPhysicsWorld::CreateCharacter(const PhysicsCharacterDescriptor& descriptor)
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !descriptor.IsValid())
        {
            return InvalidPhysicsCharacterHandle;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();

        const CapsuleShapeDescriptor capsule{
            .radius = descriptor.collider.radius,
            .cylinderHeight = descriptor.collider.cylinderHeight
        };
        const auto shapeResult = jolt::CreateShape(capsule);
        if (!shapeResult.has_value())
        {
            return InvalidPhysicsCharacterHandle;
        }

        const float halfHeight = descriptor.collider.GetTotalHeight() * 0.5f;
        
        JPH::CharacterVirtualSettings settings;
        settings.mShape = shapeResult.value();
        settings.mUp = JPH::Vec3::sAxisY();
        settings.mShapeOffset = JPH::Vec3(0.0f, halfHeight, 0.0f);
        // In base-position space, only contacts within the capsule's lower cap may support it.
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -descriptor.collider.radius);
        settings.mMaxSlopeAngle = mathUtils::DegToRad(descriptor.maximumSlopeAngleDegrees);
        settings.mMass = descriptor.mass;
        settings.mCharacterPadding = CharacterPadding;
        settings.mPredictiveContactDistance = PredictiveContactDistance;
        settings.mInnerBodyLayer = jolt::ObjectLayers::Character;
        
        const JPH::Quat rotation = JPH::Quat::sIdentity();
        const JPH::RVec3 basePosition = CharacterCenterToBasePosition(
            descriptor.position, rotation, settings.mShapeOffset);
        JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual(
            &settings, basePosition, rotation, &impl_->physicsSystem);
        return impl_->characterRegistry.AllocateHandle(
            std::move(character), descriptor.collider,
            descriptor.maximumSpeed, descriptor.maximumStepHeight);
    }

    bool JoltPhysicsWorld::DestroyCharacter(const PhysicsCharacterHandle handle)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        return impl_->characterRegistry.ReleaseHandle(handle);
    }

    bool JoltPhysicsWorld::IsCharacterValid(const PhysicsCharacterHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        return impl_->characterRegistry.IsValid(handle);
    }

    std::optional<mathUtils::Vec3> JoltPhysicsWorld::GetCharacterPosition(
        const PhysicsCharacterHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        const JPH::CharacterVirtual* character = impl_->characterRegistry.ResolveCharacter(handle);
        if (character == nullptr)
        {
            return std::nullopt;
        }
        return CharacterBaseToCenterPosition(*character);
    }

    std::optional<mathUtils::Vec3> JoltPhysicsWorld::GetCharacterVelocity(
        const PhysicsCharacterHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        return impl_->characterRegistry.GetObservedVelocity(handle);
    }

    std::optional<CharacterMotionObservation> JoltPhysicsWorld::ConsumeCharacterMotionObservation(
        const PhysicsCharacterHandle handle) noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        return impl_->characterRegistry.ConsumeMotionObservation(handle);
    }

    bool JoltPhysicsWorld::TeleportCharacter(
        const PhysicsCharacterHandle handle, const mathUtils::Vec3& position)
    {
        if (!IsInitialized() || !runtime_.IsInitialized() || !mathUtils::IsFinite(position))
        {
            return false;
        }
        
        CORE_ASSERT_PHYSICS_THREAD();
        
        JPH::CharacterVirtual* character = impl_->characterRegistry.ResolveCharacter(handle);
        if (character == nullptr)
        {
            return false;
        }
        character->SetPosition(CharacterCenterToBasePosition(
            position, character->GetRotation(), character->GetShapeOffset()));
        impl_->characterRegistry.ResetObservedVelocity(handle);
        return true;
    }
    
    bool JoltPhysicsWorld::SetCharacterDesiredVelocity(
        const PhysicsCharacterHandle handle, const mathUtils::Vec3& velocity)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }
        CORE_ASSERT_PHYSICS_THREAD();
        return impl_->characterRegistry.SetDesiredVelocity(handle, velocity);
    }
    
    bool JoltPhysicsWorld::RequestCharacterJump(
       const PhysicsCharacterHandle handle, const float verticalSpeed)
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return false;
        }
        CORE_ASSERT_PHYSICS_THREAD();
        return impl_->characterRegistry.RequestJump(handle, verticalSpeed);
    }

    std::optional<CharacterGroundState> JoltPhysicsWorld::GetCharacterGroundState(
        const PhysicsCharacterHandle handle) const noexcept
    {
        if (!IsInitialized() || !runtime_.IsInitialized())
        {
            return std::nullopt;
        }
        CORE_ASSERT_PHYSICS_THREAD();
        const JPH::CharacterVirtual* character = impl_->characterRegistry.ResolveCharacter(handle);
        if (character == nullptr)
        {
            return std::nullopt;
        }

        const auto state = character->GetGroundState();
        const bool walkable = state == JPH::CharacterBase::EGroundState::OnGround;
        const bool supported = walkable
            || state == JPH::CharacterBase::EGroundState::OnSteepGround;
        const JPH::RVec3 position = character->GetGroundPosition();
        const JPH::Vec3 normal = character->GetGroundNormal();
        const JPH::Vec3 velocity = character->GetGroundVelocity();
        CharacterGroundState result{
            .bIsSupported = supported,
            .bIsWalkable = walkable,
            .position = { static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ()) },
            .normal = { normal.GetX(), normal.GetY(), normal.GetZ() },
            .velocity = { velocity.GetX(), velocity.GetY(), velocity.GetZ() }
        };
        if (const auto body = impl_->bodyRegistry.ResolveHandle(character->GetGroundBodyID()))
        {
            result.body = *body;
            result.surface = impl_->bodyRegistry.ResolveSurface(*body).value_or(InvalidSurfaceType);
        }
        return result;
    }
    
    std::optional<PhysicsCharacterDebugState> JoltPhysicsWorld::GetCharacterDebugState(
        const PhysicsCharacterHandle handle) const noexcept
    {
        const auto collider = impl_ ? impl_->characterRegistry.GetCollider(handle) : std::nullopt;
        const auto desiredVelocity = impl_
            ? impl_->characterRegistry.GetDesiredVelocity(handle) : std::nullopt;
        const auto position = GetCharacterPosition(handle);
        const auto velocity = GetCharacterVelocity(handle);
        const auto ground = GetCharacterGroundState(handle);
        if (!collider || !desiredVelocity || !position || !velocity || !ground)
        {
            return std::nullopt;
        }
        return PhysicsCharacterDebugState{
            .collider = *collider,
            .position = *position,
            .desiredVelocity = *desiredVelocity,
            .actualVelocity = *velocity,
            .ground = *ground
        };
    }
}
