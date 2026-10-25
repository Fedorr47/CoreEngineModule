# JumpTraversalExecutor

**class** · домен «Traversal» · `src/Gameplay/Traversal/JumpTraversalExecutor.cppm:18–292` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayTraversalExecutor`

> Прыжок по link: подход к takeoff, запрос kGameplayActionJump, ожидание взлёта, приземление в допусках, тайм-аут 10 с.

## Архитектурный разбор

**Владелец и время жизни.** Член GameplayRuntime; зарегистрирован runtime-owned в реестре executors.

**Ссылки.** [теория: 16-traversal-physics](../../chapters/16-traversal-physics.md) · [схема d16-jump-states](../diagrams.md#d16-jump-states) · [TV-JUMP](../source-references.md#tv-jump) · [TV-JUMP-TICK](../source-references.md#tv-jump-tick) · [TV-JUMP-TIMEOUT](../source-references.md#tv-jump-timeout) · тест `TestJumpTraversalExecutor.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 21 | поле | public | `static constexpr GameplayTraversalTypeId kTypeId{kJumpTraversalTypeId};` |
| 23 | конструктор | public | `explicit JumpTraversalExecutor(` |
| 27 | метод | public | `: world_(world), linkRegistry_(linkRegistry), steeringSettings_(steeringSettings)` |
| 31 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Start(` |
| 49 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Tick(` |
| 147 | метод | public | `void Cancel(const GameplayTraversalExecutionContext& context) noexcept override` |
| 172 | поле | private | `static constexpr float kTraversalTimeoutSeconds = 10.0f;` |
| 174 | метод | private | `[[nodiscard]] std::optional<GameplayTraversalLink> ResolveExpectedLink_(` |
| 187 | метод | private | `[[nodiscard]] bool HasRequiredAgentState_(const EntityHandle agent) const noexcept` |
| 196 | метод | private | `[[nodiscard]] GameplayTraversalExecutionResult Fail_(` |
| 209 | метод | private | `static bool ContextsMatch_(const GameplayTraversalExecutionContext& left,` |
| 216 | метод | private | `static float PlanarDistanceSquared_(const mathUtils::Vec3& a, const mathUtils::Vec3& b) noexcept` |
| 223 | метод | private | `void ApplyMovement_(const EntityHandle agent, const GameplayMovementIntent& movement) noexcept` |
| 230 | метод | private | `void ApplyLandingMovement_(const EntityHandle agent, const mathUtils::Vec3& position,` |
| 243 | метод | private | `void ClearMovement_(const EntityHandle agent) noexcept` |
| 255 | метод | private | `void RestoreVerticalSpeed_(const EntityHandle agent, const ActiveJump& active) noexcept` |
| 266 | метод | private | `void CleanupIssuedRequest_(const EntityHandle agent, const ActiveJump& active) noexcept` |
| 288 | поле | private | `GameplayWorld& world_;` |
| 289 | поле | private | `const GameplayTraversalLinkRegistry& linkRegistry_;` |
| 290 | поле | private | `GameplayArrivalSteeringSettings steeringSettings_{};` |
| 291 | поле | private | `std::unordered_map<EntityHandle, ActiveJump> activeByAgent_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Traversal/JumpTraversalExecutor.cppm:18–78`.

```cpp linenums="18"
    class JumpTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        static constexpr GameplayTraversalTypeId kTypeId{kJumpTraversalTypeId};

        explicit JumpTraversalExecutor(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& linkRegistry,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
            : world_(world), linkRegistry_(linkRegistry), steeringSettings_(steeringSettings)
        {
        }

        [[nodiscard]] GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) override
        {
            const auto link = ResolveExpectedLink_(context);
            if (!link || !HasRequiredAgentState_(context.agentEntity) ||
                activeByAgent_.contains(context.agentEntity))
            {
                return GameplayTraversalExecutionResult::Failed;
            }
            const GameplayCharacterCommandComponent* command =
                world_.TryGetCharacterCommand(context.agentEntity);
            activeByAgent_.emplace(context.agentEntity, ActiveJump{
                .context = context,
                .data = link->jump,
                .wantsRun = command->wantsRun});
            return GameplayTraversalExecutionResult::Running;
        }

        [[nodiscard]] GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, const float deltaSeconds) override
        {
            auto it = activeByAgent_.find(context.agentEntity);
            if (it == activeByAgent_.end() || !ContextsMatch_(it->second.context, context) ||
                !ResolveExpectedLink_(context) || !HasRequiredAgentState_(context.agentEntity))
            {
                return Fail_(it, context.agentEntity);
            }

            ActiveJump& active = it->second;
            if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0f)
            {
                return Fail_(it, context.agentEntity);
            }
            active.elapsedSeconds += deltaSeconds;
            if (active.elapsedSeconds > kTraversalTimeoutSeconds)
            {
                return Fail_(it, context.agentEntity);
            }

            const GameplayTransformComponent* transform = world_.TryGetTransform(context.agentEntity);
            GameplayCharacterMovementStateComponent* movementState =
                world_.TryGetCharacterMovementState(context.agentEntity);
            GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(context.agentEntity);
            GameplayActionComponent* action = world_.TryGetAction(context.agentEntity);

            if (!active.requestIssued)
            {
                GameplayArrivalSteeringSettings settings = steeringSettings_;
```
