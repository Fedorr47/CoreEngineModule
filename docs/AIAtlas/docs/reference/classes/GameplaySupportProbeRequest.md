# GameplaySupportProbeRequest

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:29–33` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 31 | поле | public | `mathUtils::Vec3 origin{};` |
| 32 | поле | public | `float maximumDistance{0.0f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:29–33`.

```cpp linenums="29"
    struct GameplaySupportProbeRequest
    {
        mathUtils::Vec3 origin{};
        float maximumDistance{0.0f};
    };
```
