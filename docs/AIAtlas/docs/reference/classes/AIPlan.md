# AIPlan

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:99–103` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 101 | поле | public | `AIGoalId goalId{};` |
| 102 | поле | public | `std::vector<AIPlanStep> steps{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIDecisionContracts.cppm:99–103`.

```cpp linenums="99"
    struct AIPlan
    {
        AIGoalId goalId{};
        std::vector<AIPlanStep> steps{};
    };
```
