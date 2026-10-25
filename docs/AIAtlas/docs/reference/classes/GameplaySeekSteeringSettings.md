# GameplaySeekSteeringSettings

**struct** · домен «Steering» · `src/Gameplay/Navigation/GameplaySteering.cppm:41–45` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Steering».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 43 | поле | public | `float acceptanceRadius{0.25f};` |
| 44 | поле | public | `bool wantsRun{false};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Navigation/GameplaySteering.cppm:41–45`.

```cpp linenums="41"
    struct GameplaySeekSteeringSettings
    {
        float acceptanceRadius{0.25f};
        bool wantsRun{false};
    };
```
