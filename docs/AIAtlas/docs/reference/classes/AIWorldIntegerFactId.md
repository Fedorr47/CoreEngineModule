# AIWorldIntegerFactId

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:28–40` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 35 | поле | public | `ValueType index{ InvalidIndex };` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIAgentWorldState.cppm:28–40`.

```cpp linenums="28"
    struct AIWorldIntegerFactId
    {
        using ValueType = std::uint16_t;

        static constexpr ValueType InvalidIndex =
            std::numeric_limits<ValueType>::max();

        ValueType index{ InvalidIndex };

        friend constexpr bool operator==(
            const AIWorldIntegerFactId&,
            const AIWorldIntegerFactId&) noexcept = default;
    };
```
