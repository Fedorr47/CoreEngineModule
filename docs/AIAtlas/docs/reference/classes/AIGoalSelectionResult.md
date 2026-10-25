# AIGoalSelectionResult

**struct** · домен «Ядро GOAP» · `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:16–25` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ядро GOAP».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 18 | поле | public | `AIGoalId goalId{};` |
| 19 | поле | public | `float score{};` |
| 21 | метод | public | `[[nodiscard]] constexpr bool IsValid() const noexcept` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/AIGoalSelection.cppm:16–25`.

```cpp linenums="16"
    struct AIGoalSelectionResult
    {
        AIGoalId goalId{};
        float score{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return goalId.IsValid();
        }
    };
```
