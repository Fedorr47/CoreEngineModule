#pragma once

#include <cstddef>
#include <string>
#include <vector>

import core;

namespace physics
{
    class JoltPhysicsWorld;
    
    class LevelPhysicsRuntime final
    {
    public:
        explicit LevelPhysicsRuntime(JoltPhysicsWorld& world) noexcept;
        ~LevelPhysicsRuntime() noexcept;
        LevelPhysicsRuntime(const LevelPhysicsRuntime&) = delete;
        LevelPhysicsRuntime& operator=(const LevelPhysicsRuntime&) = delete;
        LevelPhysicsRuntime(LevelPhysicsRuntime&&) = delete;
        LevelPhysicsRuntime& operator=(LevelPhysicsRuntime&&) = delete;
        
        [[nodiscard]] bool EnterGame(
            rendern::LevelAsset& levelAsset,
            rendern::LevelInstance& levelInstance,
            rendern::Scene& scene,
            std::string& errorMessage);
        bool SynchronizeBeforePhysics(rendern::LevelAsset& levelAsset, std::string& errorMessage);
        bool SynchronizeAfterPhysics(rendern::LevelAsset& levelAsset, rendern::LevelInstance& levelInstance,
                                     rendern::Scene& scene, std::string& errorMessage);
        bool RequestDynamicTeleport(int nodeIndex, const PhysicsTransform& transform);
        [[nodiscard]] bool LeaveGame(
            rendern::LevelAsset& levelAsset,
            rendern::LevelInstance& levelInstance,
            rendern::Scene& scene,
            std::string& errorMessage);
        void Shutdown() noexcept;
        [[nodiscard]] bool IsActive() const noexcept;
        [[nodiscard]] std::size_t GetBindingCount() const noexcept;
        
    private:
        struct Binding
        {
            int nodeIndex{ -1 };
            PhysicsBodyHandle handle{};
            PhysicsMotionType motionType{ PhysicsMotionType::Static };
            mathUtils::Vec3 authoredPosition{};
        };
        
        struct TeleportRequest
        {
            int nodeIndex{ -1 };
            PhysicsTransform transform{};
        };
        
        [[nodiscard]] bool DestroyBindings() noexcept;
        
        JoltPhysicsWorld& world_;
        std::vector<Binding> bindings_;
        std::vector<TeleportRequest> teleportRequests_;
        bool bActive_{ false };
    };
}
