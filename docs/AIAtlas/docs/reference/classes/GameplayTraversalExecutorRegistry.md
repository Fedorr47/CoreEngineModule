# GameplayTraversalExecutorRegistry

**class** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm:14–87` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 17 | метод | public | `[[nodiscard]] bool Register(` |
| 24 | метод | public | `[[nodiscard]] bool RegisterRuntimeOwned(` |
| 31 | метод | public | `[[nodiscard]] IGameplayTraversalExecutor* Find(` |
| 38 | метод | public | `[[nodiscard]] bool Contains(const GameplayTraversalTypeId typeId) const noexcept` |
| 43 | метод | public | `[[nodiscard]] bool Remove(const GameplayTraversalTypeId typeId) noexcept` |
| 54 | метод | public | `void ResetExternalRegistrations() noexcept` |
| 69 | метод | private | `[[nodiscard]] bool Register_(` |
| 86 | поле | private | `GameplayTraversalTypeIdHasher> executorsByType_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalExecutorRegistry.cppm:14–74`.

```cpp linenums="14"
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
```
