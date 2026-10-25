# IGameplayGOAPInspection

**class** · домен «Диагностика» · `src/Gameplay/AI/GOAP/Debug/GameplayGOAPInspection.cppm:11–19` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Диагностика».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 14 | метод | public | `virtual ~IGameplayGOAPInspection() = default;` |
| 16 | метод | public | `[[nodiscard]] virtual AIPlanExecutionStatus GetGOAPStatus() const noexcept = 0;` |
| 17 | метод | public | `[[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;` |
| 18 | метод | public | `[[nodiscard]] virtual AIDebugViewModel BuildDebugViewModel() const = 0;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/GOAP/Debug/GameplayGOAPInspection.cppm:11–19`.

```cpp linenums="11"
    class IGameplayGOAPInspection
    {
    public:
        virtual ~IGameplayGOAPInspection() = default;

        [[nodiscard]] virtual AIPlanExecutionStatus GetGOAPStatus() const noexcept = 0;
        [[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;
        [[nodiscard]] virtual AIDebugViewModel BuildDebugViewModel() const = 0;
    };
```
