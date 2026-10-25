# QueryStatus

**enum** · домен «NavMesh» · `src/Navigation/Navigation.cppm:54–63` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «NavMesh».

## Объявление

Включено из исходника при сборке; `src/Navigation/Navigation.cppm:54–63`.

```cpp linenums="54"
    enum class QueryStatus
    {
        Succeeded,
        NotInitialized,
        InvalidRequest,
        StartNotOnNavMesh,
        GoalNotOnNavMesh,
        NoPath,
        BufferTooSmall
    };
```
