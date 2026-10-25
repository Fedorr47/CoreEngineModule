# GameplayObstacleAvoidanceState

**struct** · домен «Obstacle avoidance» · `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:44–47` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Obstacle avoidance».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 46 | поле | public | `GameplayObstacleAvoidanceSide committedSide{GameplayObstacleAvoidanceSide::None};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplayObstacleAvoidance.cppm:44–47`.

```cpp linenums="44"
    struct GameplayObstacleAvoidanceState
    {
        GameplayObstacleAvoidanceSide committedSide{GameplayObstacleAvoidanceSide::None};
    };
```
