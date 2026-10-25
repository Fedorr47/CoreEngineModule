# GameplayGOAPDecisionDefinition

**struct** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:31–36` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 33 | поле | public | `std::vector<AIGoalSelectionCandidate> goals{};` |
| 34 | поле | public | `std::vector<AIActionDefinition> actions{};` |
| 35 | поле | public | `AIDefinitionMetadata metadata{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDefinitionContracts.cppm:31–36`.

```cpp linenums="31"
    struct GameplayGOAPDecisionDefinition
    {
        std::vector<AIGoalSelectionCandidate> goals{};
        std::vector<AIActionDefinition> actions{};
        AIDefinitionMetadata metadata{};
    };
```
