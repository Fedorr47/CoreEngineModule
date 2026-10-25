# GameplaySteeringOutput

**struct** · домен «Steering» · `src/Gameplay/Navigation/GameplaySteering.cppm:52–57` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Steering».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 54 | поле | public | `GameplayMovementIntent movement{};` |
| 55 | поле | public | `GameplaySteeringStatus status{GameplaySteeringStatus::Arrived};` |
| 56 | поле | public | `float remainingDistance{0.0f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteering.cppm:52–57`.

```cpp linenums="52"
    struct GameplaySteeringOutput
    {
        GameplayMovementIntent movement{};
        GameplaySteeringStatus status{GameplaySteeringStatus::Arrived};
        float remainingDistance{0.0f};
    };
```
