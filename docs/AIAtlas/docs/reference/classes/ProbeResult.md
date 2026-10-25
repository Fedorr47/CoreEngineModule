# ProbeResult

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:176–182` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 178 | поле | public | `bool hit{false};` |
| 179 | поле | public | `float clearance{0.0f};` |
| 180 | поле | public | `mathUtils::Vec3 hitPosition{};` |
| 181 | поле | public | `mathUtils::Vec3 hitNormal{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:176–182`.

```cpp linenums="176"
    struct ProbeResult
    {
        bool hit{false};
        float clearance{0.0f};
        mathUtils::Vec3 hitPosition{};
        mathUtils::Vec3 hitNormal{};
    };
```
