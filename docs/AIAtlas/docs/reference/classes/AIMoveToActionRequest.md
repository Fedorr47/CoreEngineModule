# AIMoveToActionRequest

**struct** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:25–33` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 29 | поле | public | `const GameplayRouteGraph* routeGraph{nullptr};` |
| 30 | поле | public | `GameplayRouteNodeId startNodeId{};` |
| 31 | поле | public | `GameplayRouteNodeId goalNodeId{};` |
| 32 | поле | public | `GameplayArrivalSteeringSettings steeringSettings{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:25–33`.

```cpp linenums="25"
    struct AIMoveToActionRequest
    {
        // Non-owning. The graph must remain alive only through synchronous
        // runtime creation; the resolved GameplayRoute is runtime-owned.
        const GameplayRouteGraph* routeGraph{nullptr};
        GameplayRouteNodeId startNodeId{};
        GameplayRouteNodeId goalNodeId{};
        GameplayArrivalSteeringSettings steeringSettings{};
    };
```
