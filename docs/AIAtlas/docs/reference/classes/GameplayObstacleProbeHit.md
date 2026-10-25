# GameplayObstacleProbeHit

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:22–27` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 24 | поле | public | `float distance{0.0f};` |
| 25 | поле | public | `mathUtils::Vec3 position{};` |
| 26 | поле | public | `mathUtils::Vec3 normal{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:22–27`.

```cpp linenums="22"
    struct GameplayObstacleProbeHit
    {
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };
```
