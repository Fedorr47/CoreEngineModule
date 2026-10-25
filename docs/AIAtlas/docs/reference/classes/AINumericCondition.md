# AINumericCondition

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:50–55` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 52 | поле | public | `AIWorldIntegerFactId factId{};` |
| 53 | поле | public | `AINumericConditionOperator comparison{ AINumericConditionOperator::Equal };` |
| 54 | поле | public | `std::int32_t value{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:50–55`.

```cpp linenums="50"
    struct AINumericCondition
    {
        AIWorldIntegerFactId factId{};
        AINumericConditionOperator comparison{ AINumericConditionOperator::Equal };
        std::int32_t value{};
    };
```
