# AIMoveToActionBinding

**class** · домен «Навигационные действия» · `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:138–218` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Реальные базы / определение: `public IAIActionBinding`

> Binding MoveTo: запрос у провайдера → обычный или бронирующий runtime.

## Архитектурный разбор

**Runtime-методы, входы и выходы.** CreateRuntime(context): actionId 2, запрос; при цели брони — ReservedAIMoveToActionRuntime.

**Ссылки.** [теория: 09-execution-lifetime](../../chapters/09-execution-lifetime.md) · [NA-MOVETO-BINDING](../source-references.md#na-moveto-binding) · тест `TestAIMoveToActionBinding.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 141 | конструктор | public | `AIMoveToActionBinding(` |
| 149 | метод | public | `: world_(world)` |
| 150 | метод | public | `, traversalLinkRegistry_(traversalLinkRegistry)` |
| 151 | метод | public | `, traversalExecutorRegistry_(traversalExecutorRegistry)` |
| 152 | метод | public | `, requestProvider_(requestProvider)` |
| 153 | метод | public | `, obstacleQuery_(obstacleQuery)` |
| 154 | метод | public | `, debugRegistry_(debugRegistry)` |
| 155 | метод | public | `, navigationDebugRegistry_(navigationDebugRegistry)` |
| 159 | конструктор | public | `AIMoveToActionBinding(GameplayWorld& world,` |
| 168 | метод | public | `: AIMoveToActionBinding(world, traversalLinkRegistry, traversalExecutorRegistry,` |
| 176 | метод | public | `[[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(` |
| 209 | поле | private | `GameplayWorld& world_;` |
| 210 | поле | private | `const GameplayTraversalLinkRegistry& traversalLinkRegistry_;` |
| 211 | поле | private | `const GameplayTraversalExecutorRegistry& traversalExecutorRegistry_;` |
| 212 | поле | private | `IAIMoveToActionRequestProvider& requestProvider_;` |
| 213 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 214 | поле | private | `GameplaySteeringDebugRegistry* debugRegistry_{nullptr};` |
| 215 | поле | private | `GameplayNavigationDebugRegistry* navigationDebugRegistry_{nullptr};` |
| 216 | поле | private | `GameplayObjectReservationSystem* reservationSystem_{};` |
| 217 | поле | private | `IAIActionReservationTargetProvider* reservationTargetProvider_{};` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/AI/Navigation/AIMoveToActionBinding.cppm:138–198`.

```cpp linenums="138"
    class AIMoveToActionBinding final : public IAIActionBinding
    {
    public:
        AIMoveToActionBinding(
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            IAIMoveToActionRequestProvider& requestProvider,
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
             GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr) noexcept
            : world_(world)
            , traversalLinkRegistry_(traversalLinkRegistry)
            , traversalExecutorRegistry_(traversalExecutorRegistry)
            , requestProvider_(requestProvider)
            , obstacleQuery_(obstacleQuery)
            , debugRegistry_(debugRegistry)
            , navigationDebugRegistry_(navigationDebugRegistry)
        {
        }

        AIMoveToActionBinding(GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
            const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
            IAIMoveToActionRequestProvider& requestProvider,
            GameplayObjectReservationSystem& reservationSystem,
            IAIActionReservationTargetProvider& reservationTargetProvider,
            const IGameplayObstacleQuery* obstacleQuery = nullptr,
            GameplaySteeringDebugRegistry* debugRegistry = nullptr,
            GameplayNavigationDebugRegistry* navigationDebugRegistry = nullptr) noexcept
            : AIMoveToActionBinding(world, traversalLinkRegistry, traversalExecutorRegistry,
            requestProvider, obstacleQuery, debugRegistry, navigationDebugRegistry)
        {
            reservationSystem_ = &reservationSystem;
            reservationTargetProvider_ = &reservationTargetProvider;
        }


        [[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(
            const AIActionRuntimeContext& context) override
        {
            if (context.actionId != kAIMoveToActionId)
            {
                return nullptr;
            }
            const std::optional<AIMoveToActionRequest> request =
                requestProvider_.ResolveRequest(context);
            if (!request)
            {
                return nullptr;
            }
            if (reservationSystem_ != nullptr && reservationTargetProvider_ != nullptr)
            {
                const EntityHandle target = reservationTargetProvider_->ResolveReservationTarget(context);
                if (target != kNullEntity)
                {
                    return std::make_unique<ReservedAIMoveToActionRuntime>(world_,
                        traversalLinkRegistry_, traversalExecutorRegistry_, *reservationSystem_,
                        *request, target, obstacleQuery_, debugRegistry_, navigationDebugRegistry_);
                }
            }
```
