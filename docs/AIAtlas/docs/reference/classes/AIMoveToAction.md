# AIMoveToAction

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:35–110` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Навигационные действия».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 38 | метод | public | `[[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(` |
| 75 | метод | public | `[[nodiscard]] static AIActionExecutionStatus Start(` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToAction.cppm:35–95`.

```cpp linenums="35"
    class AIMoveToAction
    {
    public:
        [[nodiscard]] static std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const AIMoveToActionRequest& request,
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
            GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr)
        {
            if (context.actionId != kAIMoveToActionId || request.routeGraph == nullptr)
            {
                return nullptr;
            }

            GameplayRouteSearchResult searchResult = FindWeightedGameplayRoute(
                *request.routeGraph, request.startNodeId, request.goalNodeId);
            if (!searchResult.Succeeded())
            {
                return nullptr;
            }

            return AIFollowRouteAction::CreateRuntime(
                world,
                traversalLinkRegistry,
                traversalExecutorRegistry,
                context.agentEntity,
                std::move(searchResult.route),
                request.steeringSettings,
                obstacleQuery,
                obstacleSettings,
                debugRegistry,
                {},
                navigationDebugRegistry);
        }

        [[nodiscard]] static AIActionExecutionStatus Start(
            AISystem& aiSystem,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            const EntityHandle agentEntity,
            const GameplayRouteGraph& routeGraph,
            const GameplayRouteNodeId startNodeId,
            const GameplayRouteNodeId goalNodeId,
            const GameplayArrivalSteeringSettings& steeringSettings = {},
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            const GameplayObstacleAvoidanceSettings& obstacleSettings = {},
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
            GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr)
        {
            const AIActionRuntimeContext context{
                .agentEntity = agentEntity,
                .actionId = kAIMoveToActionId};

            const AIMoveToActionRequest request{
                .routeGraph = &routeGraph,
```
