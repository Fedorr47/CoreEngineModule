module;

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

module core;

import :thread_affinity;

namespace threadAffinity
{
    namespace detail
    {
        constexpr std::size_t RoleCount = std::to_underlying(ThreadOwnerRole::Count);

        constexpr std::array<std::string_view, RoleCount> RoleNames{
            "Main",
            "Runtime",
            "Render",
            "Asset",
            "Physics",
        };

        [[nodiscard]] constexpr bool IsValidRole(const ThreadOwnerRole role) noexcept
        {
            return std::to_underlying(role) < RoleCount;
        }

        [[nodiscard]] constexpr std::size_t RoleIndex(const ThreadOwnerRole role) noexcept
        {
            const auto index = std::to_underlying(role);
            assert(index < RoleCount);
            return index;
        }

        [[nodiscard]] constexpr std::string_view RoleName(const ThreadOwnerRole role) noexcept
        {
            if (!IsValidRole(role))
            {
                return "Unknown";
            }

            return RoleNames[RoleIndex(role)];
        }

        struct OwnerThreadEntry
        {
            bool registered{ false };
            std::thread::id threadId{};
        };

        struct WorkerExecutionContext
        {
            const void* pool{ nullptr };
            std::uint32_t workerIndex{ 0 };
            bool active{ false };
        };

        struct OwnerThreadSnapshot
        {
            bool validRole{ false };
            bool registered{ false };
            std::thread::id threadId{};
        };

        class ThreadOwnerRegistry
        {
        public:
            void Register(const ThreadOwnerRole role, const std::thread::id threadId)
            {
                if (!IsValidRole(role))
                {
                    return;
                }

                std::scoped_lock lock(mutex_);
                auto& entry = entries_[RoleIndex(role)];
                entry.registered = true;
                entry.threadId = threadId;
            }

            void Unregister(const ThreadOwnerRole role)
            {
                if (!IsValidRole(role))
                {
                    return;
                }

                std::scoped_lock lock(mutex_);
                auto& entry = entries_[RoleIndex(role)];
                entry.registered = false;
                entry.threadId = {};
            }

            void Reset()
            {
                std::scoped_lock lock(mutex_);
                for (auto& entry : entries_)
                {
                    entry.registered = false;
                    entry.threadId = {};
                }
            }

            [[nodiscard]] OwnerThreadSnapshot Snapshot(const ThreadOwnerRole role) const
            {
                if (!IsValidRole(role))
                {
                    return {};
                }

                std::scoped_lock lock(mutex_);
                const auto& entry = entries_[RoleIndex(role)];
                return OwnerThreadSnapshot{
                    .validRole = true,
                    .registered = entry.registered,
                    .threadId = entry.threadId,
                };
            }

        private:
            mutable std::mutex mutex_{};
            std::array<OwnerThreadEntry, RoleCount> entries_{};
        };

        [[nodiscard]] ThreadOwnerRegistry& Registry()
        {
            static ThreadOwnerRegistry registry{};
            return registry;
        }

        thread_local WorkerExecutionContext g_workerExecutionContext{};
    }

    void RegisterOwnerThread(const ThreadOwnerRole role)
    {
        detail::Registry().Register(role, std::this_thread::get_id());
    }

    void UnregisterOwnerThread(const ThreadOwnerRole role)
    {
        detail::Registry().Unregister(role);
    }

    void ResetOwnerThreadRegistry()
    {
        detail::Registry().Reset();
    }

    [[nodiscard]] bool IsOwnerThreadRegistered(const ThreadOwnerRole role)
    {
        return detail::Registry().Snapshot(role).registered;
    }

    [[nodiscard]] std::thread::id GetOwnerThreadId(const ThreadOwnerRole role)
    {
        const detail::OwnerThreadSnapshot snapshot = detail::Registry().Snapshot(role);
        return snapshot.registered ? snapshot.threadId : std::thread::id{};
    }

    [[nodiscard]] bool IsOwnerThread(const ThreadOwnerRole role)
    {
        const std::thread::id currentThreadId = std::this_thread::get_id();
        const detail::OwnerThreadSnapshot snapshot = detail::Registry().Snapshot(role);
        return snapshot.registered && snapshot.threadId == currentThreadId;
    }

    [[nodiscard]] ThreadOwnerCheckResult CheckOwnerThreadDetailed(const ThreadOwnerRole role)
    {
        const std::thread::id currentThreadId = std::this_thread::get_id();
        const detail::OwnerThreadSnapshot snapshot = detail::Registry().Snapshot(role);

        ThreadOwnerCheckResult result{};
        result.role = role;
        result.registered = snapshot.registered;
        result.ownerThreadId = snapshot.threadId;
        result.currentThreadId = currentThreadId;
        result.matches = snapshot.registered && snapshot.threadId == currentThreadId;

        if (result.matches)
        {
            return result;
        }

        if (!snapshot.validRole)
        {
            result.message = std::format(
                "Thread affinity mismatch: expected {} owner thread, but the role is invalid",
                detail::RoleName(role));
        }
        else if (!snapshot.registered)
        {
            result.message = std::format(
                "Thread affinity mismatch: expected {} owner thread, but the role is not registered",
                detail::RoleName(role));
        }
        else
        {
            result.message = std::format(
                "Thread affinity mismatch: expected {} owner thread, registered thread={}, current thread={}",
                detail::RoleName(role),
                snapshot.threadId,
                currentThreadId);
        }

        return result;
    }

    [[nodiscard]] std::string BuildThreadAffinityMismatchMessage(ThreadOwnerRole expectedRole)
    {
        return CheckOwnerThreadDetailed(expectedRole).message;
    }

    [[nodiscard]] bool CheckOwnerThread(ThreadOwnerRole role)
    {
        return CheckOwnerThreadDetailed(role).matches;
    }

    void AssertOwnerThread(ThreadOwnerRole role)
    {
        const ThreadOwnerCheckResult result = CheckOwnerThreadDetailed(role);
        if (result.matches)
        {
            return;
        }

        std::fputs((result.message + "\n").c_str(), stderr);
        std::abort();
    }

    ScopedWorkerContext::ScopedWorkerContext(const void* pool, const std::uint32_t workerIndex) noexcept
    {
        detail::g_workerExecutionContext.pool = pool;
        detail::g_workerExecutionContext.workerIndex = workerIndex;
        detail::g_workerExecutionContext.active = (pool != nullptr);
    }

    ScopedWorkerContext::~ScopedWorkerContext() noexcept
    {
        detail::g_workerExecutionContext.pool = nullptr;
        detail::g_workerExecutionContext.workerIndex = 0;
        detail::g_workerExecutionContext.active = false;
    }

    [[nodiscard]] bool IsWorkerThread()
    {
        return detail::g_workerExecutionContext.active;
    }

    [[nodiscard]] bool IsWorkerThreadFor(const void* pool)
    {
        return detail::g_workerExecutionContext.active && detail::g_workerExecutionContext.pool == pool;
    }
}
