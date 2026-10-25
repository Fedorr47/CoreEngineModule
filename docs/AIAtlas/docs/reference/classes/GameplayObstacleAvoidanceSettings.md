# GameplayObstacleAvoidanceSettings

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:102–113` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 104 | поле | public | `float forwardProbeDistance{3.0f};` |
| 105 | поле | public | `float sideProbeDistance{2.0f};` |
| 106 | поле | public | `float sideProbeAngleDegrees{30.0f};` |
| 107 | поле | public | `float sideSwitchClearanceAdvantage{0.15f};` |
| 108 | поле | public | `float clearanceMargin{0.02f};` |
| 109 | поле | public | `float supportProbeForwardDistance{0.75f};` |
| 110 | поле | public | `float supportProbeUpOffset{0.25f};` |
| 111 | поле | public | `float maximumSupportDropDistance{0.35f};` |
| 112 | поле | public | `float forwardProbeTimeHorizonSeconds{0.75f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:102–113`.

```cpp linenums="102"
    struct GameplayObstacleAvoidanceSettings
    {
        float forwardProbeDistance{3.0f};
        float sideProbeDistance{2.0f};
        float sideProbeAngleDegrees{30.0f};
        float sideSwitchClearanceAdvantage{0.15f};
        float clearanceMargin{0.02f};
        float supportProbeForwardDistance{0.75f};
        float supportProbeUpOffset{0.25f};
        float maximumSupportDropDistance{0.35f};
        float forwardProbeTimeHorizonSeconds{0.75f};
    };
```
