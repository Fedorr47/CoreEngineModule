# ReservedAIMoveToActionRuntime

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:37–135` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionRuntime`

> MoveTo с бронью InteractionPoint цели на время шага.

## Архитектурный разбор

**Runtime-методы, входы и выходы.** Start (отклоняет любую существующую бронь, бронирует), Tick (потеря брони → отмена, Failed), Cancel (освобождение).

**Ссылки.** [теория: 17-resources-multiagent](../../chapters/17-resources-multiagent.md) · [схема d17-contention](../diagrams.md#d17-contention) · [NA-RESERVED](../source-references.md#na-reserved) · [NA-RESERVED-START](../source-references.md#na-reserved-start)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 40 | конструктор | public | `ReservedAIMoveToActionRuntime(GameplayWorld& world,` |
| 48 | метод | public | `: world_(world), links_(links), executors_(executors), reservations_(reservations),` |
| 49 | метод | public | `request_(request), target_(target), obstacleQuery_(obstacleQuery),` |
| 50 | метод | public | `debugRegistry_(debugRegistry), navigationDebugRegistry_(navigationDebugRegistry)` |
| 54 | метод | public | `[[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override` |
| 80 | метод | public | `[[nodiscard]] AIActionRuntimeResult Tick(` |
| 103 | метод | public | `void Cancel(const AIActionRuntimeContext& context) noexcept override` |
| 114 | метод | private | `void Release_(const EntityHandle agent) noexcept` |
| 123 | поле | private | `GameplayWorld& world_;` |
| 124 | поле | private | `const GameplayTraversalLinkRegistry& links_;` |
| 125 | поле | private | `const GameplayTraversalExecutorRegistry& executors_;` |
| 126 | поле | private | `GameplayObjectReservationSystem& reservations_;` |
| 127 | поле | private | `AIMoveToActionRequest request_{};` |
| 128 | поле | private | `EntityHandle target_{kNullEntity};` |
| 129 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 130 | поле | private | `GameplaySteeringDebugRegistry* debugRegistry_{nullptr};` |
| 131 | поле | private | `GameplayNavigationDebugRegistry* navigationDebugRegistry_{nullptr};` |
| 132 | поле | private | `std::unique_ptr<IAIActionRuntime> runtime_{};` |
| 133 | поле | private | `bool ownsReservation_{};` |
| 134 | поле | private | `bool started_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:37–97`.

```cpp linenums="37"
    class ReservedAIMoveToActionRuntime final : public IAIActionRuntime
    {
    public:
        ReservedAIMoveToActionRuntime(GameplayWorld& world,
            const GameplayTraversalLinkRegistry& links,
            const GameplayTraversalExecutorRegistry& executors,
            GameplayObjectReservationSystem& reservations,
            AIMoveToActionRequest request, const EntityHandle target,
            const IGameplayObstacleQuery* obstacleQuery,
            GameplaySteeringDebugRegistry* debugRegistry,
            GameplayNavigationDebugRegistry* navigationDebugRegistry) noexcept
            : world_(world), links_(links), executors_(executors), reservations_(reservations),
            request_(request), target_(target), obstacleQuery_(obstacleQuery),
            debugRegistry_(debugRegistry), navigationDebugRegistry_(navigationDebugRegistry)
        {
        }

        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            if (started_ || reservations_.IsReserved(target_) ||
                !reservations_.TryReserve(world_, target_, context.agentEntity))
            {
                return AIActionRuntimeResult::Failed;
            }
            ownsReservation_ = true;
            runtime_ = AIMoveToAction::CreateRuntime(
                context, world_, links_, executors_, request_, obstacleQuery_, {}, debugRegistry_,
                navigationDebugRegistry_);
            if (runtime_ == nullptr)
            {
                Release_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }
            started_ = true;
            const AIActionRuntimeResult result = runtime_->Start(context);
            if (result != AIActionRuntimeResult::Running)
            {
                Release_(context.agentEntity);
                started_ = false;
            }
            return result;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context, const float deltaSeconds) override
        {
            if (!started_ || runtime_ == nullptr)
            {
                return AIActionRuntimeResult::Failed;
            }
            if (!ownsReservation_ || !reservations_.IsReservedBy(target_, context.agentEntity))
            {
                runtime_->Cancel(context);
                ownsReservation_ = false;
                started_ = false;
                return AIActionRuntimeResult::Failed;
            }
            const AIActionRuntimeResult result = runtime_->Tick(context, deltaSeconds);
            if (result != AIActionRuntimeResult::Running)
            {
                Release_(context.agentEntity);
```
