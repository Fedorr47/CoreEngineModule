# GameplayRouteSearchQueueEntry

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:108–113` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 110 | поле | public | `float accumulatedCost{};` |
| 111 | поле | public | `std::size_t nodeIndex{};` |
| 112 | поле | public | `std::size_t insertionOrder{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:108–113`.

```cpp linenums="108"
        struct GameplayRouteSearchQueueEntry
        {
            float accumulatedCost{};
            std::size_t nodeIndex{};
            std::size_t insertionOrder{};
        };
```
