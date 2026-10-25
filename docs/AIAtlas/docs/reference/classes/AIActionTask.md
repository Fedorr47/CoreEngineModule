# AIActionTask

**class** · домен «Action runtime» · `src/Gameplay/AI/Runtime/AIActionTask.cppm:14–124` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Владеющая обёртка IAIActionRuntime со статусом и порядком протокола Start/Tick/Cancel.

## Архитектурный разбор

**Владелец и время жизни.** Значение в карте AISystem; владеет unique_ptr runtime.

**Runtime-методы, входы и выходы.** Start (однократно), Tick (только Running), Cancel (только Running).

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [схема d09-task-states](../diagrams.md#d09-task-states) · [AT-CLASS](../source-references.md#at-class) · [AT-CANCEL](../source-references.md#at-cancel) · тест `TestAIActionTask.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 17 | конструктор | public | `AIActionTask(` |
| 20 | метод | public | `: context_(context)` |
| 21 | метод | public | `, runtime_(std::move(runtime))` |
| 27 | конструктор | public | `AIActionTask(const AIActionTask&) = delete;` |
| 28 | поле | public | `AIActionTask& operator=(const AIActionTask&) = delete;` |
| 29 | конструктор | public | `AIActionTask(AIActionTask&&) noexcept = default;` |
| 30 | поле | public | `AIActionTask& operator=(AIActionTask&&) noexcept = default;` |
| 32 | метод | public | `[[nodiscard]] AIActionExecutionStatus GetStatus() const noexcept` |
| 37 | метод | public | `[[nodiscard]] const AIActionRuntimeContext& GetContext() const noexcept` |
| 42 | метод | public | `[[nodiscard]] bool IsRunning() const noexcept` |
| 47 | метод | public | `[[nodiscard]] bool IsTerminal() const noexcept` |
| 52 | метод | public | `[[nodiscard]] AIActionExecutionStatus Start()` |
| 63 | метод | public | `[[nodiscard]] AIActionExecutionStatus Tick(const float deltaSeconds)` |
| 75 | метод | public | `void Cancel() noexcept` |
| 88 | метод | private | `[[nodiscard]] static constexpr  AIActionExecutionStatus MapRuntimeResultToExecutionStatus(` |
| 104 | метод | private | `[[nodiscard]] static constexpr bool IsTerminalStatus(` |
| 121 | поле | private | `AIActionRuntimeContext context_{};` |
| 122 | поле | private | `std::unique_ptr<IAIActionRuntime> runtime_{};` |
| 123 | поле | private | `AIActionExecutionStatus status_{ AIActionExecutionStatus::NotStarted };` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Runtime/AIActionTask.cppm:14–74`.

```cpp linenums="14"
    class AIActionTask
    {
    public:
        AIActionTask(
            AIActionRuntimeContext context,
            std::unique_ptr<IAIActionRuntime> runtime) noexcept
        : context_(context)
        , runtime_(std::move(runtime))
        {
            assert(context_.IsValid() && "AIActionTask requires a valid runtime context.");
            assert(runtime_ != nullptr && "AIActionTask requires an owned runtime");
        }

        AIActionTask(const AIActionTask&) = delete;
        AIActionTask& operator=(const AIActionTask&) = delete;
        AIActionTask(AIActionTask&&) noexcept = default;
        AIActionTask& operator=(AIActionTask&&) noexcept = default;

        [[nodiscard]] AIActionExecutionStatus GetStatus() const noexcept
        {
            return status_;
        }

        [[nodiscard]] const AIActionRuntimeContext& GetContext() const noexcept
        {
            return context_;
        }

        [[nodiscard]] bool IsRunning() const noexcept
        {
            return status_ == AIActionExecutionStatus::Running;
        }

        [[nodiscard]] bool IsTerminal() const noexcept
        {
            return IsTerminalStatus(status_);
        }

        [[nodiscard]] AIActionExecutionStatus Start()
        {
            if (status_ != AIActionExecutionStatus::NotStarted || runtime_ == nullptr)
            {
                return status_;
            }

            status_ = MapRuntimeResultToExecutionStatus(runtime_->Start(context_));
            return status_;
        }

        [[nodiscard]] AIActionExecutionStatus Tick(const float deltaSeconds)
        {
            if (status_ != AIActionExecutionStatus::Running || runtime_ == nullptr)
            {
                return status_;
            }

            status_ = MapRuntimeResultToExecutionStatus(
                runtime_->Tick(context_, deltaSeconds));
            return status_;
        }

```
