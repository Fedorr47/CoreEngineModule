# AIDefinitionMetadata

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:23–29` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 25 | поле | public | `std::vector<AINamedBooleanFact> booleanFacts{};` |
| 26 | поле | public | `std::vector<AINamedIntegerFact> integerFacts{};` |
| 27 | поле | public | `std::vector<AINamedGoal> goals{};` |
| 28 | поле | public | `std::vector<AINamedAction> actions{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:23–29`.

```cpp linenums="23"
    struct AIDefinitionMetadata
    {
        std::vector<AINamedBooleanFact> booleanFacts{};
        std::vector<AINamedIntegerFact> integerFacts{};
        std::vector<AINamedGoal> goals{};
        std::vector<AINamedAction> actions{};
    };
```
