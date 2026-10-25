# GameplayTraversalExecutionContext

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:20–34` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 22 | поле | public | `EntityHandle agentEntity{kNullEntity};` |
| 23 | поле | public | `GameplayTraversalLinkHandle traversalLink{};` |
| 24 | поле | public | `GameplayTraversalTypeId traversalTypeId{};` |
| 25 | поле | public | `EntityHandle targetEntity{kNullEntity};` |
| 27 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:20–34`.

```cpp linenums="20"
    struct GameplayTraversalExecutionContext
    {
        EntityHandle agentEntity{kNullEntity};
        GameplayTraversalLinkHandle traversalLink{};
        GameplayTraversalTypeId traversalTypeId{};
        EntityHandle targetEntity{kNullEntity};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return agentEntity != kNullEntity
            && traversalLink.IsValid()
            && traversalTypeId.IsValid()
            && targetEntity != kNullEntity;
        }
    };
```
