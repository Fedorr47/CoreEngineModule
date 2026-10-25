# AIDebugActionApplicabilityView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:62–71` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 64 | поле | public | `AIActionId actionId{};` |
| 65 | поле | public | `AIActionContextId contextId{};` |
| 66 | поле | public | `bool applicable{};` |
| 67 | поле | public | `std::vector<AIDebugFailedBooleanConditionView> failedBooleanConditions{};` |
| 68 | поле | public | `std::vector<AIDebugFailedNumericConditionView> failedNumericConditions{};` |
| 69 | поле | public | `std::string actionName{};` |
| 70 | поле | public | `std::string contextName{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:62–71`.

```cpp linenums="62"
    struct AIDebugActionApplicabilityView
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        bool applicable{};
        std::vector<AIDebugFailedBooleanConditionView> failedBooleanConditions{};
        std::vector<AIDebugFailedNumericConditionView> failedNumericConditions{};
        std::string actionName{};
        std::string contextName{};
    };
```
