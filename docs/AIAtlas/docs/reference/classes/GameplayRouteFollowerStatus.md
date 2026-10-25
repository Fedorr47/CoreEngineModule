# GameplayRouteFollowerStatus

**enum** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteFollower.cppm:19–26` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteFollower.cppm:19–26`.

```cpp linenums="19"
    enum class GameplayRouteFollowerStatus : std::uint8_t
    {
        NotStarted,
        Following,
        TraversalRequired,
        Succeeded,
        InvalidRoute
    };
```
