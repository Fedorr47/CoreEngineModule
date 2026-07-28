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
            if (!typeId.IsValid())
            {
                return false;
            }
            return executorsByType_.emplace(typeId, &executor).second;
        }

        [[nodiscard]] IGameplayTraversalExecutor* Find(
            const GameplayTraversalTypeId typeId) const noexcept
        {
            const auto iterator = executorsByType_.find(typeId);
            return iterator == executorsByType_.end() ? nullptr : iterator->second;
        }

        [[nodiscard]] bool Contains(const GameplayTraversalTypeId typeId) const noexcept
        {
            return executorsByType_.contains(typeId);
        }

        [[nodiscard]] bool Remove(const GameplayTraversalTypeId typeId) noexcept
        {
            return executorsByType_.erase(typeId) != 0u;
        }

        void Reset() noexcept
        {
            executorsByType_.clear();
        }

    private:
        std::unordered_map<
            GameplayTraversalTypeId, 
            IGameplayTraversalExecutor*,
            GameplayTraversalTypeIdHasher> executorsByType_{};
    };
}