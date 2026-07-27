#pragma once

namespace JPH
{
    class JobSystem;
    class TempAllocator;
}

namespace physics
{
    class JoltRuntime final
    {
    public:
        JoltRuntime() = default;
        ~JoltRuntime();

        JoltRuntime(const JoltRuntime&) = delete;
        JoltRuntime& operator=(const JoltRuntime&) = delete;
        JoltRuntime(JoltRuntime&&) = delete;
        JoltRuntime& operator=(JoltRuntime&&) = delete;

        [[nodiscard]] bool Initialize();
        void Shutdown() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

        // Internal Jolt backend access. These types must not leave Physics/Jolt.
        [[nodiscard]] JPH::TempAllocator& GetTempAllocator() noexcept;
        [[nodiscard]] JPH::JobSystem& GetJobSystem() noexcept;

    private:
        struct Implementation;
        Implementation* impl_ = nullptr;
    };
}