#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

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
        [[nodiscard]] std::optional<mathUtils::Vec3> GetLinearVelocity(PhysicsBodyHandle handle) const noexcept;
        [[nodiscard]] bool SetLinearVelocity(PhysicsBodyHandle handle, const mathUtils::Vec3& velocity);
        [[nodiscard]] bool AddImpulse(PhysicsBodyHandle handle, const mathUtils::Vec3& impulse);
        [[nodiscard]] bool TeleportBody(PhysicsBodyHandle handle, const PhysicsTransform& transform);
        [[nodiscard]] bool MoveKinematic(
            PhysicsBodyHandle handle, 
            const PhysicsTransform& target, 
            float durationSeconds);
        [[nodiscard]] bool SetKinematicTarget(PhysicsBodyHandle handle, const PhysicsTransform& target);
        [[nodiscard]] std::optional<PhysicsHit> RayCastClosest(const PhysicsRayCastRequest& request) const noexcept;
        [[nodiscard]] std::optional<PhysicsHit> ShapeCastClosest(const PhysicsShapeCastRequest& request) const noexcept;
        [[nodiscard]] std::vector<PhysicsBodyHandle> OverlapShape(const PhysicsOverlapRequest& request) const;
        [[nodiscard]] bool CanPlaceShape(const PhysicsOverlapRequest& request) const noexcept;
        [[nodiscard]] PhysicsCharacterHandle CreateCharacter(const PhysicsCharacterDescriptor& descriptor);
        [[nodiscard]] bool DestroyCharacter(PhysicsCharacterHandle handle);
        [[nodiscard]] bool IsCharacterValid(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<mathUtils::Vec3> GetCharacterPosition(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<mathUtils::Vec3> GetCharacterVelocity(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<CharacterMotionObservation> ConsumeCharacterMotionObservation(PhysicsCharacterHandle handle) noexcept;
        [[nodiscard]] bool TeleportCharacter(PhysicsCharacterHandle handle, const mathUtils::Vec3& position);
        [[nodiscard]] bool SetCharacterDesiredVelocity(PhysicsCharacterHandle handle, const mathUtils::Vec3& velocity);
        [[nodiscard]] bool RequestCharacterJump(PhysicsCharacterHandle handle, float verticalSpeed);
        [[nodiscard]] std::optional<CharacterGroundState> GetCharacterGroundState(PhysicsCharacterHandle handle) const noexcept;
        [[nodiscard]] std::optional<PhysicsCharacterDebugState> GetCharacterDebugState(
            PhysicsCharacterHandle handle) const noexcept;
        
    private:
        struct Implementation;
        JoltRuntime& runtime_;
        std::unique_ptr<Implementation> impl_;
    };
}