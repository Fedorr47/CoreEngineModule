# IAIActionRuntime

**class** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:41–66` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Протокол исполнения шага: Start → Running/Succeeded/Failed, Tick, Cancel (без применения эффектов).

## Архитектурный разбор

**Владелец и время жизни.** Реализации принадлежат AIActionTask.

**Кто вызывает и в какой фазе.** AIActionTask.

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [AR-IFACE](../source-references.md#ar-iface) · [AR-RESULT](../source-references.md#ar-result) · тест `TestAIActionRuntime.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 44 | метод | public | `virtual ~IAIActionRuntime() = default;` |
| 50 | метод | public | `[[nodiscard]] virtual AIActionRuntimeResult Start(` |
| 57 | метод | public | `[[nodiscard]] virtual AIActionRuntimeResult Tick(` |
| 64 | метод | public | `virtual void Cancel(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionRuntime.cppm:41–66`.

```cpp linenums="41"
    class IAIActionRuntime
    {
    public:
        virtual ~IAIActionRuntime() = default;

        // Starts one action invocation for the supplied agent/action pair. The
        // caller owns lifecycle ordering and must observe only Running,
        // Succeeded, or Failed as valid start results; NotStarted is reserved
        // for future task-side state before Start is called.
        [[nodiscard]] virtual AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) = 0;

        // Advances an invocation that previously returned Running from Start.
        // The caller, not this interface, decides when Tick is allowed and how
        // terminal states are handled; valid tick results are Running,
        // Succeeded, or Failed.
        [[nodiscard]] virtual AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            float deltaSeconds) = 0;

        // Provides an explicit interruption and cleanup boundary for a running
        // invocation. Cancellation does not apply planner effects; future task
        // state remains responsible for deciding when cancellation is legal.
        virtual void Cancel(
            const AIActionRuntimeContext& context) noexcept = 0;
    };
```
