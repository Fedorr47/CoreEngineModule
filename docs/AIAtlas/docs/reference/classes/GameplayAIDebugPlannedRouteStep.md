# GameplayAIDebugPlannedRouteStep

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:16–22` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | поле | public | `std::size_t planStepIndex{};` |
| 19 | поле | public | `AIActionId actionId{};` |
| 20 | поле | public | `AIActionContextId contextId{};` |
| 21 | поле | public | `std::optional<GameplayRoute> route{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:16–22`.

```cpp linenums="16"
    struct GameplayAIDebugPlannedRouteStep
    {
        std::size_t planStepIndex{};
        AIActionId actionId{};
        AIActionContextId contextId{};
        std::optional<GameplayRoute> route{};
    };
```
