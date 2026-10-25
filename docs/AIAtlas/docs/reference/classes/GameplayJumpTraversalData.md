# GameplayJumpTraversalData

**struct** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalLink.cppm:42–59` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 44 | поле | public | `mathUtils::Vec3 takeoffPosition{};` |
| 45 | поле | public | `mathUtils::Vec3 landingPosition{};` |
| 46 | поле | public | `float verticalSpeed{0.0f};` |
| 47 | поле | public | `float takeoffTolerance{0.0f};` |
| 48 | поле | public | `float landingHorizontalTolerance{0.0f};` |
| 49 | поле | public | `float landingVerticalTolerance{0.0f};` |
| 51 | метод | public | `[[nodiscard]] bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalLink.cppm:42–59`.

```cpp linenums="42"
    struct GameplayJumpTraversalData
    {
        mathUtils::Vec3 takeoffPosition{};
        mathUtils::Vec3 landingPosition{};
        float verticalSpeed{0.0f};
        float takeoffTolerance{0.0f};
        float landingHorizontalTolerance{0.0f};
        float landingVerticalTolerance{0.0f};

        [[nodiscard]] bool IsValid() const noexcept
        {
            return mathUtils::IsFinite(takeoffPosition) && mathUtils::IsFinite(landingPosition) &&
                std::isfinite(verticalSpeed) && verticalSpeed > 0.0f &&
                std::isfinite(takeoffTolerance) && takeoffTolerance > 0.0f &&
                std::isfinite(landingHorizontalTolerance) && landingHorizontalTolerance > 0.0f &&
                std::isfinite(landingVerticalTolerance) && landingVerticalTolerance > 0.0f;
        }
    };
```
