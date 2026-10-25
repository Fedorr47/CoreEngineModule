# AIFollowTargetActionRuntime

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:27–207` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionRuntime`

> Следование за живой целью: Seek с выборкой и переизданием команды; бесконечно Running.

## Архитектурный разбор

**Ссылки.** [теория: 14-temporal-behaviors](../../chapters/14-temporal-behaviors.md) · [схема d14-sampling](../diagrams.md#d14-sampling) · [NA-FOLLOW-RT](../source-references.md#na-follow-rt) · [NA-FOLLOW-TICK](../source-references.md#na-follow-tick)

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 30 | конструктор | public | `AIFollowTargetActionRuntime(` |
| 37 | метод | public | `: world_(world)` |
| 38 | метод | public | `, targetEntity_(targetEntity)` |
| 39 | метод | public | `, settings_(settings)` |
| 40 | метод | public | `, obstacleQuery_(obstacleQuery)` |
| 41 | метод | public | `, obstacleSettings_(obstacleSettings)` |
| 42 | метод | public | `, debugRegistry_(debugRegistry)` |
| 48 | метод | public | `[[nodiscard]] AIActionRuntimeResult Start(` |
| 64 | метод | public | `[[nodiscard]] AIActionRuntimeResult Tick(` |
| 98 | метод | public | `void Cancel(const AIActionRuntimeContext& context) noexcept override` |
| 109 | метод | private | `[[nodiscard]] bool Validate_(const AIActionRuntimeContext& context) const noexcept` |
| 121 | метод | private | `void RefreshSteering_(const EntityHandle agentEntity) noexcept` |
| 163 | метод | private | `void ApplyCurrentMovement_(const EntityHandle agentEntity) const noexcept` |
| 180 | метод | private | `void ClearMovementIfAccessible_(const EntityHandle agentEntity) noexcept` |
| 198 | поле | private | `GameplayWorld& world_;` |
| 199 | поле | private | `EntityHandle targetEntity_{kNullEntity};` |
| 200 | поле | private | `AIFollowTargetSettings settings_{};` |
| 201 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 202 | поле | private | `GameplayObstacleAvoidanceSettings obstacleSettings_{};` |
| 203 | поле | private | `GameplaySteeringDebugRegistry* debugRegistry_{nullptr};` |
| 204 | поле | private | `GameplayMovementIntent currentMovement_{};` |
| 205 | поле | private | `GameplayObstacleAvoidanceState obstacleAvoidanceState_{};` |
| 206 | поле | private | `float elapsedSinceSteeringUpdate_{0.0f};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFollowTargetActionRuntime.cppm:27–87`.

```cpp linenums="27"
    class AIFollowTargetActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFollowTargetActionRuntime(
            GameplayWorld& world,
            const EntityHandle targetEntity,
            AIFollowTargetSettings settings = {},
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
            settings_.steeringUpdateIntervalSeconds =
                std::max(settings_.steeringUpdateIntervalSeconds, 0.0f);
        }

        [[nodiscard]] AIActionRuntimeResult Start(
            const AIActionRuntimeContext& context) override
        {
            elapsedSinceSteeringUpdate_ = 0.0f;
            obstacleAvoidanceState_ = {};
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
                return AIActionRuntimeResult::Failed;
            }

            if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
            {
                elapsedSinceSteeringUpdate_ += deltaSeconds;
            }

            if (settings_.steeringUpdateIntervalSeconds <= 0.0f ||
                elapsedSinceSteeringUpdate_ >= settings_.steeringUpdateIntervalSeconds)
            {
                RefreshSteering_(context.agentEntity);
                elapsedSinceSteeringUpdate_ =
                    settings_.steeringUpdateIntervalSeconds > 0.0f
                    ? std::fmod(
                        elapsedSinceSteeringUpdate_,
                        settings_.steeringUpdateIntervalSeconds)
```
