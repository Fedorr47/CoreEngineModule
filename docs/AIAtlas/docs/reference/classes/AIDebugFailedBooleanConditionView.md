# AIDebugFailedBooleanConditionView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:45–51` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 47 | поле | public | `AIWorldFactId factId{};` |
| 48 | поле | public | `bool expected{};` |
| 49 | поле | public | `bool actual{};` |
| 50 | поле | public | `std::string factName{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:45–51`.

```cpp linenums="45"
    struct AIDebugFailedBooleanConditionView
    {
        AIWorldFactId factId{};
        bool expected{};
        bool actual{};
        std::string factName{};
    };
```
