# AIDebugPlanStepView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:35–43` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 37 | поле | public | `AIActionId actionId{};` |
| 38 | поле | public | `AIActionContextId contextId{};` |
| 39 | поле | public | `float cost{};` |
| 40 | поле | public | `bool bCostResolved{};` |
| 41 | поле | public | `std::string actionName{};` |
| 42 | поле | public | `std::string contextName{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:35–43`.

```cpp linenums="35"
    struct AIDebugPlanStepView
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        float cost{};
        bool bCostResolved{};
        std::string actionName{};
        std::string contextName{};
    };
```
