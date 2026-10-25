# GameplayTraversalLinkHandleHasher

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:15–21` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLinkRegistry.cppm:15–21`.

```cpp linenums="15"
    struct GameplayTraversalLinkHandleHasher
    {
        [[nodiscard]] constexpr std::size_t operator()(const GameplayTraversalLinkHandle handle) const noexcept
        {
            return static_cast<std::size_t>(handle.value);
        }
    };
```
