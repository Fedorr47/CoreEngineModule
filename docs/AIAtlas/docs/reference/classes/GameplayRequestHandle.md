# GameplayRequestHandle

**struct** · домен «Диагностика» · `src/Gameplay/Navigation/GameplayRequest.cppm:16–38` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | поле | public | `ValueType value{InvalidValue};` |
| 25 | конструктор | public | `constexpr GameplayRequestHandle() noexcept = default;` |
| 27 | конструктор | public | `explicit constexpr GameplayRequestHandle(const ValueType inValue) noexcept` |
| 28 | метод | public | `: value(inValue) {}` |
| 30 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRequest.cppm:16–38`.

```cpp linenums="16"
    struct GameplayRequestHandle
    {
        using ValueType = std::uint64_t;

        static constexpr ValueType InvalidValue =
            std::numeric_limits<ValueType>::max();

        ValueType value{InvalidValue};

        constexpr GameplayRequestHandle() noexcept = default;

        explicit constexpr GameplayRequestHandle(const ValueType inValue) noexcept
            : value(inValue) {}

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        friend constexpr bool operator==(
            const GameplayRequestHandle&,
            const GameplayRequestHandle&) noexcept = default;
    };
```
