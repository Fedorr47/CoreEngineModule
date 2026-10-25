# AIGoalScoreRule

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:11–15` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 13 | поле | public | `AIFactCondition condition{};` |
| 14 | поле | public | `float scoreDelta{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIGoalSelectionContracts.cppm:11–15`.

```cpp linenums="11"
    struct AIGoalScoreRule
    {
        AIFactCondition condition{};
        float scoreDelta{};
    };
```
