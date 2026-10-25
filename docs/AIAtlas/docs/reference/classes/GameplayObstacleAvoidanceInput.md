# GameplayObstacleAvoidanceInput

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:115–123` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 117 | поле | public | `GameplayMovementIntent baseMovement{};` |
| 118 | поле | public | `mathUtils::Vec3 probeOrigin{};` |
| 119 | поле | public | `float characterRadius{0.0f};` |
| 120 | поле | public | `float supportOriginVerticalOffset{0.0f};` |
| 121 | поле | public | `float currentPlanarSpeed{0.0f};` |
| 122 | поле | public | `std::optional<float> maximumWalkableSlopeAngleDegrees{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:115–123`.

```cpp linenums="115"
    struct GameplayObstacleAvoidanceInput
    {
        GameplayMovementIntent baseMovement{};
        mathUtils::Vec3 probeOrigin{};
        float characterRadius{0.0f};
        float supportOriginVerticalOffset{0.0f};
        float currentPlanarSpeed{0.0f};
        std::optional<float> maximumWalkableSlopeAngleDegrees{};
    };
```
