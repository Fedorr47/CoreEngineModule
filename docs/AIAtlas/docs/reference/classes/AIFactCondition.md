# AIFactCondition

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:24–30` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 26 | поле | public | `AIWorldFactId factId{};` |
| 27 | поле | public | `bool bExpectedValue{ true };` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:24–30`.

```cpp linenums="24"
    struct AIFactCondition
    {
        AIWorldFactId factId{};
        bool bExpectedValue{ true };

        friend constexpr bool operator==(const AIFactCondition&, const AIFactCondition&) noexcept = default;
    };
```
