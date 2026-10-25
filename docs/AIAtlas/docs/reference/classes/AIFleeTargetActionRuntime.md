# AIFleeTargetActionRuntime

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:30–213` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionRuntime`

> Бегство от живой цели с гистерезисом trigger/safe и выборкой.

## Архитектурный разбор

**Ссылки.** [теория: 14-temporal-behaviors](../../chapters/14-temporal-behaviors.md) · [схема g14-flee-hysteresis](../diagrams.md#g14-flee-hysteresis) · [схема d14-flee-states](../diagrams.md#d14-flee-states) · [NA-FLEE-REFRESH](../source-references.md#na-flee-refresh) · [NA-FLEE-HYST](../source-references.md#na-flee-hyst) · [NA-FLEE-SETTINGS](../source-references.md#na-flee-settings)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 33 | конструктор | public | `AIFleeTargetActionRuntime(` |
| 40 | метод | public | `: world_(world)` |
| 41 | метод | public | `, targetEntity_(targetEntity)` |
| 42 | метод | public | `, settings_(settings)` |
| 43 | метод | public | `, obstacleQuery_(obstacleQuery)` |
| 44 | метод | public | `, obstacleSettings_(obstacleSettings)` |
| 45 | метод | public | `, debugRegistry_(debugRegistry)` |
| 53 | метод | public | `[[nodiscard]] AIActionRuntimeResult Start(` |
| 74 | метод | public | `[[nodiscard]] AIActionRuntimeResult Tick(` |
| 106 | метод | public | `void Cancel(const AIActionRuntimeContext& context) noexcept override` |
| 114 | метод | private | `[[nodiscard]] bool Validate_(const AIActionRuntimeContext& context) const noexcept` |
| 127 | метод | private | `void RefreshSteering_(const EntityHandle agentEntity) noexcept` |
| 168 | метод | private | `void ApplyCurrentMovement_(const EntityHandle agentEntity) const noexcept` |
| 185 | метод | private | `void ClearMovementIfAccessible_(const EntityHandle agentEntity) noexcept` |
| 203 | поле | private | `GameplayWorld& world_;` |
| 204 | поле | private | `EntityHandle targetEntity_{kNullEntity};` |
| 205 | поле | private | `AIFleeTargetSettings settings_{};` |
| 206 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 207 | поле | private | `GameplayObstacleAvoidanceSettings obstacleSettings_{};` |
| 208 | поле | private | `GameplaySteeringDebugRegistry* debugRegistry_{nullptr};` |
| 209 | поле | private | `GameplayMovementIntent currentMovement_{};` |
| 210 | поле | private | `GameplayObstacleAvoidanceState obstacleAvoidanceState_{};` |
| 211 | поле | private | `float elapsedSinceSteeringUpdate_{0.0f};` |
| 212 | поле | private | `bool isFleeing_{false};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFleeTargetActionRuntime.cppm:30–90`.

```cpp linenums="30"
    class AIFleeTargetActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFleeTargetActionRuntime(
            GameplayWorld& world,
            const EntityHandle targetEntity,
            AIFleeTargetSettings settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            GameplayObstacleAvoidanceSettings obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr) noexcept
        : world_(world)
        , targetEntity_(targetEntity)
        , settings_(settings)
        , obstacleQuery_(obstacleQuery)
        , obstacleSettings_(obstacleSettings)
        , debugRegistry_(debugRegistry)
        {
            settings_.triggerRadius = std::max(settings_.triggerRadius, 0.0f);
            settings_.safeRadius = std::max(settings_.safeRadius, settings_.triggerRadius);
            settings_.steeringUpdateIntervalSeconds =
                std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
        }

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            elapsedSinceSteeringUpdate_ = 0.0f;
            isFleeing_ = false;
            obstacleAvoidanceState_ = {};
            if (debugRegistry_ != nullptr)
            {
                debugRegistry_->Clear(context.agentEntity);
            }
            if (!Validate_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }

            RefreshSteering_(context.agentEntity);
            ApplyCurrentMovement_(context.agentEntity);
            return AIActionRuntimeResult::Running;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            if (!Validate_(context))
            {
                ClearMovementIfAccessible_(context.agentEntity);
                isFleeing_ = false;
                return AIActionRuntimeResult::Failed;
            }

            if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
            {
                elapsedSinceSteeringUpdate_ += deltaSeconds;
            }

            if (settings_.steeringUpdateIntervalSeconds <= 0.0f ||
```
