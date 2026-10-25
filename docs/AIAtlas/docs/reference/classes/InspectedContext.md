# InspectedContext

**class** · домен «Композиция» · `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:117–141` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public ComposedContext, public IGameplayGOAPPlannedPathProvider`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Композиция».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 121 | поле | public | `std::vector<AIActionId> actionIds;` |
| 122 | метод | public | `GameplayAIDebugPlannedPathView BuildPlannedPath(std::span<const AIPlanStep> plan,` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Composition/GameplayGOAPAssetComposition.cppm:117–141`.

```cpp linenums="117"
    class InspectedContext final : public ComposedContext, public IGameplayGOAPPlannedPathProvider
    {
    public:
        using ComposedContext::ComposedContext;
        std::vector<AIActionId> actionIds;
        GameplayAIDebugPlannedPathView BuildPlannedPath(std::span<const AIPlanStep> plan,
            std::optional<std::size_t> current) const override
        {
            GameplayAIDebugPlannedPathView result;
            for (std::size_t step = current.value_or(plan.size()); step < plan.size(); ++step)
            {
                for (std::size_t index = 0; index < capabilities.size(); ++index)
                {
                    const auto* paths = dynamic_cast<const IGameplayGOAPActionPathProvider*>(capabilities[index].get());
                    if (actionIds[index] == plan[step].actionId && paths != nullptr)
                    {
                        auto route = paths->BuildDebugRoute(plan[step].contextId);
                        result.complete = result.complete && route.has_value();
                        result.routeSteps.push_back({step, plan[step].actionId, plan[step].contextId, std::move(route)});
                    }
                }
            }
            return result;
        }
    };
```
