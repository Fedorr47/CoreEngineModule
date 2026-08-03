#include "Physics/Jolt/JoltPhysicsWorld.h"

#include "Physics/Jolt/JoltCollisionLayers.h"
#include "Physics/Jolt/JoltRuntime.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <utility>

namespace
{
    constexpr JPH::uint MaximumBodies = 16'384;
    constexpr JPH::uint BodyMutexCount = 0;
    constexpr JPH::uint MaximumBodyPairs = 65'536;
    constexpr JPH::uint MaximumContactConstraints = 10'240;
}

namespace physics
{
    struct JoltPhysicsWorld::Implementation
    {
        jolt::JoltBroadPhaseLayerInterface broadPhaseLayerInterface;
        jolt::JoltObjectLayerPairFilter objectLayerPairFilter;
        jolt::JoltObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter;
        JPH::PhysicsSystem physicsSystem;
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
        return true;
    }

    void JoltPhysicsWorld::Shutdown() noexcept { impl_.reset(); }
    bool JoltPhysicsWorld::IsInitialized() const noexcept { return impl_ != nullptr; }
}
