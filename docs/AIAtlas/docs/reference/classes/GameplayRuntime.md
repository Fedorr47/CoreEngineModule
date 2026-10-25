# GameplayRuntime

**class** · домен «Runtime-фасад» · `src/Gameplay/Runtime/GameplayRuntime.cppm:88–283` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

> Фасад gameplay: владеет AISystem, решениями, реестрами traversal, бронями; задаёт порядок кадра.

## Архитектурный разбор

**Владелец и время жизни.** Принадлежит App (runtimeState.gameplayRuntime); создаётся с каталогом решений.

**Runtime-методы, входы и выходы.** BeginFrame, PrePhysicsUpdate, PostPhysicsUpdate, StartAIDecision, CancelAIDecision, StartAI*, BuildAIDebugAgentViews, SetObstacleQuery.

**Ссылки.** [теория: 01-architecture](../../chapters/01-architecture.md) · [теория: 10-frame-order](../../chapters/10-frame-order.md) · [схема d10-frame-sequence](../diagrams.md#d10-frame-sequence) · [схема d09-ownership](../diagrams.md#d09-ownership) · [GR-PREPHYS](../source-references.md#gr-prephys) · [GR-START-DECISION](../source-references.md#gr-start-decision) · [GR-CTOR](../source-references.md#gr-ctor) · [GR-MEMBERS](../source-references.md#gr-members) · тест `TestGameplayRuntimeModeTransitions.cpp`

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 91 | конструктор | public | `GameplayRuntime() : GameplayRuntime(GameplayAIDecisionFactoryRegistry{}) {}` |
| 93 | конструктор | public | `explicit GameplayRuntime(GameplayAIDecisionFactoryRegistry decisionFactories)` |
| 94 | метод | public | `: aiDecisionFactoryRegistry_(std::move(decisionFactories))` |
| 104 | метод | public | `void Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene);` |
| 105 | метод | public | `void Shutdown();` |
| 107 | метод | public | `void BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback);` |
| 108 | метод | public | `void BindKeyboardMouseIntentSource(` |
| 111 | метод | public | `void UnbindIntentSource(const EntityHandle entity);` |
| 112 | метод | public | `[[nodiscard]] bool ApplyKeyboardMouseBindings(const GameplayKeyboardMouseBindings& bindings);` |
| 113 | метод | public | `[[nodiscard]] const GameplayKeyboardMouseBindings& GetKeyboardMouseBindings() const noexcept;` |
| 114 | метод | public | `[[nodiscard]] const GameplayActionDefinitions& GetGameplayActionDefinitions() const noexcept;` |
| 115 | метод | public | `[[nodiscard]] const GameplayActionAnimationBindings& GetGameplayActionAnimationBindings() const noexcept;` |
| 117 | метод | public | `void SetObstacleQuery(const IGameplayObstacleQuery* query) noexcept;` |
| 118 | метод | public | `[[nodiscard]] bool ApplyGameplayActionConfiguration(` |
| 123 | метод | public | `void BeginFrame();` |
| 124 | метод | public | `void PrePhysicsUpdate(const GameplayUpdateContext& ctx);` |
| 125 | метод | public | `void PostPhysicsUpdate(const GameplayUpdateContext& ctx);` |
| 126 | метод | public | `void PostAnimationUpdate(const GameplayUpdateContext& ctx);` |
| 127 | метод | public | `void SetSkipDuplicatePostAnimationSyncEnabled( bool enabled) noexcept;` |
| 129 | метод | public | `[[nodiscard]] bool IsSkipDuplicatePostAnimationSyncEnabled() const noexcept;` |
| 130 | метод | public | `[[nodiscard]] GameplayWorld& GetWorld() noexcept;` |
| 131 | метод | public | `[[nodiscard]] const GameplayWorld& GetWorld() const noexcept;` |
| 132 | метод | public | `[[nodiscard]] EntityHandle GetControlledEntity() const noexcept;` |
| 133 | метод | public | `[[nodiscard]] GameplayRuntimeMode GetCurrentMode() const noexcept;` |
| 134 | метод | public | `[[nodiscard]] bool IsCurrentLevelAsset(const LevelAsset& levelAsset) const noexcept;` |
| 135 | метод | public | `[[nodiscard]] bool IsCurrentLevelContext(const GameplayUpdateContext& ctx) const noexcept;` |
| 136 | метод | public | `[[nodiscard]] const std::vector<EntityHandle>& GetNodeBoundEntities() const noexcept;` |
| 137 | метод | public | `[[nodiscard]] const GameplaySteeringDebugRegistry& GetSteeringDebugRegistry() const noexcept` |
| 139 | метод | public | `void SetSteeringDebugEnabled(const bool enabled) noexcept` |
| 141 | метод | public | `[[nodiscard]] const GameplayNavigationDebugRegistry& GetNavigationDebugRegistry() const noexcept` |
| 143 | метод | public | `[[nodiscard]] std::span<const GameplayWorldEvent> GetCurrentWorldEvents() const noexcept;` |
| 144 | метод | public | `void ClearCurrentWorldEvents() noexcept;` |
| 146 | метод | public | `[[nodiscard]] EntityHandle SpawnNodeBoundEntity(` |
| 150 | метод | public | `[[nodiscard]] bool DestroyNodeBoundEntity(EntityHandle entity);` |
| 151 | метод | public | `[[nodiscard]] AIActionExecutionStatus StartAIFollowRoute(` |
| 154 | поле | public | `const GameplayArrivalSteeringSettings& steeringSettings = {});` |
| 155 | метод | public | `[[nodiscard]] AIActionExecutionStatus StartAIMoveTo(` |
| 160 | поле | public | `const GameplayArrivalSteeringSettings& steeringSettings = {});` |
| 161 | метод | public | `[[nodiscard]] AIActionExecutionStatus StartAIFollowTarget(` |
| 163 | поле | public | `const AIFollowTargetSettings& settings = {});` |
| 164 | метод | public | `[[nodiscard]] AIActionExecutionStatus StartAIFleeTarget(` |
| 166 | поле | public | `const AIFleeTargetSettings& settings = {});` |
| 167 | метод | public | `[[nodiscard]] std::unique_ptr<AIMoveToActionBinding> CreateAIMoveToActionBinding(` |
| 169 | метод | public | `[[nodiscard]] AIPlanExecutionStatus UpdateAIDecision(` |
| 175 | метод | public | `void CancelAIDecision(AIDecisionRuntime& decision) noexcept;` |
| 176 | метод | public | `[[nodiscard]] bool StartAIDecision(EntityHandle agentEntity, std::string_view definitionId,` |
| 177 | поле | public | `std::string* diagnostic = nullptr);` |
| 178 | метод | public | `void CancelAIDecision(EntityHandle agentEntity) noexcept;` |
| 179 | метод | public | `[[nodiscard]] AIPlanExecutionStatus GetAIDecisionStatus(EntityHandle agentEntity) const noexcept;` |
| 180 | метод | public | `[[nodiscard]] const AIAgentWorldState* GetAIDecisionObservedState(EntityHandle agentEntity) const noexcept;` |
| 181 | метод | public | `[[nodiscard]] std::vector<GameplayAIDebugAgentView> BuildAIDebugAgentViews() const;` |
| 183 | метод | public | `BuildAIPlannedPathDebugAgentViews() const;` |
| 184 | метод | public | `[[nodiscard]] bool HasAIDecisionDefinition(std::string_view definitionId) const noexcept;` |
| 185 | метод | public | `[[nodiscard]] bool RegisterGameplayTraversalLink(GameplayTraversalLink link);` |
| 186 | метод | public | `[[nodiscard]] bool RemoveGameplayTraversalLink(GameplayTraversalLinkHandle handle) noexcept;` |
| 187 | метод | public | `[[nodiscard]] std::optional<GameplayTraversalLink> FindGameplayTraversalLink(` |
| 189 | метод | public | `[[nodiscard]] bool RegisterGameplayTraversalExecutor(` |
| 192 | метод | public | `[[nodiscard]] bool RemoveGameplayTraversalExecutor(` |
| 194 | метод | public | `[[nodiscard]] bool HasGameplayTraversalExecutor(` |
| 196 | метод | public | `void CancelAIAction(EntityHandle agentEntity);` |
| 197 | метод | public | `void ClearAIAction(EntityHandle agentEntity);` |
| 199 | метод | public | `void ResetNodeBoundEntitySimulationState(EntityHandle entity);` |
| 200 | метод | public | `[[nodiscard]] AIActionExecutionStatus GetAIActionStatus(EntityHandle agentEntity) const noexcept;` |
| 201 | метод | public | `[[nodiscard]] bool TryReserveGameplayObject(EntityHandle objectEntity, EntityHandle agentEntity);` |
| 202 | метод | public | `[[nodiscard]] bool ReleaseGameplayObject(EntityHandle objectEntity, EntityHandle agentEntity) noexcept;` |
| 203 | метод | public | `[[nodiscard]] bool IsGameplayObjectReserved(EntityHandle objectEntity) const noexcept;` |
| 204 | метод | public | `[[nodiscard]] bool IsGameplayObjectReservedBy(EntityHandle objectEntity, EntityHandle agentEntity) const noexcept;` |
| 205 | метод | public | `[[nodiscard]] EntityHandle GetGameplayObjectReservationOwner(EntityHandle objectEntity) const noexcept;` |
| 208 | метод | private | `void ResetEntityFrameState_(const EntityHandle entity);` |
| 209 | метод | private | `void UpdateActiveAIDecisions_();` |
| 210 | метод | private | `void CancelAllAIDecisions_() noexcept;` |
| 211 | метод | private | `void UpdateFollowCamera_(const GameplayUpdateContext& ctx, const bool consumeInput);` |
| 212 | метод | private | `void CompactTrackedState_();` |
| 213 | метод | private | `void RebuildIntentBindingIndex_();` |
| 214 | метод | private | `bool ValidateIntentBindingIndex_() const;` |
| 215 | метод | private | `void EraseIntentBindingAtStableIndex_(std::size_t index);` |
| 216 | метод | private | `void UpsertIntentBinding_(EntityHandle entity, GameplayIntentSourceCallback callback);` |
| 217 | метод | private | `void CreateDefaultGraphInstance_(EntityHandle entity);` |
| 218 | метод | private | `void SyncActionStateToGraphParameters_(EntityHandle entity, GameplayGraphInstance& graph);` |
| 219 | метод | private | `void WriteActionStateToGraphParameters_(GameplayGraphInstance& graph, const GameplayActionComponent* action);` |
| 220 | метод | private | `void ExecuteGameplayGraphs_(const GameplayUpdateContext& ctx);` |
| 221 | метод | private | `void ExecuteGraphLayer_(EntityHandle entity, GameplayGraphInstance& graph,` |
| 225 | метод | private | `void ExecuteGraphTasks_(EntityHandle entity, GameplayGraphInstance& graph,` |
| 227 | метод | private | `void BeginActionState_(EntityHandle entity, GameplayGraphInstance& graph);` |
| 228 | метод | private | `void ResetSimulationState_();` |
| 229 | метод | private | `void ResetEntitySimulationState_(EntityHandle entity);` |
| 230 | метод | private | `void RemoveDeadNodeBoundEntities_(const GameplayUpdateContext& ctx);` |
| 231 | метод | private | `void HandleRuntimeModeChanged_(const GameplayUpdateContext& ctx);` |
| 233 | метод | private | `void EnsureBootstrapEntity_(const GameplayUpdateContext& ctx);` |
| 234 | метод | private | `void RecordSyncInstrumentationSample_(` |
| 238 | метод | private | `void LogSyncInstrumentationSample_() const;` |
| 241 | поле | private | `GameplayWorld world_{};` |
| 242 | поле | private | `const IGameplayObstacleQuery* obstacleQuery_{nullptr};` |
| 243 | поле | private | `GameplaySteeringDebugRegistry steeringDebugRegistry_{};` |
| 244 | поле | private | `GameplayNavigationDebugRegistry navigationDebugRegistry_{};` |
| 245 | поле | private | `EntityHandle controlledEntity_{ kNullEntity };` |
| 246 | поле | private | `GameplayKeyboardMouseBindings keyboardMouseBindings_{};` |
| 247 | поле | private | `GameplayActionDefinitions actionDefinitions_{ MakeDefaultGameplayActionDefinitions() };` |
| 248 | поле | private | `GameplayActionAnimationBindings actionAnimationBindings_{ MakeDefaultGameplayActionAnimationBindings() };` |
| 249 | поле | private | `LevelAsset* currentLevelAsset_{ nullptr };` |
| 250 | поле | private | `LevelInstance* currentLevelInstance_{ nullptr };` |
| 251 | поле | private | `Scene* currentScene_{ nullptr };` |
| 252 | поле | private | `std::vector<GameplayIntentBinding> intentBindings_{};` |
| 253 | поле | private | `std::unordered_map<EntityHandle, std::size_t> intentBindingIndexByEntity_{};` |
| 258 | поле | private | `std::vector<EntityHandle> nodeBoundEntities_{};` |
| 259 | поле | private | `std::unordered_map<EntityHandle, GameplayGraphInstance> graphInstances_{};` |
| 260 | поле | private | `GameplayGraphAsset defaultGraphAsset_{};` |
| 261 | поле | private | `GameplayRuntimeMode lastMode_{ GameplayRuntimeMode::Editor };` |
| 262 | поле | private | `AISystem aiSystem_{};` |
| 263 | поле | private | `GameplayAIDecisionFactoryRegistry aiDecisionFactoryRegistry_{};` |
| 264 | поле | private | `std::unordered_map<EntityHandle, std::unique_ptr<GameplayAIDecisionInstance>> activeAIDecisions_{};` |
| 265 | поле | private | `GameplayTraversalLinkRegistry traversalLinkRegistry_{};` |
| 266 | поле | private | `GameplayTraversalExecutorRegistry traversalExecutorRegistry_{};` |
| 267 | поле | private | `GameplayUnsupportedTraversalExecutor unsupportedTraversalExecutor_{};` |
| 268 | поле | private | `GameplayObjectReservationSystem objectReservationSystem_{};` |
| 269 | поле | private | `GameplayPickupSystem pickupSystem_{};` |
| 270 | поле | private | `DoorTraversalExecutor doorTraversalExecutor_{world_, objectReservationSystem_};` |
| 271 | поле | private | `JumpTraversalExecutor jumpTraversalExecutor_{world_, traversalLinkRegistry_};` |
| 272 | поле | private | `std::vector<GameplayAnimationNotifyRecord> recentNotifyEvents_{};` |
| 273 | поле | private | `std::vector<GameplayEventRecord> recentGameplayEvents_{};` |
| 274 | поле | private | `std::vector<GameplayWorldEvent> currentWorldEvents_{};` |
| 275 | поле | private | `GameplayFollowCameraController followCameraController_{};` |
| 277 | поле | private | `ProfileUtils::SyncInstrumentationAggregate preSyncInstAggregate_{};` |
| 278 | поле | private | `ProfileUtils::SyncInstrumentationAggregate postSyncInstAggregate_{};` |
| 279 | поле | private | `std::uint64_t postSyncSkippedFrameCount_{ 0 };` |
| 280 | поле | private | `std::uint64_t postSyncExecutedFrameCount_{ 0 };` |
| 281 | поле | private | `bool skipDuplicatePostAnimationSyncEnabled_{ false };` |

## Начало объявления (первые 61 строка)

Включено из исходника при сборке; `src/Gameplay/Runtime/GameplayRuntime.cppm:88–148`.

```cpp linenums="88"
    class GameplayRuntime
    {
    public:
        GameplayRuntime() : GameplayRuntime(GameplayAIDecisionFactoryRegistry{}) {}

        explicit GameplayRuntime(GameplayAIDecisionFactoryRegistry decisionFactories)
            : aiDecisionFactoryRegistry_(std::move(decisionFactories))
        {
            const bool bRegisteredDoor = traversalExecutorRegistry_.RegisterRuntimeOwned(
                DoorTraversalExecutor::kTypeId, doorTraversalExecutor_);
            const bool bRegisteredJump = traversalExecutorRegistry_.RegisterRuntimeOwned(
                JumpTraversalExecutor::kTypeId, jumpTraversalExecutor_);
            assert(bRegisteredDoor && bRegisteredJump &&
                "Built-in traversal executors must register exactly once.");
        }

        void Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene);
        void Shutdown();

        void BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback);
        void BindKeyboardMouseIntentSource(
            const EntityHandle entity,
            const GameplayKeyboardMouseBindings& bindings);
        void UnbindIntentSource(const EntityHandle entity);
        [[nodiscard]] bool ApplyKeyboardMouseBindings(const GameplayKeyboardMouseBindings& bindings);
        [[nodiscard]] const GameplayKeyboardMouseBindings& GetKeyboardMouseBindings() const noexcept;
        [[nodiscard]] const GameplayActionDefinitions& GetGameplayActionDefinitions() const noexcept;
        [[nodiscard]] const GameplayActionAnimationBindings& GetGameplayActionAnimationBindings() const noexcept;
        // Non-owning; the integration owner must outlive this runtime and all active AI actions.
        void SetObstacleQuery(const IGameplayObstacleQuery* query) noexcept;
        [[nodiscard]] bool ApplyGameplayActionConfiguration(
            const GameplayActionDefinitions& definitions,
            const GameplayActionAnimationBindings& animationBindings,
            std::string& diagnostic);

        void BeginFrame();
        void PrePhysicsUpdate(const GameplayUpdateContext& ctx);
        void PostPhysicsUpdate(const GameplayUpdateContext& ctx);
        void PostAnimationUpdate(const GameplayUpdateContext& ctx);
        void SetSkipDuplicatePostAnimationSyncEnabled( bool enabled) noexcept;

        [[nodiscard]] bool IsSkipDuplicatePostAnimationSyncEnabled() const noexcept;
        [[nodiscard]] GameplayWorld& GetWorld() noexcept;
        [[nodiscard]] const GameplayWorld& GetWorld() const noexcept;
        [[nodiscard]] EntityHandle GetControlledEntity() const noexcept;
        [[nodiscard]] GameplayRuntimeMode GetCurrentMode() const noexcept;
        [[nodiscard]] bool IsCurrentLevelAsset(const LevelAsset& levelAsset) const noexcept;
        [[nodiscard]] bool IsCurrentLevelContext(const GameplayUpdateContext& ctx) const noexcept;
        [[nodiscard]] const std::vector<EntityHandle>& GetNodeBoundEntities() const noexcept;
        [[nodiscard]] const GameplaySteeringDebugRegistry& GetSteeringDebugRegistry() const noexcept
        { return steeringDebugRegistry_; }
        void SetSteeringDebugEnabled(const bool enabled) noexcept
        { steeringDebugRegistry_.SetEnabled(enabled); }
        [[nodiscard]] const GameplayNavigationDebugRegistry& GetNavigationDebugRegistry() const noexcept
        { return navigationDebugRegistry_; }
        [[nodiscard]] std::span<const GameplayWorldEvent> GetCurrentWorldEvents() const noexcept;
        void ClearCurrentWorldEvents() noexcept;

        [[nodiscard]] EntityHandle SpawnNodeBoundEntity(
            const GameplayUpdateContext& ctx,
            const int nodeIndex,
```
