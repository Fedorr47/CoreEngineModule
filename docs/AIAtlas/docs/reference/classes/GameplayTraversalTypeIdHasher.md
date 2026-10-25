# GameplayTraversalTypeIdHasher

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLink.cppm:31–37` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLink.cppm:31–37`.

```cpp linenums="31"
    struct GameplayTraversalTypeIdHasher
    {
        [[nodiscard]] constexpr std::size_t operator()(const GameplayTraversalTypeId typeId) const noexcept
        {
            return static_cast<std::size_t>(typeId.value);
        }
    };
```
