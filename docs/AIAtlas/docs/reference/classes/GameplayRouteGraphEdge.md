# GameplayRouteGraphEdge

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:51–62` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 54 | поле | public | `GameplayRouteNodeId fromNodeId{};` |
| 55 | поле | public | `GameplayRouteNodeId toNodeId{};` |
| 58 | поле | public | `float cost{1.0f};` |
| 61 | поле | public | `GameplayRouteSegmentAnnotation annotation{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:51–62`.

```cpp linenums="51"
    struct GameplayRouteGraphEdge
    {
        // Directed endpoint IDs. Reverse traversal requires a separate edge.
        GameplayRouteNodeId fromNodeId{};
        GameplayRouteNodeId toNodeId{};

        // Complete effective non-negative cost used by weighted search.
        float cost{1.0f};

        // Copied unchanged into the reconstructed GameplayRoute segment.
        GameplayRouteSegmentAnnotation annotation{};
    };
```
