# IGameplayGOAPPlannedPathProvider

**class** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:38–45` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 41 | метод | public | `virtual ~IGameplayGOAPPlannedPathProvider() = default;` |
| 42 | метод | public | `[[nodiscard]] virtual GameplayAIDebugPlannedPathView BuildPlannedPath(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPPathInspection.cppm:38–45`.

```cpp linenums="38"
    class IGameplayGOAPPlannedPathProvider
    {
    public:
        virtual ~IGameplayGOAPPlannedPathProvider() = default;
        [[nodiscard]] virtual GameplayAIDebugPlannedPathView BuildPlannedPath(
            std::span<const AIPlanStep> selectedPlan,
            std::optional<std::size_t> currentStepIndex) const = 0;
    };
```
