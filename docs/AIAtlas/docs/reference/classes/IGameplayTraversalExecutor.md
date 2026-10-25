# IGameplayTraversalExecutor

**class** · домен «Traversal» · `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:36–45` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Traversal».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 39 | метод | public | `virtual ~IGameplayTraversalExecutor() = default;` |
| 40 | метод | public | `[[nodiscard]] virtual GameplayTraversalExecutionResult Start(` |
| 42 | метод | public | `[[nodiscard]] virtual GameplayTraversalExecutionResult Tick(` |
| 44 | метод | public | `virtual void Cancel(const GameplayTraversalExecutionContext& context) noexcept = 0;` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Traversal/GameplayTraversalExecutor.cppm:36–45`.

```cpp linenums="36"
    class IGameplayTraversalExecutor
    {
    public:
        virtual ~IGameplayTraversalExecutor() = default;
        [[nodiscard]] virtual GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) = 0;
        [[nodiscard]] virtual GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, float deltaSeconds) = 0;
        virtual void Cancel(const GameplayTraversalExecutionContext& context) noexcept = 0;
    };
```
