# GameplayTraversalLinkHandle

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRoute.cppm:14–33` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | поле | public | `static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();` |
| 20 | поле | public | `ValueType value{InvalidValue};` |
| 22 | конструктор | public | `constexpr GameplayTraversalLinkHandle() noexcept = default;` |
| 24 | конструктор | public | `explicit constexpr GameplayTraversalLinkHandle(` |
| 27 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRoute.cppm:14–33`.

```cpp linenums="14"
    struct GameplayTraversalLinkHandle
    {
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();

        ValueType value{InvalidValue};

        constexpr GameplayTraversalLinkHandle() noexcept = default;

        explicit constexpr GameplayTraversalLinkHandle(
            const ValueType inValue) noexcept : value(inValue) {}

        [[nodiscard]] constexpr bool IsValid() const noexcept
        { return value != InvalidValue; }

        friend constexpr bool operator==(
            const GameplayTraversalLinkHandle&,
            const GameplayTraversalLinkHandle&) noexcept = default;
    };
```
