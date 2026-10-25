# GameplayRouteSegmentAnnotation

**struct** · домен «Маршруты» · `src/Gameplay/Navigation/GameplayRoute.cppm:42–54` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Маршруты».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 46 | поле | public | `std::optional<GameplayTraversalLinkHandle> traversalLink{};` |
| 48 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayRoute.cppm:42–54`.

```cpp linenums="42"
    struct GameplayRouteSegmentAnnotation
    {
        // Empty means the segment is traversed through ordinary steering.
        // A valid link delegates special traversal to an external system.
        std::optional<GameplayTraversalLinkHandle> traversalLink{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            const bool bHasTraversalLink = traversalLink.has_value();

            return !bHasTraversalLink || traversalLink->IsValid();
        }
    };
```
