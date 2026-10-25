# GameplayRoute

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRoute.cppm:56–109` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 60 | поле | public | `std::vector<GameplayRoutePoint> points{};` |
| 64 | поле | public | `std::vector<GameplayRouteSegmentAnnotation> segmentAnnotations{};` |
| 66 | метод | public | `[[nodiscard]] bool IsEmpty() const noexcept` |
| 71 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRoute.cppm:56–109`.

```cpp linenums="56"
    struct GameplayRoute
    {
        // Ordered world-space points. Segment annotation i describes travel
        // from points[i] to points[i + 1].
        std::vector<GameplayRoutePoint> points{};

       // Contains exactly one annotation for every pair of adjacent points.
       // An annotation without a traversal link represents ordinary movement.
        std::vector<GameplayRouteSegmentAnnotation> segmentAnnotations{};

        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return points.empty() && segmentAnnotations.empty();
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            if (points.empty())
            {
                return segmentAnnotations.empty();
            }

            const std::size_t expectedSegmentCount = points.size() - 1u;
            const bool bHasExpectedSegmentCount = segmentAnnotations.size() == expectedSegmentCount;

            if (!bHasExpectedSegmentCount)
            {
                return false;
            }

            for (const GameplayRoutePoint& point : points)
            {
                const bool bHasFinitePosition =
                    std::isfinite(point.worldPosition.x) &&
                    std::isfinite(point.worldPosition.y) &&
                    std::isfinite(point.worldPosition.z);

                if (!bHasFinitePosition)
                {
                    return false;
                }
            }

            for (const GameplayRouteSegmentAnnotation& annotation : segmentAnnotations)
            {
                if (!annotation.IsValid())
                {
                    return false;
                }
            }

            return true;
        }
    };
```
