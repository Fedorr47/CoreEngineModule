# GameplayObstacleProbeRequest

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:14–20` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 16 | поле | public | `mathUtils::Vec3 origin{};` |
| 17 | поле | public | `mathUtils::Vec3 direction{};` |
| 18 | поле | public | `float maximumDistance{0.0f};` |
| 19 | поле | public | `float clearanceRadius{0.0f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:14–20`.

```cpp linenums="14"
    struct GameplayObstacleProbeRequest
    {
        mathUtils::Vec3 origin{};
        mathUtils::Vec3 direction{};
        float maximumDistance{0.0f};
        float clearanceRadius{0.0f};
    };
```
