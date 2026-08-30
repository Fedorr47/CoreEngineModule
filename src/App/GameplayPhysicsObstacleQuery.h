#pragma once

// This integration header is included after importing core so the neutral
// gameplay query and physics value types are available without reversing a
// subsystem dependency.
namespace physics
{
    class JoltPhysicsWorld;
}

namespace appRuntime
{
    // Non-owning synchronous adapter. Its Jolt world must outlive every AI runtime
    // receiving this query, and calls must remain on the physics owner thread.
    class GameplayPhysicsObstacleQuery final : public rendern::IGameplayObstacleQuery
    {
    public:
        explicit GameplayPhysicsObstacleQuery(
            const physics::JoltPhysicsWorld& physicsWorld,
            physics::PhysicsQueryLayerMask layerMask =
                physics::PhysicsQueryLayerMask::StaticWorld) noexcept;

        [[nodiscard]] bool Probe(
            const rendern::GameplayObstacleProbeRequest& request,
            rendern::GameplayObstacleProbeHit& hit) const noexcept override;

    private:
        const physics::JoltPhysicsWorld& physicsWorld_;
        physics::PhysicsQueryLayerMask layerMask_;
    };
}
