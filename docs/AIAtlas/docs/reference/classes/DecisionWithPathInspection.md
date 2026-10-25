# DecisionWithPathInspection

**class** · домен «Решение (instance)» · `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:149–174` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Решение (instance)».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 154 | конструктор | public | `DecisionWithPathInspection(const EntityHandle agent, GameplayGOAPDecisionSetup setup,` |
| 156 | метод | public | `: GameplayGOAPDecisionInstance(agent, std::move(setup)), paths_(paths)` |
| 160 | метод | public | `[[nodiscard]] GameplayAIDebugPlannedPathView BuildPlannedPathDebugView() const override` |
| 173 | поле | private | `const IGameplayGOAPPlannedPathProvider& paths_;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/GameplayGOAPDecisionInstance.cppm:149–174`.

```cpp linenums="149"
        class DecisionWithPathInspection final :
            public GameplayGOAPDecisionInstance,
            public IGameplayGOAPPathInspection
        {
        public:
            DecisionWithPathInspection(const EntityHandle agent, GameplayGOAPDecisionSetup setup,
                const IGameplayGOAPPlannedPathProvider& paths)
                : GameplayGOAPDecisionInstance(agent, std::move(setup)), paths_(paths)
            {
            }

            [[nodiscard]] GameplayAIDebugPlannedPathView BuildPlannedPathDebugView() const override
            {
                const AIDebugViewModel plan = BuildDebugViewModel();
                std::vector<AIPlanStep> steps;
                steps.reserve(plan.selectedPlan.size());
                for (const AIDebugPlanStepView& step : plan.selectedPlan)
                {
                    steps.push_back({step.actionId, step.contextId});
                }
                return paths_.BuildPlannedPath(steps, plan.currentStepIndex);
            }

        private:
            const IGameplayGOAPPlannedPathProvider& paths_;
        };
```
