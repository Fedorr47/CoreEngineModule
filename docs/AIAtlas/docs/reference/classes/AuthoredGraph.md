# AuthoredGraph

**struct** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:31–35` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Компоненты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 33 | поле | public | `GameplayRouteGraph graph;` |
| 34 | поле | public | `std::map<std::string, GameplayRouteNodeId, std::less<>> nodes;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:31–35`.

```cpp linenums="31"
    struct AuthoredGraph
    {
        GameplayRouteGraph graph;
        std::map<std::string, GameplayRouteNodeId, std::less<>> nodes;
    };
```
