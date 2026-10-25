# GameplayUnsupportedTraversalExecutor

**class** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:47–59` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayTraversalExecutor`

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 50 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Start(const GameplayTraversalExecutionContext&) override` |
| 54 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Tick(const GameplayTraversalExecutionContext&, float) override` |
| 58 | метод | public | `void Cancel(const GameplayTraversalExecutionContext&) noexcept override {}` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:47–59`.

```cpp linenums="47"
    class GameplayUnsupportedTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        [[nodiscard]] GameplayTraversalExecutionResult Start(const GameplayTraversalExecutionContext&) override
        {
            return GameplayTraversalExecutionResult::Failed;
        }
        [[nodiscard]] GameplayTraversalExecutionResult Tick(const GameplayTraversalExecutionContext&, float) override
        {
            return GameplayTraversalExecutionResult::Failed;
        }
        void Cancel(const GameplayTraversalExecutionContext&) noexcept override {}
    };
```
