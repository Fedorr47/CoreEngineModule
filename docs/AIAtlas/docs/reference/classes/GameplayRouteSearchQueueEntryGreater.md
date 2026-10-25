# GameplayRouteSearchQueueEntryGreater

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRouteSearch.cppm:115–128` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRouteSearch.cppm:115–128`.

```cpp linenums="115"
        struct GameplayRouteSearchQueueEntryGreater
        {
            [[nodiscard]] bool operator()(
                const GameplayRouteSearchQueueEntry& lhs,
                const GameplayRouteSearchQueueEntry& rhs) const noexcept
            {
                if (lhs.accumulatedCost != rhs.accumulatedCost)
                {
                    return lhs.accumulatedCost > rhs.accumulatedCost;
                }

                return lhs.insertionOrder > rhs.insertionOrder;
            }
        };
```
