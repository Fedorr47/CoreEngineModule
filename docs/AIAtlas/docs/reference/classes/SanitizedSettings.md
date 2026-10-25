# SanitizedSettings

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:161–174` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 163 | поле | public | `float forwardDistance{0.0f};` |
| 164 | поле | public | `float sideDistance{0.0f};` |
| 165 | поле | public | `float sideAngleRadians{0.0f};` |
| 166 | поле | public | `float sideSwitchClearanceAdvantage{0.0f};` |
| 167 | поле | public | `float effectiveClearanceRadius{0.0f};` |
| 168 | поле | public | `float supportForwardDistance{0.0f};` |
| 169 | поле | public | `float supportUpOffset{0.0f};` |
| 170 | поле | public | `float maximumSupportDropDistance{0.0f};` |
| 171 | поле | public | `float supportOriginVerticalOffset{0.0f};` |
| 172 | поле | public | `bool validateWalkability{false};` |
| 173 | поле | public | `float minimumWalkableUpDot{0.0f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:161–174`.

```cpp linenums="161"
    struct SanitizedSettings
    {
        float forwardDistance{0.0f};
        float sideDistance{0.0f};
        float sideAngleRadians{0.0f};
        float sideSwitchClearanceAdvantage{0.0f};
        float effectiveClearanceRadius{0.0f};
        float supportForwardDistance{0.0f};
        float supportUpOffset{0.0f};
        float maximumSupportDropDistance{0.0f};
        float supportOriginVerticalOffset{0.0f};
        bool validateWalkability{false};
        float minimumWalkableUpDot{0.0f};
    };
```
