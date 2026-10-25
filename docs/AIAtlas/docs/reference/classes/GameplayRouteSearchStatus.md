# GameplayRouteSearchStatus

**enum** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:72–78` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:72–78`.

```cpp linenums="72"
    enum class GameplayRouteSearchStatus : std::uint8_t
    {
        Succeeded,
        NoRoute,
        InvalidGraph,
        InvalidRequest
    };
```
