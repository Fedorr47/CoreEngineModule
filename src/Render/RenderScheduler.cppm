module;

#include <cassert>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>

export module core:render_scheduler;

import :render_command_queue;

export namespace rendern
{
    class FThreadedRenderScheduler
    {
    public:
        using FCommand = FRenderCommandQueue::FCommand;

        FThreadedRenderScheduler() = default;

        ~FThreadedRenderScheduler() noexcept
        {
            StopAndJoin();
        }

        FThreadedRenderScheduler(const FThreadedRenderScheduler&) = delete;
        FThreadedRenderScheduler& operator=(const FThreadedRenderScheduler&) = delete;

        [[nodiscard]] bool Start()
        {
            std::lock_guard Lock(lifecycleMutex_);
            if (state_ != ELifecycleState::NotStarted)
            {
                return false;
            }

            renderThread_ = std::thread([this]
            {
                ThreadMain();
            });

            state_ = ELifecycleState::Running;
            return true;
        }

        [[nodiscard]] bool Enqueue(FCommand command)
        {
            {
                std::lock_guard Lock(lifecycleMutex_);
                if (state_ != ELifecycleState::Running)
                {
                    return false;
                }
            }

            return commandQueue_.Enqueue(std::move(command));
        }

        void RequestStop() noexcept
        {
            bool bShouldRequestQueueStop = false;
            {
                std::lock_guard Lock(lifecycleMutex_);
                if (state_ == ELifecycleState::Running)
                {
                    state_ = ELifecycleState::StopRequested;
                    bShouldRequestQueueStop = true;
                }
                else if (state_ == ELifecycleState::NotStarted)
                {
                    state_ = ELifecycleState::Stopped;
                    bShouldRequestQueueStop = true;
                }
                else if (state_ == ELifecycleState::StopRequested)
                {
                    bShouldRequestQueueStop = true;
                }
            }

            if (bShouldRequestQueueStop)
            {
                commandQueue_.RequestStop();
            }
        }

        void StopAndJoin() noexcept
        {
            AssertCallerCanBlockOnWorker("Threaded render scheduler cannot join its own worker thread.");

            RequestStop();

            if (renderThread_.joinable())
            {
                renderThread_.join();
            }

            std::lock_guard Lock(lifecycleMutex_);
            if (state_ == ELifecycleState::StopRequested)
            {
                state_ = ELifecycleState::Stopped;
            }
        }

        void WaitIdle()
        {
            AssertCallerCanBlockOnWorker("Threaded render scheduler cannot wait idle from its own worker thread.");

            commandQueue_.WaitIdle();
        }

        [[nodiscard]] bool HasFailure() const
        {
            std::lock_guard Lock(lifecycleMutex_);
            return static_cast<bool>(failure_);
        }

        void RethrowIfFailed() const
        {
            std::exception_ptr failure;
            {
                std::lock_guard Lock(lifecycleMutex_);
                failure = failure_;
            }

            if (failure)
            {
                std::rethrow_exception(failure);
            }
        }

        [[nodiscard]] bool IsRunning() const
        {
            std::lock_guard Lock(lifecycleMutex_);
            return state_ == ELifecycleState::Running;
        }

        [[nodiscard]] bool IsStopRequested() const
        {
            std::lock_guard Lock(lifecycleMutex_);
            return state_ == ELifecycleState::StopRequested || state_ == ELifecycleState::Stopped;
        }

    private:
        enum class ELifecycleState
        {
            NotStarted,
            Running,
            StopRequested,
            Stopped
        };

        void ThreadMain() noexcept
        {
            while (true)
            {
                try
                {
                    if (!commandQueue_.WaitAndDrainReadyCommands())
                    {
                        return;
                    }
                }
                catch (...)
                {
                    RecordFailure(std::current_exception());
                    RequestStop();
                }
            }
        }

        void RecordFailure(std::exception_ptr failure) noexcept
        {
            std::lock_guard Lock(lifecycleMutex_);

            if (!failure_)
            {
                failure_ = std::move(failure);
            }

            if (state_ == ELifecycleState::Running)
            {
                state_ = ELifecycleState::StopRequested;
            }
        }

        void AssertCallerCanBlockOnWorker(const char* message) const
        {
            assert((!renderThread_.joinable() || std::this_thread::get_id() != renderThread_.get_id()) && message);
        }

        FRenderCommandQueue commandQueue_;
        std::thread renderThread_;
        mutable std::mutex lifecycleMutex_;
        std::exception_ptr failure_;
        ELifecycleState state_{ELifecycleState::NotStarted};
    };

    class FInlineRenderScheduler
    {
    public:
        using FCommand = FRenderCommandQueue::FCommand;
        
        FInlineRenderScheduler() = default;
        
        FInlineRenderScheduler(const FInlineRenderScheduler&) = delete;
        FInlineRenderScheduler& operator=(const FInlineRenderScheduler&) = delete;
        
        [[nodiscard]] bool Enqueue(FCommand command)
        {
            return commandQueue_.Enqueue(std::move(command));
        }
        
        void DrainReadyCommands()
        {
            commandQueue_.DrainReadyCommands();
        }
        
        bool WaitAndDrainReadyCommands()
        {
            return commandQueue_.WaitAndDrainReadyCommands();
        }
        
        void RequestStop()
        {
            commandQueue_.RequestStop();
        }
        
        void WaitIdle()
        {
            commandQueue_.WaitIdle();
        }
        
        [[nodiscard]] bool IsStopRequested() const
        {
            return commandQueue_.IsStopRequested();
        }
        
    private:
        FRenderCommandQueue commandQueue_;
    };
}
