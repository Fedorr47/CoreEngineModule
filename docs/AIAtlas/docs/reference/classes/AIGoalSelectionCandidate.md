# AIGoalSelectionCandidate

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:17–22` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 19 | поле | public | `AIGoalDefinition goal{};` |
| 20 | поле | public | `float baseScore{};` |
| 21 | поле | public | `std::vector<AIGoalScoreRule> scoreRules{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:17–22`.

```cpp linenums="17"
    struct AIGoalSelectionCandidate
    {
        AIGoalDefinition goal{};
        float baseScore{};
        std::vector<AIGoalScoreRule> scoreRules{};
    };
```
