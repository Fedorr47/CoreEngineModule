#pragma once

#include <cstdint>
#include <memory>
#include <optional>

import core;

namespace physics
{
    class JoltRuntime;

    class JoltPhysicsWorld final
    {
    public:
        explicit JoltPhysicsWorld(JoltRuntime& runtime) noexcept;
        ~JoltPhysicsWorld();
        JoltPhysicsWorld(const JoltPhysicsWorld&) = delete;
        JoltPhysicsWorld& operator=(const JoltPhysicsWorld&) = delete;
        JoltPhysicsWorld(JoltPhysicsWorld&&) = delete;
        JoltPhysicsWorld& operator=(JoltPhysicsWorld&&) = delete;
        
        [[nodiscard]] bool Initialize();
        void Shutdown() noexcept;
        [[nodiscard]] bool IsInitialized() const noexcept;
        std::uint32_t Update(float deltaSeconds);
        void ResetSimulationClock() noexcept;
        [[nodiscard]] PhysicsBodyHandle CreateBody(const PhysicsBodyDescriptor& descriptor);
        [[nodiscard]] bool DestroyBody(PhysicsBodyHandle handle);
        [[nodiscard]] bool IsBodyValid(PhysicsBodyHandle handle) const noexcept;
        [[nodiscard]] std::optional<PhysicsTransform> GetBodyTransform(
            PhysicsBodyHandle handle) const noexcept;
        [[nodiscard]] std::optional<mathUtils::Vec3> GetLinearVelocity(
            PhysicsBodyHandle handle) const noexcept;

    private:
        struct Implementation;
        JoltRuntime& runtime_;
        std::unique_ptr<Implementation> impl_;
    };
}