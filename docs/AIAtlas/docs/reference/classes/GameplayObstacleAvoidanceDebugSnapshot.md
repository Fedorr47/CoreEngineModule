# GameplayObstacleAvoidanceDebugSnapshot

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:59–77` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 61 | поле | public | `bool evaluated{false};` |
| 62 | поле | public | `bool active{false};` |
| 63 | поле | public | `mathUtils::Vec3 probeOrigin{};` |
| 64 | поле | public | `GameplayMovementIntent baseMovement{};` |
| 65 | поле | public | `GameplayMovementIntent finalMovement{};` |
| 66 | поле | public | `GameplayObstacleProbeDebugState forward{};` |
| 67 | поле | public | `GameplayObstacleProbeDebugState left{};` |
| 68 | поле | public | `GameplayObstacleProbeDebugState right{};` |
| 69 | поле | public | `GameplayObstacleAvoidanceSide preferredSide{GameplayObstacleAvoidanceSide::None};` |
| 70 | поле | public | `GameplayObstacleAvoidanceSide chosenSide{GameplayObstacleAvoidanceSide::None};` |
| 71 | поле | public | `bool sideHeldByHysteresis{false};` |
| 72 | поле | public | `bool leftSupported{false};` |
| 73 | поле | public | `bool rightSupported{false};` |
| 74 | поле | public | `bool safetyOverride{false};` |
| 75 | поле | public | `mathUtils::Vec3 leftEscapeCandidate{};` |
| 76 | поле | public | `mathUtils::Vec3 rightEscapeCandidate{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:59–77`.

```cpp linenums="59"
    struct GameplayObstacleAvoidanceDebugSnapshot
    {
        bool evaluated{false};
        bool active{false};
        mathUtils::Vec3 probeOrigin{};
        GameplayMovementIntent baseMovement{};
        GameplayMovementIntent finalMovement{};
        GameplayObstacleProbeDebugState forward{};
        GameplayObstacleProbeDebugState left{};
        GameplayObstacleProbeDebugState right{};
        GameplayObstacleAvoidanceSide preferredSide{GameplayObstacleAvoidanceSide::None};
        GameplayObstacleAvoidanceSide chosenSide{GameplayObstacleAvoidanceSide::None};
        bool sideHeldByHysteresis{false};
        bool leftSupported{false};
        bool rightSupported{false};
        bool safetyOverride{false};
        mathUtils::Vec3 leftEscapeCandidate{};
        mathUtils::Vec3 rightEscapeCandidate{};
    };
```
