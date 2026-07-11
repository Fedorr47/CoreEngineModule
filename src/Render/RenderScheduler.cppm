module;

#include <utility>

export module core:render_scheduler;

import :render_command_queue;

export namespace rendern
{
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