# AIPlanExecutionStatus

**enum** · домен «Исполнение плана» · `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:15–23` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Исполнение плана».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIPlanExecution.cppm:15–23`.

```cpp linenums="15"
    enum class AIPlanExecutionStatus : std::uint8_t
    {
        NotStarted,
        ReadyToStartStep,
        RunningStep,
        Succeeded,
        Failed,
        Cancelled
    };
```
