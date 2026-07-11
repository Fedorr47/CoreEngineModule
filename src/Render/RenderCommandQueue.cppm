module;

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <utility>
#include <version>

export module core:render_command_queue;

export namespace rendern
{
    class FRenderCommandQueue
    {
    public:
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
        using FCommand = std::move_only_function<void()>;
#else
        using FCommand = std::function<void()>;
#endif
        FRenderCommandQueue() = default;
        
        ~FRenderCommandQueue()
        {
            RequestStop();
        }
        
        FRenderCommandQueue(const FRenderCommandQueue&) = delete;
        FRenderCommandQueue& operator=(const FRenderCommandQueue&) = delete;
        
        bool Enqueue(FCommand command)
        {
            assert(static_cast<bool>(command) && "Render command must not be empty.");

            {
                std::lock_guard Lock(mutex_);
                if (bStopRequested_)
                {
                    return false;
                }

                commands_.emplace_back(std::move(command));
            }

            workAvailableCondition_.notify_one();
            return true;
        }
        
        void DrainReadyCommands()
        {
            const FScopedDrain scopedDrain(*this);

            while (true)
            {
                FCommand command;

                {
                    std::unique_lock Lock(mutex_);
                    if (commands_.empty())
                    {
                        NotifyIdleIfIdleLocked();
                        return;
                    }

                    command = std::move(commands_.front());
                    commands_.pop_front();
                    
                    // The command is marked active while holding the queue lock so WaitIdle()
                    // cannot observe the queue as idle after the command has been removed but
                    // before it starts executing.
                    ++activeCommandCount_;
                }

                const FScopedActiveCommandCompletion scopedActiveCommand(*this);
                command();
            }
        }
        
        bool WaitAndDrainReadyCommands()
        {
            {
                std::unique_lock Lock(mutex_);
                workAvailableCondition_.wait(Lock, [this]
                {
                    return bStopRequested_ || !commands_.empty();
                });
            
                if (bStopRequested_ && commands_.empty())
                {
                    NotifyIdleIfIdleLocked();
                    return false;
                }
            }
            
            DrainReadyCommands();
            return true;
        }
        
        void RequestStop()
        {
            {
                std::unique_lock Lock(mutex_);
                bStopRequested_ = true;
                NotifyIdleIfIdleLocked();
            }
            
            workAvailableCondition_.notify_all();
            idleCondition_.notify_all();
        }
        
        void WaitIdle()
        {
            std::unique_lock Lock(mutex_);
            idleCondition_.wait(Lock, [this]
            {
               return commands_.empty() && activeCommandCount_ == 0; 
            });
        }
        
        bool IsStopRequested() const
        {
            std::lock_guard Lock(mutex_);
            return bStopRequested_;
        }
        
    friend class FDrainScope;
        
    private:
        class FScopedDrain
        {
        public:
            explicit FScopedDrain(FRenderCommandQueue& queue)
                : queue_(queue)
            {
                queue_.BeginDrain();
            }

            ~FScopedDrain()
            {
                queue_.EndDrain();
            }

            FScopedDrain(const FScopedDrain&) = delete;
            FScopedDrain& operator=(const FScopedDrain&) = delete;

        private:
            FRenderCommandQueue& queue_;
        };

        class FScopedActiveCommandCompletion
        {
        public:
            explicit FScopedActiveCommandCompletion(FRenderCommandQueue& queue)
                : queue_(queue)
            {
            }

            ~FScopedActiveCommandCompletion()
            {
                queue_.FinishActiveCommand();
            }

            FScopedActiveCommandCompletion(const FScopedActiveCommandCompletion&) = delete;
            FScopedActiveCommandCompletion& operator=(const FScopedActiveCommandCompletion&) = delete;

        private:
            FRenderCommandQueue& queue_;
        };
        
        void FinishActiveCommand()
        {
            std::lock_guard Lock(mutex_);

            assert(activeCommandCount_ > 0 && "Active render command counter underflow.");
            --activeCommandCount_;

            NotifyIdleIfIdleLocked();
        }
        
        void NotifyIdleIfIdleLocked()
        {
            if (commands_.empty() && activeCommandCount_ == 0)
            {
                idleCondition_.notify_all();
            }
        }
        
        void BeginDrain()
        {
            std::lock_guard Lock(mutex_);
            assert(!bIsDraining_ && "Render command queue supports only one draining consumer.");
            bIsDraining_ = true;
        }

        void EndDrain()
        {
            std::lock_guard Lock(mutex_);
            bIsDraining_ = false;
            NotifyIdleIfIdleLocked();
        }
        
        mutable std::mutex mutex_;
        std::condition_variable workAvailableCondition_;
        std::condition_variable idleCondition_;
        
        std::deque<FCommand> commands_;
        
        std::size_t activeCommandCount_{0};
        bool bStopRequested_ = false;
        
        bool bIsDraining_{false};
    };
}
