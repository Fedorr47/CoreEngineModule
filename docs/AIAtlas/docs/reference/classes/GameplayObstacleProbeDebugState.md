# GameplayObstacleProbeDebugState

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:49–57` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 51 | поле | public | `GameplayObstacleProbeRequest request{};` |
| 52 | поле | public | `bool queried{false};` |
| 53 | поле | public | `bool hit{false};` |
| 54 | поле | public | `float clearance{0.0f};` |
| 55 | поле | public | `mathUtils::Vec3 hitPosition{};` |
| 56 | поле | public | `mathUtils::Vec3 hitNormal{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:49–57`.

```cpp linenums="49"
    struct GameplayObstacleProbeDebugState
    {
        GameplayObstacleProbeRequest request{};
        bool queried{false};
        bool hit{false};
        float clearance{0.0f};
        mathUtils::Vec3 hitPosition{};
        mathUtils::Vec3 hitNormal{};
    };
```
