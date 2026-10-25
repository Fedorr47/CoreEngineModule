# GameplayRouteNodeId

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:21–42` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 25 | поле | public | `static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();` |
| 27 | поле | public | `ValueType value{InvalidValue};` |
| 29 | конструктор | public | `constexpr GameplayRouteNodeId() noexcept = default;` |
| 31 | конструктор | public | `explicit constexpr GameplayRouteNodeId(const ValueType inValue) noexcept : value(inValue)` |
| 34 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:21–42`.

```cpp linenums="21"
    struct GameplayRouteNodeId
    {
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue = std::numeric_limits<ValueType>::max();

        ValueType value{InvalidValue};

        constexpr GameplayRouteNodeId() noexcept = default;

        explicit constexpr GameplayRouteNodeId(const ValueType inValue) noexcept : value(inValue)
        {}

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        friend constexpr bool operator==(
            const GameplayRouteNodeId&,
            const GameplayRouteNodeId&) noexcept = default;
    };
```
