module;

#include <cstdint>
#include <string>
#include <thread>

export module core:thread_affinity;

export namespace threadAffinity
{
    enum class ThreadOwnerRole : std::uint8_t
    {
        Main = 0,
        Runtime,
        Render,
        Asset,
        Physics,
        Count
    };

    struct ThreadOwnerCheckResult
    {
        ThreadOwnerRole role{ ThreadOwnerRole::Main };
        bool registered{ false };
        bool matches{ false };
        std::thread::id ownerThreadId{};
        std::thread::id currentThreadId{};
        std::string message{};
    };

    void RegisterOwnerThread(ThreadOwnerRole role);
    void UnregisterOwnerThread(ThreadOwnerRole role);
    void ResetOwnerThreadRegistry();
    [[nodiscard]] bool IsOwnerThreadRegistered(ThreadOwnerRole role);
    [[nodiscard]] bool IsOwnerThread(ThreadOwnerRole role);
    [[nodiscard]] std::thread::id GetOwnerThreadId(ThreadOwnerRole role);
    [[nodiscard]] std::string BuildThreadAffinityMismatchMessage(ThreadOwnerRole expectedRole);
    [[nodiscard]] bool CheckOwnerThread(ThreadOwnerRole role);
    [[nodiscard]] ThreadOwnerCheckResult CheckOwnerThreadDetailed(ThreadOwnerRole role);
    void AssertOwnerThread(ThreadOwnerRole role);

    class ScopedWorkerContext
    {
    public:
        ScopedWorkerContext(const void* pool, std::uint32_t workerIndex) noexcept;
        ~ScopedWorkerContext() noexcept;

        ScopedWorkerContext(const ScopedWorkerContext&) = delete;
        ScopedWorkerContext& operator=(const ScopedWorkerContext&) = delete;
        ScopedWorkerContext(ScopedWorkerContext&&) = delete;
        ScopedWorkerContext& operator=(ScopedWorkerContext&&) = delete;
    };

    [[nodiscard]] bool IsWorkerThread();
    [[nodiscard]] bool IsWorkerThreadFor(const void* pool);
}
