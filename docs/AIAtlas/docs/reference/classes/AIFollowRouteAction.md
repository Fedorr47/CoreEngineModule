# AIFollowRouteAction

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm:24–120` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 27 | метод | public | `[[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(` |
| 58 | метод | public | `[[nodiscard]] static AIActionExecutionStatus Start(` |
| 103 | метод | private | `[[nodiscard]] static bool CanCreateRuntime_(` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFollowRouteAction.cppm:24–84`.

```cpp linenums="24"
    class AIFollowRouteAction
    {
    public:
        [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const EntityHandle agentEntity,
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
            const GameplayRouteFollowerSettings& followerSettings = {},
            GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr)
        {
            if (!CanCreateRuntime_(world, agentEntity, route))
            {
                return nullptr;
            }

            return std::make_unique<AIFollowRouteActionRuntime>(
                world,
                traversalLinkRegistry,
                traversalExecutorRegistry,
                std::move(route),
                steeringSettings,
                obstacleQuery,
                obstacleSettings,
                debugRegistry,
                followerSettings,
                navigationDebugRegistry);
        }

        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const EntityHandle agentEntity,
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
            const GameplayRouteFollowerSettings& followerSettings = {},
            GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr)
        {

            const AIActionRuntimeContext context
            {
                .agentEntity = agentEntity,
                .actionId =  kAIFollowRouteActionId
            };
            if (!context.IsValid())
            {
                return AIActionExecutionStatus::Failed;
            }

            std::unique_ptr<IAIActionRuntime> runtime = CreateRuntime(
                world,
```
