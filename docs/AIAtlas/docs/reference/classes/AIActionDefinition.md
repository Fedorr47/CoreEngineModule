# AIActionDefinition

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:76–89` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 78 | поле | public | `AIActionId actionId{};` |
| 79 | поле | public | `std::vector<AIFactCondition> preconditions{};` |
| 80 | поле | public | `std::vector<AIFactEffect> effects{};` |
| 81 | поле | public | `AIActionContextId contextId{};` |
| 82 | поле | public | `float baseCost{ 1.0f };` |
| 83 | поле | public | `std::vector<AINumericCondition> numericPreconditions{};` |
| 84 | поле | public | `std::vector<AINumericEffect> numericEffects{};` |
| 87 | поле | public | `std::vector<AIFactCondition> continuationConditions{};` |
| 88 | поле | public | `std::vector<AINumericCondition> numericContinuationConditions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:76–89`.

```cpp linenums="76"
    struct AIActionDefinition
    {
        AIActionId actionId{};
        std::vector<AIFactCondition> preconditions{};
        std::vector<AIFactEffect> effects{};
        AIActionContextId contextId{};
        float baseCost{ 1.0f };
        std::vector<AINumericCondition> numericPreconditions{};
        std::vector<AINumericEffect> numericEffects{};
        // Preconditions answer whether an action may start. Continuation conditions
        // are opt-in requirements evaluated only while its runtime is active.
        std::vector<AIFactCondition> continuationConditions{};
        std::vector<AINumericCondition> numericContinuationConditions{};
    };
```
