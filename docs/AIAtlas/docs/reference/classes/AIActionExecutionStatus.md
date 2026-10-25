# AIActionExecutionStatus

**enum** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionContracts.cppm:53–60` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Action runtime».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionContracts.cppm:53–60`.

```cpp linenums="53"
    enum class AIActionExecutionStatus : std::uint8_t
    {
        NotStarted,
        Running,
        Succeeded,
        Failed,
        Cancelled
    };
```
