# GameplaySupportProbeHit

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:35–40` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 37 | поле | public | `float distance{0.0f};` |
| 38 | поле | public | `mathUtils::Vec3 position{};` |
| 39 | поле | public | `mathUtils::Vec3 normal{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:35–40`.

```cpp linenums="35"
    struct GameplaySupportProbeHit
    {
        float distance{0.0f};
        mathUtils::Vec3 position{};
        mathUtils::Vec3 normal{};
    };
```
