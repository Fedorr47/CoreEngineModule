# AIActionRuntimeResult

**enum** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:34–39` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Action runtime».

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:34–39`.

```cpp linenums="34"
    enum class AIActionRuntimeResult : std::uint8_t
    {
        Running,
        Succeeded,
        Failed
    };
```
