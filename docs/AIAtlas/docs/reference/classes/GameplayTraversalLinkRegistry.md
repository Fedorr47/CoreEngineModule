# GameplayTraversalLinkRegistry

**class** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:23–66` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 26 | метод | public | `[[nodiscard]] bool Register(GameplayTraversalLink link)` |
| 36 | метод | public | `[[nodiscard]] std::optional<GameplayTraversalLink> Find(const GameplayTraversalLinkHandle handle) const noexcept` |
| 46 | метод | public | `[[nodiscard]] bool Contains(const GameplayTraversalLinkHandle handle) const noexcept` |
| 51 | метод | public | `[[nodiscard]] bool Remove(const GameplayTraversalLinkHandle handle) noexcept` |
| 56 | метод | public | `void Reset() noexcept` |
| 65 | поле | private | `GameplayTraversalLinkHandleHasher> linksByHandle_{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:23–66`.

```cpp linenums="23"
    class GameplayTraversalLinkRegistry
    {
    public:
        [[nodiscard]] bool Register(GameplayTraversalLink link)
        {
            if (!link.IsValid())
            {
                return false;
            }
            const GameplayTraversalLinkHandle handle = link.handle;
            return linksByHandle_.emplace(handle, std::move(link)).second;
        }

        [[nodiscard]] std::optional<GameplayTraversalLink> Find(const GameplayTraversalLinkHandle handle) const noexcept
        {
            const auto iterator = linksByHandle_.find(handle);
            if (iterator == linksByHandle_.end())
            {
                return std::nullopt;
            }
            return iterator->second;
        }

        [[nodiscard]] bool Contains(const GameplayTraversalLinkHandle handle) const noexcept
        {
            return linksByHandle_.contains(handle);
        }

        [[nodiscard]] bool Remove(const GameplayTraversalLinkHandle handle) noexcept
        {
            return linksByHandle_.erase(handle) != 0u;
        }

        void Reset() noexcept
        {
            linksByHandle_.clear();
        }

    private:
        std::unordered_map<
            GameplayTraversalLinkHandle,
            GameplayTraversalLink,
            GameplayTraversalLinkHandleHasher> linksByHandle_{};
    };
```
