# AIGoalDefinition

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:70–74` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 72 | поле | public | `AIGoalId goalId{};` |
| 73 | поле | public | `std::vector<AIFactCondition> desiredFacts{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:70–74`.

```cpp linenums="70"
    struct AIGoalDefinition
    {
        AIGoalId goalId{};
        std::vector<AIFactCondition> desiredFacts{};
    };
```
