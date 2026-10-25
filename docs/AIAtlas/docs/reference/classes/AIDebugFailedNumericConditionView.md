# AIDebugFailedNumericConditionView

**struct** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:53–60` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 55 | поле | public | `AIWorldIntegerFactId factId{};` |
| 56 | поле | public | `AINumericConditionOperator comparison{AINumericConditionOperator::Equal};` |
| 57 | поле | public | `std::int32_t expected{};` |
| 58 | поле | public | `std::int32_t actual{};` |
| 59 | поле | public | `std::string factName{};` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/AIDebugViewModel.cppm:53–60`.

```cpp linenums="53"
    struct AIDebugFailedNumericConditionView
    {
        AIWorldIntegerFactId factId{};
        AINumericConditionOperator comparison{AINumericConditionOperator::Equal};
        std::int32_t expected{};
        std::int32_t actual{};
        std::string factName{};
    };
```
