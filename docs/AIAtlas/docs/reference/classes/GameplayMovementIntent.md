# GameplayMovementIntent

**struct** · домен «Steering» · `src/Gameplay/Navigation/GameplaySteering.cppm:14–25` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Steering».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 16 | поле | public | `mathUtils::Vec3 moveWorld{0.0f, 0.0f, 0.0f};` |
| 17 | поле | public | `float moveMagnitude{0.0f};` |
| 18 | поле | public | `bool wantsRun{false};` |
| 20 | метод | public | `[[nodiscard]] bool IsMoving() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteering.cppm:14–25`.

```cpp linenums="14"
    struct GameplayMovementIntent
    {
        mathUtils::Vec3 moveWorld{0.0f, 0.0f, 0.0f};
        float moveMagnitude{0.0f};
        bool wantsRun{false};

        [[nodiscard]] bool IsMoving() const noexcept
        {
            return moveMagnitude > mathUtils::kMoveEpsilon &&
                mathUtils::Dot(moveWorld, moveWorld) > mathUtils::kLengthEpsilon;
        }
    };
```
