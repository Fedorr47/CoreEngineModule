#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"

#include "Physics/Jolt/JoltCollisionLayers.h"
#include "Physics/Jolt/JoltRuntime.h"

import core;

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

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
}

namespace physics
{
    struct JoltPhysicsWorld::Implementation
    {
        jolt::JoltBroadPhaseLayerInterface broadPhaseLayerInterface;
        jolt::JoltObjectLayerPairFilter objectLayerPairFilter;
        jolt::JoltObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter;
        JPH::PhysicsSystem physicsSystem;
        FixedStepScheduler fixedStepScheduler{ FixedPhysicsDeltaSeconds };
    };
    
    JoltPhysicsWorld::JoltPhysicsWorld(JoltRuntime& runtime) noexcept : runtime_(runtime) {}
    JoltPhysicsWorld::~JoltPhysicsWorld() { Shutdown(); }

    bool JoltPhysicsWorld::Initialize()
    {
        if (impl_)
        {
            return true;
        }
        if (!runtime_.IsInitialized())
        {
            return false;
        }
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
        impl_->fixedStepScheduler.SetMaxCatchupTicks(MaximumPhysicsStepsPerFrame);
        ResetSimulationClock();
        return true;
    }

    void JoltPhysicsWorld::Shutdown() noexcept { impl_.reset(); }
    bool JoltPhysicsWorld::IsInitialized() const noexcept { return impl_ != nullptr; }
    
    std::uint32_t JoltPhysicsWorld::Update(const float deltaSeconds)
    {
        if (impl_ == nullptr || !runtime_.IsInitialized())
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
}
