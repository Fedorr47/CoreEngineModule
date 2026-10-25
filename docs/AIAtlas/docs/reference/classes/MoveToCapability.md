# MoveToCapability

**class** · домен «Компоненты» · `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:92–247` · внутренняя сущность модуля · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IGameplayGOAPCapability,`

> Provider move_to: компиляция графа маршрутов, route costs, запросы MoveTo, бронь цели, debug-маршрут.

## Архитектурный разбор

**Владелец и время жизни.** unique_ptr в ComposedContext instance; binding заимствует его.

**Runtime-методы, входы и выходы.** CreateBinding (без obstacle query и debug), CostOverrides, ResolveRequest (sourceRadius), ResolveReservationTarget, BuildDebugRoute.

**Ошибки, terminal states, отмена.** Ошибки создания: радиусы, нет пути, routeCost без графа, неверный traversal link.

**Ссылки.** [теория: 11-spatial-routes](../../chapters/11-spatial-routes.md) · [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [схема d11-route-sources](../diagrams.md#d11-route-sources) · [CP-MOVE-CAP](../source-references.md#cp-move-cap) · [CP-MOVE-GRAPH](../source-references.md#cp-move-graph) · [CP-MOVE-RESOLVE](../source-references.md#cp-move-resolve) · [CP-MOVE-BINDING](../source-references.md#cp-move-binding) · тест `TestAIMoveToActionBinding.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 97 | конструктор | public | `MoveToCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)` |
| 98 | метод | public | `: world_(context.services.world), links_(context.services.traversalLinkRegistry),` |
| 99 | метод | public | `executors_(context.services.traversalExecutorRegistry), reservations_(context.services.reservationSystem)` |
| 171 | метод | public | `std::unique_ptr<IAIActionBinding> CreateBinding(AIAgentWorldState&, std::vector<GameplayWorldEvent>&) override` |
| 175 | метод | public | `std::vector<GameplayGOAPActionCostOverride> CostOverrides() const override { return costs_; }` |
| 176 | метод | public | `std::optional<GameplayRoute> BuildDebugRoute(AIActionContextId id) const override` |
| 186 | метод | public | `EntityHandle ResolveReservationTarget(const AIActionRuntimeContext& context) override` |
| 191 | метод | public | `std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override` |
| 235 | метод | private | `const Route* Find(AIActionContextId id) const` |
| 240 | поле | private | `GameplayWorld& world_;` |
| 241 | поле | private | `const GameplayTraversalLinkRegistry& links_;` |
| 242 | поле | private | `const GameplayTraversalExecutorRegistry& executors_;` |
| 243 | поле | private | `GameplayObjectReservationSystem& reservations_;` |
| 244 | поле | private | `std::vector<GameplayRouteGraph> graphs_;` |
| 245 | поле | private | `std::vector<Route> routes_;` |
| 246 | поле | private | `std::vector<GameplayGOAPActionCostOverride> costs_;` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Composition/Components/GameplayGOAPMoveToComponent.cppm:92–152`.

```cpp linenums="92"
    class MoveToCapability final : public IGameplayGOAPCapability,
        public IAIMoveToActionRequestProvider, public IAIActionReservationTargetProvider,
        public IGameplayGOAPActionPathProvider
    {
    public:
        MoveToCapability(std::span<const GameplayAICapabilityAsset> assets, const GameplayGOAPCompositionContext& context)
            : world_(context.services.world), links_(context.services.traversalLinkRegistry),
              executors_(context.services.traversalExecutorRegistry), reservations_(context.services.reservationSystem)
        {
            std::optional<AuthoredGraph> authored;
            if (context.routeGraph != nullptr)
            {
                authored = CompileGraph(context);
                graphs_.push_back(std::move(authored->graph));
            }
            for (const auto& asset : assets)
            {
                const auto& parameters = context.Parameters<GameplayAIMoveToAsset>(asset);
                if (!(parameters.acceptanceRadius > 0)
                    || !std::isfinite(parameters.acceptanceRadius * parameters.acceptanceRadius)
                    || !std::isfinite(parameters.slowingRadius) || parameters.slowingRadius < parameters.acceptanceRadius
                    || parameters.sourceRadius < 0 || !std::isfinite(parameters.sourceRadius * parameters.sourceRadius))
                {
                    context.Fail(asset.context, "require finite slowingRadius >= acceptanceRadius > 0 and sourceRadius >= 0");
                }
                const auto id = context.definition.FindActionContext(asset.context);
                if (!id)
                {
                    context.Fail(asset.context, "unknown action context");
                }
                Route route{.id = *id, .sourceRadius = parameters.sourceRadius};
                if (authored)
                {
                    const auto source = authored->nodes.find(parameters.source), target = authored->nodes.find(parameters.target);
                    if (source == authored->nodes.end() || target == authored->nodes.end())
                    {
                        context.Fail(asset.context, "move source/target is not in the route graph");
                    }
                    route.start = source->second;
                    route.goal = target->second;
                }
                else
                {
                    route.graphIndex = graphs_.size();
                    route.start = GameplayRouteNodeId{1u};
                    route.goal = GameplayRouteNodeId{2u};
                    GameplayRouteGraph graph;
                    graph.nodes = {{route.start, context.Role(parameters.source).position},
                        {route.goal, context.Role(parameters.target).position}};
                    graph.edges = {{route.start, route.goal, 1.0f, {}}};
                    graphs_.push_back(std::move(graph));
                    if (parameters.routeCost)
                    {
                        context.Fail(asset.context, "routeCost requires an authored route graph");
                    }
                }
                const auto planned = FindWeightedGameplayRoute(graphs_[route.graphIndex], route.start, route.goal);
                if (!planned.Succeeded() || !planned.totalCost || !std::isfinite(*planned.totalCost))
                {
                    context.Fail(asset.context, "no valid route between action endpoints");
                }
```
