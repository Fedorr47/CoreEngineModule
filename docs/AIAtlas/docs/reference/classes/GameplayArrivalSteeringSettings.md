# GameplayArrivalSteeringSettings

**struct** · домен «Steering» · `src/Gameplay/Navigation/GameplaySteering.cppm:33–39` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Steering».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 35 | поле | public | `float acceptanceRadius{0.25f};` |
| 36 | поле | public | `float slowingRadius{1.0f};` |
| 37 | поле | public | `float minimumMoveMagnitude{0.1f};` |
| 38 | поле | public | `bool wantsRun{false};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteering.cppm:33–39`.

```cpp linenums="33"
    struct GameplayArrivalSteeringSettings
    {
        float acceptanceRadius{0.25f};
        float slowingRadius{1.0f};
        float minimumMoveMagnitude{0.1f};
        bool wantsRun{false};
    };
```
