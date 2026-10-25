# AIFollowRouteActionRuntime

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:27–360` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionRuntime`

> Исполнение маршрута: follower, avoidance (если есть query), команда и facing, traversal handoff, physicallyBlocked.

## Архитектурный разбор

**Владелец и время жизни.** Владеет копией маршрута и follower; заимствует мир, реестры traversal, query, debug-реестры.

**Runtime-методы, входы и выходы.** Start (публикация маршрута), Tick, Cancel, IsPhysicallyBlocked.

**Кто вызывает и в какой фазе.** AISystem::Update через AIActionTask.

**Ошибки, terminal states, отмена.** Невалидный агент/маршрут, неуспех executor → Failed с остановкой (вертикаль сохраняется).

**Ссылки.** [теория: 12-route-follower](../../chapters/12-route-follower.md) · [теория: 16-traversal-physics](../../chapters/16-traversal-physics.md) · [схема d16-traversal-handoff](../diagrams.md#d16-traversal-handoff) · [NA-ROUTE-RT](../source-references.md#na-route-rt) · [NA-ROUTE-TICK](../source-references.md#na-route-tick) · [NA-ROUTE-TRAVERSAL](../source-references.md#na-route-traversal) · [NA-ROUTE-APPLY](../source-references.md#na-route-apply) · тест `TestAIFollowRouteActionRuntime.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 30 | конструктор | public | `AIFollowRouteActionRuntime(` |
| 41 | метод | public | `: world_(world)` |
| 42 | метод | public | `, traversalLinkRegistry_(traversalLinkRegistry)` |
| 43 | метод | public | `, traversalExecutorRegistry_(traversalExecutorRegistry)` |
| 44 | метод | public | `, route_(std::move(route))` |
| 45 | метод | public | `, steeringSettings_(steeringSettings)` |
| 46 | метод | public | `, followerSettings_(followerSettings)` |
| 47 | метод | public | `, obstacleQuery_(obstacleQuery)` |
| 48 | метод | public | `, obstacleSettings_(obstacleSettings)` |
| 49 | метод | public | `, debugRegistry_(debugRegistry)` |
| 50 | метод | public | `, navigationDebugRegistry_(navigationDebugRegistry)` |
| 54 | метод | public | `[[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override` |
| 83 | метод | public | `[[nodiscard]] AIActionRuntimeResult Tick(` |
| 126 | метод | public | `[[nodiscard]] bool IsPhysicallyBlocked() const noexcept` |
| 131 | метод | public | `void Cancel(const AIActionRuntimeContext& context) noexcept override` |
| 156 | метод | private | `[[nodiscard]] AIActionRuntimeResult AdvanceFollower_(const EntityHandle entity)` |
| 185 | метод | private | `void ApplyFollowerMovement_(` |
| 223 | метод | private | `[[nodiscard]] AIActionRuntimeResult StartTraversal_(` |
| 277 | метод | private | `void CancelActiveTraversal_() noexcept` |
| 286 | метод | private | `void ClearNavigationDebug_(const EntityHandle entity) noexcept` |
| 294 | метод | private | `[[nodiscard]] bool ValidateContextAndAgent_(const AIActionRuntimeContext& context) const noexcept` |
| 308 | метод | private | `void ClearMovementIntentIfAccessible_(const EntityHandle entity) const noexcept` |
| 330 | метод | private | `void StopMovementIfAccessible_(const EntityHandle entity) noexcept` |
| 346 | поле | private | `GameplayWorld& world_;` |
| 347 | поле | private | `const GameplayTraversalLinkRegistry& traversalLinkRegistry_;` |
| 348 | поле | private | `const GameplayTraversalExecutorRegistry& traversalExecutorRegistry_;` |
| 349 | поле | private | `GameplayRoute route_{};` |
| 350 | поле | private | `GameplayArrivalSteeringSettings steeringSettings_{};` |
| 351 | поле | private | `GameplayRouteFollowerSettings followerSettings_{};` |
| 352 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 353 | поле | private | `GameplayObstacleAvoidanceSettings obstacleSettings_{};` |
| 354 | поле | private | `GameplayObstacleAvoidanceState obstacleAvoidanceState_{};` |
| 355 | поле | private | `GameplayNavigationDebugRegistry* navigationDebugRegistry_{nullptr};` |
| 356 | поле | private | `GameplaySteeringDebugRegistry* debugRegistry_{nullptr};` |
| 357 | поле | private | `bool physicallyBlocked_{ false };` |
| 358 | поле | private | `GameplayRouteFollower follower_{};` |
| 359 | поле | private | `std::optional<ActiveTraversalState> activeTraversal_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIFollowRouteActionRuntime.cppm:27–87`.

```cpp linenums="27"
    class AIFollowRouteActionRuntime final : public IAIActionRuntime
    {
    public:
        AIFollowRouteActionRuntime(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            GameplayRoute route,
            GameplayArrivalSteeringSettings steeringSettings = {},
             const IGameplayObstacleQuery* obstacleQuery = nullptr,
             GameplayObstacleAvoidanceSettings obstacleSettings = {},
             GameplaySteeringDebugRegistry* debugRegistry = nullptr,
             GameplayRouteFollowerSettings followerSettings = {},
             GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr) noexcept
        : world_(world)
        , traversalLinkRegistry_(traversalLinkRegistry)
        , traversalExecutorRegistry_(traversalExecutorRegistry)
        , route_(std::move(route))
        , steeringSettings_(steeringSettings)
        , followerSettings_(followerSettings)
        , obstacleQuery_(obstacleQuery)
        , obstacleSettings_(obstacleSettings)
        , debugRegistry_(debugRegistry)
        , navigationDebugRegistry_(navigationDebugRegistry)
        {
        }

        [[nodiscard]] AIActionRuntimeResult Start(const AIActionRuntimeContext& context) override
        {
            physicallyBlocked_ = false;
            obstacleAvoidanceState_ = {};
            if (!ValidateContextAndAgent_(context) || !route_.IsValid())
            {
                ClearNavigationDebug_(context.agentEntity);
                StopMovementIfAccessible_(context.agentEntity);
                return AIActionRuntimeResult::Failed;
            }

            if (navigationDebugRegistry_ != nullptr)
            {
                navigationDebugRegistry_->Publish(context.agentEntity, route_);
            }

            const GameplayRouteFollowerStatus followerStatus = follower_.Start(
                std::move(route_), steeringSettings_, followerSettings_);
            if (followerStatus == GameplayRouteFollowerStatus::Following)
            {
                return AIActionRuntimeResult::Running;
            }
            StopMovementIfAccessible_(context.agentEntity);
            ClearNavigationDebug_(context.agentEntity);
            return followerStatus == GameplayRouteFollowerStatus::Succeeded
                ? AIActionRuntimeResult::Succeeded
                : AIActionRuntimeResult::Failed;
        }

        [[nodiscard]] AIActionRuntimeResult Tick(
            const AIActionRuntimeContext& context,
            const float deltaSeconds) override
        {
            if (!ValidateContextAndAgent_(context))
```
