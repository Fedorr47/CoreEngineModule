# AIId

**struct** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionContracts.cppm:22–46` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Action runtime».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 29 | поле | public | `ValueType value{ InvalidValue };` |
| 31 | конструктор | public | `constexpr AIId() noexcept = default;` |
| 33 | конструктор | public | `explicit constexpr AIId(const ValueType inValue) noexcept` |
| 38 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionContracts.cppm:22–46`.

```cpp linenums="22"
    struct AIId
    {
        using ValueType = std::uint16_t;

        static constexpr ValueType InvalidValue =
            std::numeric_limits<ValueType>::max();

        ValueType value{ InvalidValue };

        constexpr AIId() noexcept = default;

        explicit constexpr AIId(const ValueType inValue) noexcept
            : value{ inValue }
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value != InvalidValue;
        }

        friend constexpr bool operator==(
            const AIId&,
            const AIId&) noexcept = default;
    };
```
