# GameplaySteeringDebugState

**struct** · домен «Диагностика» · `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:22–26` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 24 | поле | public | `GameplaySteeringDebugMode mode{GameplaySteeringDebugMode::None};` |
| 25 | поле | public | `GameplayObstacleAvoidanceDebugSnapshot avoidance{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteeringDebug.cppm:22–26`.

```cpp linenums="22"
    struct GameplaySteeringDebugState
    {
        GameplaySteeringDebugMode mode{GameplaySteeringDebugMode::None};
        GameplayObstacleAvoidanceDebugSnapshot avoidance{};
    };
```
