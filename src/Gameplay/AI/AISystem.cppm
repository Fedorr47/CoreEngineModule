module;

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

export module core:ai_system;

import :gameplay;
import :ai_action_task;

export namespace rendern
{
    class AISystem
    {
    public:
        // Runs the synchronous AI update boundary, advances active actions, and
        // returns the number of agents that remained valid for processing.
        std::size_t Update(GameplayWorld& world, const float deltaSeconds = 0.0f)
        {
            world.CollectAIEntities(aiEntitiesScratch_);
            CleanupInactiveTasks_(world);
            
            for (const EntityHandle entity : aiEntitiesScratch_)
            {
                auto taskIt = tasksByEntity_.find(entity);
                if (taskIt == tasksByEntity_.end())
                {
                    continue;
                }
                
                AIActionTask& task = taskIt->second;
                if (task.GetStatus() == AIActionExecutionStatus::NotStarted)
                {
                    [[maybe_unused]] const AIActionExecutionStatus status = task.Start();
                }
                if (task.IsRunning())
                {
                    [[maybe_unused]] const AIActionExecutionStatus status =
                        task.Tick(deltaSeconds);
                }
            }
            
            return aiEntitiesScratch_.size();
        }
        
        [[nodiscard]] AIActionExecutionStatus StartAction(
            const GameplayWorld& world,
            const AIActionRuntimeContext context,
            std::unique_ptr<IAIActionRuntime> runtime)
        {
            const bool bHasValidContext = context.IsValid();
            const bool bHasRuntime = runtime != nullptr;
            const bool bHasValidEntity = bHasValidContext && world.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasValidEntity && world.HasAI(context.agentEntity);

            if (!bHasValidContext || !bHasRuntime || !bHasValidEntity || !bIsAIAgent)
            {
                return AIActionExecutionStatus::Failed;
            }
            
            CancelAction(context.agentEntity);
            tasksByEntity_.erase(context.agentEntity);
            auto taskEntry = tasksByEntity_.emplace(
                context.agentEntity,
                AIActionTask(context, std::move(runtime)));
            
            return taskEntry.first->second.Start();
        }
        
        void CancelAction(const EntityHandle agentEntity) noexcept
        {
            auto taskIt = tasksByEntity_.find(agentEntity);
            if (taskIt == tasksByEntity_.end())
            {
                return;
            }
            if (taskIt->second.IsRunning())
            {
                taskIt->second.Cancel();
            }
        }
        
        void Reset() noexcept
        {
            for (auto& taskEntry : tasksByEntity_)
            {
                AIActionTask& task = taskEntry.second;
                if (task.IsRunning())
                {
                    task.Cancel();
                }
            }
            tasksByEntity_.clear();
            aiEntitiesScratch_.clear();
        }
        
        [[nodiscard]] AIActionExecutionStatus GetActionStatus(const EntityHandle agentEntity) const noexcept
        {
            const auto taskIt = tasksByEntity_.find(agentEntity);
            if (taskIt == tasksByEntity_.end())
            {
                return AIActionExecutionStatus::NotStarted;
            }
            return taskIt->second.GetStatus();
        }
        
        [[nodiscard]] bool HasActiveAction(const EntityHandle agentEntity) const noexcept
        {
            const auto taskIt = tasksByEntity_.find(agentEntity);
            return taskIt != tasksByEntity_.end() && taskIt->second.IsRunning();
        }

    private:
        
        void CleanupInactiveTasks_(GameplayWorld& world)
        {
            for (auto it = tasksByEntity_.begin(); it != tasksByEntity_.end();)
            {
                const EntityHandle entity = it->first;
                const bool bHasLiveAgent = world.IsEntityValid(entity) && world.HasAI(entity);
                if (!bHasLiveAgent)
                {
                    if (it->second.IsRunning())
                    {
                        it->second.Cancel();
                    }
                    it = tasksByEntity_.erase(it);
                    continue;
                }
                ++it;
            }
        }
        
        // Reused between frames to avoid allocating a temporary agent list on
        // every synchronous AI update.
        std::vector<EntityHandle> aiEntitiesScratch_{};
        std::unordered_map<EntityHandle, AIActionTask> tasksByEntity_{};
    };
}
