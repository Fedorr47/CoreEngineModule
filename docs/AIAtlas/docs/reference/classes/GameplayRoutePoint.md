# GameplayRoutePoint

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRoute.cppm:35–40` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 39 | поле | public | `mathUtils::Vec3 worldPosition{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRoute.cppm:35–40`.

```cpp linenums="35"
    struct GameplayRoutePoint
    {
        // World-space point in the ordered route. Adjacent points define
        // the endpoints of the corresponding route segment.
        mathUtils::Vec3 worldPosition{};
    };
```
