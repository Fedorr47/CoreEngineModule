# GameplayTraversalLink

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLink.cppm:61–73` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 63 | поле | public | `GameplayTraversalLinkHandle handle{};` |
| 64 | поле | public | `GameplayTraversalTypeId traversalTypeId{};` |
| 65 | поле | public | `EntityHandle targetEntity{kNullEntity};` |
| 66 | поле | public | `GameplayJumpTraversalData jump{};` |
| 68 | метод | public | `[[nodiscard]] bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLink.cppm:61–73`.

```cpp linenums="61"
    struct GameplayTraversalLink
    {
        GameplayTraversalLinkHandle handle{};
        GameplayTraversalTypeId traversalTypeId{};
        EntityHandle targetEntity{kNullEntity};
        GameplayJumpTraversalData jump{};

        [[nodiscard]] bool IsValid() const noexcept
        {
            return handle.IsValid() && traversalTypeId.IsValid() && targetEntity != kNullEntity &&
                (traversalTypeId != kJumpTraversalTypeId || jump.IsValid());
        }
    };
```
