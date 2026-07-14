module;

#include <cassert>
#include <condition_variable>
#include <exception>
#include <stdexcept>
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
            schedulerThreadId_ = renderThread_.get_id();

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

                return commandQueue_.Enqueue(std::move(command));
            }            
        }

        void RequestStop() noexcept
        {
            bool bShouldRequestQueueStop{ false };
            {
                std::lock_guard Lock(lifecycleMutex_);
                if (state_ != ELifecycleState::Stopped)
                {
                    bShouldRequestQueueStop = true;

                    if (state_ == ELifecycleState::Running)
                    {
                        state_ = ELifecycleState::StopRequested;
                    }
                    else if (state_ == ELifecycleState::NotStarted)
                    {
                        state_ = ELifecycleState::Stopped;
                    }
                }
            }

            if (bShouldRequestQueueStop)
            {
                commandQueue_.RequestStop();
            }
        }

        void StopAndJoin() noexcept
        {
            std::thread threadToJoin;
            bool bShouldRequestQueueStop = false;

            {
                std::unique_lock lock(lifecycleMutex_);

                AssertCallerCanBlockOnWorkerLocked("Threaded render scheduler cannot join its own worker thread.");

                switch (state_)
                {
                case ELifecycleState::Joining:
                    lifecycleCondition_.wait(lock, [this]
                        {
                            return state_ == ELifecycleState::Stopped;
                        });
                    return;

                case ELifecycleState::Stopped:
                    return;

                case ELifecycleState::NotStarted:
                    bShouldRequestQueueStop = true;
                    state_ = ELifecycleState::Stopped;
                    break;

                case ELifecycleState::Running:
                case ELifecycleState::StopRequested:
                    bShouldRequestQueueStop = true;
                    state_ = ELifecycleState::Joining;

                    assert(renderThread_.joinable() &&
                        "Running threaded scheduler must own a joinable worker thread.");
                    threadToJoin = std::move(renderThread_);
                    break;
                }
            }

            if (bShouldRequestQueueStop)
            {
                commandQueue_.RequestStop();
            }

            if (threadToJoin.joinable())
            {
                threadToJoin.join();
            }

            {
                std::lock_guard Lock(lifecycleMutex_);
                schedulerThreadId_ = {};
                state_ = ELifecycleState::Stopped;
            }

            lifecycleCondition_.notify_all();
        }

        void WaitIdle()
        {
            {
                std::lock_guard Lock(lifecycleMutex_);
                AssertCallerCanBlockOnWorkerLocked(
                    "Threaded render scheduler cannot wait idle from its own worker thread.");
            }

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
            return state_ == ELifecycleState::StopRequested ||
                state_ == ELifecycleState::Joining ||
                state_ == ELifecycleState::Stopped;
        }

    private:
        enum class ELifecycleState
        {
            NotStarted,
            Running,
            StopRequested,
            Joining,
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

        void AssertCallerCanBlockOnWorkerLocked(const char* message) const
        {
            const bool bIsWorkerThread =
                schedulerThreadId_ != std::thread::id{} &&
                std::this_thread::get_id() == schedulerThreadId_;

            assert(!bIsWorkerThread && message);

            if (bIsWorkerThread)
            {
                std::terminate();
            }
        }

        FRenderCommandQueue commandQueue_;
        std::thread renderThread_;
        mutable std::mutex lifecycleMutex_;
        std::condition_variable lifecycleCondition_;
        std::exception_ptr failure_;
        std::thread::id schedulerThreadId_;
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
