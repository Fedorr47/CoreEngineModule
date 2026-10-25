# GameplayAIDecisionStatus

**enum** · домен «Решение (instance)» · `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:15–22` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/GameplayAIDecisionContracts.cppm:15–22`.

```cpp linenums="15"
    enum class GameplayAIDecisionStatus
    {
        NotStarted,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
```
