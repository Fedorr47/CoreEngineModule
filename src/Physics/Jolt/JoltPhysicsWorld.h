#pragma once

#include <memory>

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
    
    private:
        struct Implementation;
        JoltRuntime& runtime_;
        std::unique_ptr<Implementation> impl_;
    };
}