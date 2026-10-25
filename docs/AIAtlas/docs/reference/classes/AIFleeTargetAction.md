# AIFleeTargetAction

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFleeTargetAction.cppm:18–76` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 21 | метод | public | `[[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(` |
| 38 | метод | public | `[[nodiscard]] static AIActionExecutionStatus Start(` |
| 61 | метод | private | `[[nodiscard]] static bool CanCreateRuntime_(` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFleeTargetAction.cppm:18–76`.

```cpp linenums="18"
    class AIFleeTargetAction
    {
    public:
        [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity,
            const AIFleeTargetSettings& settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr)
        {
            if (!CanCreateRuntime_(world, agentEntity, targetEntity))
            {
                return nullptr;
            }
            return std::make_unique<AIFleeTargetActionRuntime>(
                world, targetEntity, settings, obstacleQuery, obstacleSettings, debugRegistry);
        }

        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity,
            const AIFleeTargetSettings& settings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr)
        {
            const AIActionRuntimeContext context{
                .agentEntity = agentEntity,
                .actionId = kAIFleeTargetActionId};
            std::unique_ptr<IAIActionRuntime> runtime = CreateRuntime(
                world, agentEntity, targetEntity, settings, obstacleQuery, obstacleSettings, debugRegistry);
            if (!context.IsValid() || runtime == nullptr)
            {
                return AIActionExecutionStatus::Failed;
            }
            return aiSystem.StartAction(world, context, std::move(runtime));
        }

    private:
        [[nodiscard]] static bool CanCreateRuntime_(
            const GameplayWorld& world,
            const EntityHandle agentEntity,
            const EntityHandle targetEntity) noexcept
        {
            const bool bHasAgent = world.IsEntityValid(agentEntity);
            const bool bHasTarget = world.IsEntityValid(targetEntity);
            return bHasAgent && world.HasAI(agentEntity) &&
                world.HasTransform(agentEntity) &&
                world.HasCharacterCommand(agentEntity) &&
                world.HasCharacterMotor(agentEntity) &&
                   world.HasCharacterMovementState(agentEntity) &&
                bHasTarget &&
                world.HasTransform(targetEntity);
        }
    };
```
