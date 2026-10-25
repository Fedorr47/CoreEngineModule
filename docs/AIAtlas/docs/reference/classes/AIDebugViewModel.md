# AIDebugViewModel

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:73–86` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 75 | поле | public | `std::vector<AIDebugBooleanFactView> booleanFacts{};` |
| 76 | поле | public | `std::vector<AIDebugIntegerFactView> integerFacts{};` |
| 77 | поле | public | `std::optional<AIGoalId> selectedGoalId{};` |
| 78 | поле | public | `std::string selectedGoalName{};` |
| 79 | поле | public | `std::vector<AIDebugPlanStepView> selectedPlan{};` |
| 80 | поле | public | `float totalPlanCost{};` |
| 81 | поле | public | `bool bPlanCostComplete{true};` |
| 82 | поле | public | `AIPlanExecutionStatus decisionStatus{AIPlanExecutionStatus::NotStarted};` |
| 83 | поле | public | `std::optional<AIPlanExecutionStatus> executionStatus{};` |
| 84 | поле | public | `std::optional<std::size_t> currentStepIndex{};` |
| 85 | поле | public | `std::vector<AIDebugActionApplicabilityView> actionApplicability{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:73–86`.

```cpp linenums="73"
    struct AIDebugViewModel
    {
        std::vector<AIDebugBooleanFactView> booleanFacts{};
        std::vector<AIDebugIntegerFactView> integerFacts{};
        std::optional<AIGoalId> selectedGoalId{};
        std::string selectedGoalName{};
        std::vector<AIDebugPlanStepView> selectedPlan{};
        float totalPlanCost{};
        bool bPlanCostComplete{true};
        AIPlanExecutionStatus decisionStatus{AIPlanExecutionStatus::NotStarted};
        std::optional<AIPlanExecutionStatus> executionStatus{};
        std::optional<std::size_t> currentStepIndex{};
        std::vector<AIDebugActionApplicabilityView> actionApplicability{};
    };
```
