# GameplayRouteGraph

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:64–70` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 66 | поле | public | `std::vector<GameplayRouteGraphNode> nodes{};` |
| 67 | поле | public | `std::vector<GameplayRouteGraphEdge> edges{};` |
| 69 | метод | public | `[[nodiscard]] bool IsValid() const;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:64–70`.

```cpp linenums="64"
    struct GameplayRouteGraph
    {
        std::vector<GameplayRouteGraphNode> nodes{};
        std::vector<GameplayRouteGraphEdge> edges{};

        [[nodiscard]] bool IsValid() const;
    };
```
