# AIFleeTargetSettings

**struct** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:22–28` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 24 | поле | public | `GameplayFleeSteeringSettings steering{};` |
| 25 | поле | public | `float triggerRadius{2.0f};` |
| 26 | поле | public | `float safeRadius{3.0f};` |
| 27 | поле | public | `float steeringUpdateIntervalSeconds{0.05f};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:22–28`.

```cpp linenums="22"
    struct AIFleeTargetSettings
    {
        GameplayFleeSteeringSettings steering{};
        float triggerRadius{2.0f};
        float safeRadius{3.0f};
        float steeringUpdateIntervalSeconds{0.05f};
    };
```
