# GameplayAIDebugPlannedPathView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:24–28` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 26 | поле | public | `std::vector<GameplayAIDebugPlannedRouteStep> routeSteps{};` |
| 27 | поле | public | `bool complete{true};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:24–28`.

```cpp linenums="24"
    struct GameplayAIDebugPlannedPathView
    {
        std::vector<GameplayAIDebugPlannedRouteStep> routeSteps{};
        bool complete{true};
    };
```
