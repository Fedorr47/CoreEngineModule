# AIFollowTargetSettings

**struct** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:21–25` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 23 | поле | public | `GameplaySeekSteeringSettings steering{};` |
| 24 | поле | public | `float steeringUpdateIntervalSeconds{0.05f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:21–25`.

```cpp linenums="21"
    struct AIFollowTargetSettings
    {
        GameplaySeekSteeringSettings steering{};
        float steeringUpdateIntervalSeconds{0.05f};
    };
```
