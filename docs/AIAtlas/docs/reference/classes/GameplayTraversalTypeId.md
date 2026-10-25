# GameplayTraversalTypeId

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLink.cppm:15–29` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 19 | поле | public | `static constexpr ValueType InvalidValue{0u};` |
| 21 | поле | public | `ValueType value{InvalidValue};` |
| 23 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLink.cppm:15–29`.

```cpp linenums="15"
    struct GameplayTraversalTypeId
    {
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue{0u};

        ValueType value{InvalidValue};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        [[nodiscard]] constexpr bool operator==(const GameplayTraversalTypeId&) const noexcept = default;
    };
```
