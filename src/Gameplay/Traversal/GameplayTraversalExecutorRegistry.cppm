module;

#include <unordered_map>

export module core:gameplay_traversal_executor_registry;

import :gameplay_traversal_link;
import :gameplay_traversal_executor;

export namespace rendern
{
    // The registry does not own executors. Each executor must outlive its
    // registration and every active traversal that selected it.
    class GameplayTraversalExecutorRegistry
    {
    public:
        [[nodiscard]] bool Register(
            const GameplayTraversalTypeId typeId,
            IGameplayTraversalExecutor& executor)
        {
            return Register_(typeId, executor, true);
        }

        [[nodiscard]] bool RegisterRuntimeOwned(
            const GameplayTraversalTypeId typeId,
            IGameplayTraversalExecutor& executor)
        {
            return Register_(typeId, executor, false);
        }

        [[nodiscard]] IGameplayTraversalExecutor* Find(
            const GameplayTraversalTypeId typeId) const noexcept
        {
            const auto iterator = executorsByType_.find(typeId);
            return iterator == executorsByType_.end() ? nullptr : iterator->second.executor;
        }

        [[nodiscard]] bool Contains(const GameplayTraversalTypeId typeId) const noexcept
        {
            return executorsByType_.contains(typeId);
        }

        [[nodiscard]] bool Remove(const GameplayTraversalTypeId typeId) noexcept
        {
            const auto iterator = executorsByType_.find(typeId);
            if (iterator == executorsByType_.end() || !iterator->second.removable)
            {
                return false;
            }
            executorsByType_.erase(iterator);
            return true;
        }

        void ResetExternalRegistrations() noexcept
        {
            std::erase_if(executorsByType_, [](const auto& entry)
            {
                return entry.second.removable;
            });
        }

    private:
        struct Registration
        {
            IGameplayTraversalExecutor* executor{nullptr};
            bool removable{true};
        };

        [[nodiscard]] bool Register_(
            const GameplayTraversalTypeId typeId,
            IGameplayTraversalExecutor& executor,
            const bool removable)
        {
            if (!typeId.IsValid())
            {
                return false;
            }
            return executorsByType_.emplace(typeId, Registration{
                .executor = &executor,
                .removable = removable}).second;
        }

        std::unordered_map<
            GameplayTraversalTypeId, 
            Registration,
            GameplayTraversalTypeIdHasher> executorsByType_{};
    };
}
