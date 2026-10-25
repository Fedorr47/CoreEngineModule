# GameplayRouteFollowerSettings

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteFollower.cppm:35–39` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 38 | поле | public | `float cornerLookAheadDistance{0.75f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteFollower.cppm:35–39`.

```cpp linenums="35"
    struct GameplayRouteFollowerSettings
    {
        // Zero explicitly preserves waypoint-directed route following.
        float cornerLookAheadDistance{0.75f};
    };
```
