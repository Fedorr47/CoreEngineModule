# DoorTraversalExecutor

**class** · домен «Traversal» · `src/Gameplay/Traversal/DoorTraversalExecutor.cppm:17–196` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayTraversalExecutor`

> Дверь по link: бронь, подход к InteractionPoint, логическое открытие, освобождение.

## Архитектурный разбор

**Ссылки.** [теория: 16-traversal-physics](../../chapters/16-traversal-physics.md) · [TV-DOOR](../source-references.md#tv-door) · [TV-DOOR-TICK](../source-references.md#tv-door-tick) · тест `TestDoorTraversalExecutor.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 20 | поле | public | `static constexpr GameplayTraversalTypeId kTypeId{kDoorTraversalTypeId};` |
| 22 | конструктор | public | `DoorTraversalExecutor(` |
| 26 | метод | public | `: world_(world)` |
| 27 | метод | public | `, reservationSystem_(reservationSystem)` |
| 28 | метод | public | `, steeringSettings_(steeringSettings)` |
| 32 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Start(` |
| 74 | метод | public | `[[nodiscard]] GameplayTraversalExecutionResult Tick(` |
| 133 | метод | public | `void Cancel(const GameplayTraversalExecutionContext& context) noexcept override` |
| 155 | метод | private | `[[nodiscard]] static bool ContextsMatch_(const GameplayTraversalExecutionContext& left,` |
| 162 | метод | private | `void ApplyMovement_(const EntityHandle agent, const GameplayMovementIntent& movement) const noexcept` |
| 174 | метод | private | `void ClearMovementIfAccessible_(const EntityHandle agent) const noexcept` |
| 192 | поле | private | `GameplayWorld& world_;` |
| 193 | поле | private | `GameplayObjectReservationSystem& reservationSystem_;` |
| 194 | поле | private | `GameplayArrivalSteeringSettings steeringSettings_{};` |
| 195 | поле | private | `std::unordered_map<EntityHandle, ActiveDoorTraversal> activeByAgent_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Traversal/DoorTraversalExecutor.cppm:17–77`.

```cpp linenums="17"
    class DoorTraversalExecutor final : public IGameplayTraversalExecutor
    {
    public:
        static constexpr GameplayTraversalTypeId kTypeId{kDoorTraversalTypeId};

        DoorTraversalExecutor(
            GameplayWorld& world,
            GameplayObjectReservationSystem& reservationSystem,
            GameplayArrivalSteeringSettings steeringSettings = {}) noexcept
            : world_(world)
            , reservationSystem_(reservationSystem)
            , steeringSettings_(steeringSettings)
        {
        }

        [[nodiscard]] GameplayTraversalExecutionResult Start(
            const GameplayTraversalExecutionContext& context) override
        {
            const bool bHasValidContext = context.IsValid();
            const bool bIsDoorTraversal = context.traversalTypeId == kDoorTraversalTypeId;
            const bool bHasValidAgent = bHasValidContext && world_.IsEntityValid(context.agentEntity);
            const bool bIsAIAgent = bHasValidAgent && world_.HasAI(context.agentEntity);
            const bool bHasMovementComponents = bHasValidAgent && world_.HasTransform(context.agentEntity) &&
                world_.HasCharacterCommand(context.agentEntity) && world_.HasCharacterMotor(context.agentEntity) &&
                world_.HasCharacterMovementState(context.agentEntity);
            const bool bHasValidDoor = bHasValidContext && world_.IsEntityValid(context.targetEntity);
            const bool bIsDoor = bHasValidDoor && world_.HasDoor(context.targetEntity);
            const bool bHasInteractionPoint = bHasValidDoor && world_.HasInteractionPoint(context.targetEntity);
            const bool bHasNoActiveTraversal = !activeByAgent_.contains(context.agentEntity);
            if (!bHasValidContext || !bIsDoorTraversal || !bIsAIAgent || !bHasMovementComponents ||
                !bIsDoor || !bHasInteractionPoint || !bHasNoActiveTraversal)
            {
                return GameplayTraversalExecutionResult::Failed;
            }

            const bool bIsAlreadyReserved = reservationSystem_.IsReserved(context.targetEntity);
            if (bIsAlreadyReserved)
            {
                return GameplayTraversalExecutionResult::Failed;
            }
            if (!reservationSystem_.TryReserve(world_, context.targetEntity, context.agentEntity))
            {
                return GameplayTraversalExecutionResult::Failed;
            }

            const auto interactionPoint = ResolveGameplayInteractionPoint(world_, context.targetEntity);
            if (!interactionPoint)
            {
                (void)reservationSystem_.Release(context.targetEntity, context.agentEntity);
                return GameplayTraversalExecutionResult::Failed;
            }

            activeByAgent_.emplace(context.agentEntity, ActiveDoorTraversal{context, *interactionPoint});
            ClearMovementIfAccessible_(context.agentEntity);
            return GameplayTraversalExecutionResult::Running;
        }

        [[nodiscard]] GameplayTraversalExecutionResult Tick(
            const GameplayTraversalExecutionContext& context, float) override
        {
            auto it = activeByAgent_.find(context.agentEntity);
```
