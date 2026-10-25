# GameplayRouteGraphNode

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:44–49` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 47 | поле | public | `GameplayRouteNodeId nodeId{};` |
| 48 | поле | public | `mathUtils::Vec3 worldPosition{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:44–49`.

```cpp linenums="44"
    struct GameplayRouteGraphNode
    {
        // Stable graph identity; repeated positions are allowed for distinct IDs.
        GameplayRouteNodeId nodeId{};
        mathUtils::Vec3 worldPosition{};
    };
```
