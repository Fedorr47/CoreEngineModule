        void GameplayRuntime::Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            defaultGraphAsset_ = MakeDefaultHumanoidGameplayGraphAsset();
            currentLevelAsset_ = &levelAsset;
            currentLevelInstance_ = &levelInstance;
            currentScene_ = &scene;

            GameplayUpdateContext ctx{};
            ctx.mode = GameplayRuntimeMode::Editor;
            ctx.levelAsset = &levelAsset;
            ctx.levelInstance = &levelInstance;
            ctx.scene = &scene;
            EnsureBootstrapEntity_(ctx);
            lastMode_ = GameplayRuntimeMode::Editor;
        }

        void GameplayRuntime::Shutdown()
        {
            CORE_ASSERT_RUNTIME_THREAD();

            LogSyncInstrumentationSample_();
            aiSystem_.Reset();
            traversalLinkRegistry_.Reset();
            traversalExecutorRegistry_.Reset();
            objectReservationSystem_.Reset();
            
            world_.Clear();
            currentLevelAsset_ = nullptr;
            currentLevelInstance_ = nullptr;
            currentScene_ = nullptr;
            recentNotifyEvents_.clear();
            recentGameplayEvents_.clear();
            intentBindings_.clear();
            intentBindingIndexByEntity_.clear();
            nodeBoundEntities_.clear();
            graphInstances_.clear();
            controlledEntity_ = kNullEntity;
            lastMode_ = GameplayRuntimeMode::Editor;
            
            preSyncInstAggregate_ = {};
            postSyncInstAggregate_ = {};
            postSyncExecutedFrameCount_ = 0;
            postSyncSkippedFrameCount_ = 0;
        }

        // TODO: maybe I should decomposite it to 
        // Input config asset
        // InputSource objects
        // Gamepad support
        // AI/replay/network input source
        void GameplayRuntime::BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            recentNotifyEvents_.clear();
            recentGameplayEvents_.clear();

            if (!world_.IsEntityValid(entity) || !callback)
            {
                return;
            }

            UpsertIntentBinding_(entity, std::move(callback));
            //assert(ValidateIntentBindingIndex_());
        }

        void GameplayRuntime::BindKeyboardMouseIntentSource(const EntityHandle entity, const GameplayKeyboardMouseBindings& bindings = {})
        {
            CORE_ASSERT_RUNTIME_THREAD();

            BindIntentSource(entity,
                [bindings]([[maybe_unused]] const EntityHandle entity,
                    const GameplayUpdateContext& ctx,
                    [[maybe_unused]] GameplayWorld& world,
                    GameplayInputIntentComponent& outIntent,
                    [[maybe_unused]] GameplayActionComponent* action)
                {
                    if (ctx.mode != GameplayRuntimeMode::Game || ctx.input == nullptr)
                    {
                        return;
                    }

                    ReadKeyboardMouseGameplayIntent(*ctx.input, bindings, outIntent);
                });
        }

        void GameplayRuntime::UnbindIntentSource(const EntityHandle entity)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            const auto it = intentBindingIndexByEntity_.find(entity);
            if (it == intentBindingIndexByEntity_.end())
            {
                return;
            }
            
            EraseIntentBindingAtStableIndex_(it->second);
            //assert(ValidateIntentBindingIndex_());
        }

        void GameplayRuntime::BeginFrame()
        {
            CORE_ASSERT_RUNTIME_THREAD();

            recentNotifyEvents_.clear();
            recentGameplayEvents_.clear();
            CompactTrackedState_();

            for (const EntityHandle entity : nodeBoundEntities_)
            {
                ResetEntityFrameState_(entity);
            }
        }

        void GameplayRuntime::PreAnimationUpdate(const GameplayUpdateContext& ctx)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            EnsureBootstrapEntity_(ctx);

            if (ctx.mode != lastMode_)
            {
                HandleRuntimeModeChanged_(ctx);
                lastMode_ = ctx.mode;
            }

            if (ctx.mode != GameplayRuntimeMode::Game)
            {
                return;
            }

            UpdateFollowCamera_(ctx, true);

            UpdateGameplayIntentSources(world_, intentBindings_, ctx);
            BuildGameplayCharacterCommands(world_, nodeBoundEntities_, ctx);
            objectReservationSystem_.CleanupInvalidReservations(world_);
            aiSystem_.Update(world_, ctx.deltaSeconds);
            UpdateGameplayCombatRequests(world_, nodeBoundEntities_);
            UpdateGameplayInteractionRequests(world_, nodeBoundEntities_);
            ExecuteGameplayGraphs_(ctx);
            UpdateGameplayCharacterMovement(world_, nodeBoundEntities_, ctx.deltaSeconds);
            UpdateGameplayCharacterLocomotion(world_, nodeBoundEntities_);
            SyncGameplayTransformsToRuntime(world_, nodeBoundEntities_, ctx);
            UpdateFollowCamera_(ctx, false);
            PushGameplayStateToAnimation(world_, nodeBoundEntities_, ctx);
            
            const auto syncStartedAt = std::chrono::steady_clock::now();
            std::size_t processedEntityCount = 0;
            SyncGameplayAnimationStateFromRuntime(
                world_,
                nodeBoundEntities_,
                ctx,
                &graphInstances_,
                &processedEntityCount);
            
            const auto syncEndedAt = std::chrono::steady_clock::now();
            RecordSyncInstrumentationSample_(
                preSyncInstAggregate_,
                std::chrono::duration_cast<std::chrono::nanoseconds>(syncEndedAt - syncStartedAt),
                processedEntityCount);
        }

        void GameplayRuntime::PostAnimationUpdate(const GameplayUpdateContext& ctx)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            if (ctx.mode != GameplayRuntimeMode::Game)
            {
                return;
            }

            if (!skipDuplicatePostAnimationSyncEnabled_)
            {
                ++postSyncExecutedFrameCount_;
                const auto syncStartedAt = std::chrono::steady_clock::now();
                std::size_t processedEntityCount = 0;
                SyncGameplayAnimationStateFromRuntime(
                    world_,
                    nodeBoundEntities_,
                    ctx,
                    &graphInstances_,
                    &processedEntityCount);

                const auto syncEndedAt = std::chrono::steady_clock::now();
                RecordSyncInstrumentationSample_(
                    postSyncInstAggregate_,
                    std::chrono::duration_cast<std::chrono::nanoseconds>(syncEndedAt - syncStartedAt),
                    processedEntityCount);
            }
            else
            {
                ++postSyncSkippedFrameCount_;
            }

            ConsumeGameplayAnimationEvents(
                world_,
                nodeBoundEntities_,
                ctx,
                recentNotifyEvents_,
                recentGameplayEvents_);

            for (const GameplayEventRecord& eventRecord : recentGameplayEvents_)
            {
                auto it = graphInstances_.find(eventRecord.entity);
                if (it != graphInstances_.end())
                {
                    PushGameplayGraphEvent(it->second, eventRecord.gameplayEventId);
                }
            }
        }

        [[nodiscard]] GameplayWorld& GameplayRuntime::GetWorld() noexcept
        {
            return world_;
        }

        bool GameplayRuntime::TryReserveGameplayObject(
            const EntityHandle objectEntity,
            const EntityHandle agentEntity)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return objectReservationSystem_.TryReserve(world_, objectEntity, agentEntity);
        }
        
        bool GameplayRuntime::ReleaseGameplayObject(
            const EntityHandle objectEntity,
            const EntityHandle agentEntity) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return objectReservationSystem_.Release(objectEntity, agentEntity);
        }
        
        bool GameplayRuntime::IsGameplayObjectReserved(const EntityHandle objectEntity) const noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return objectReservationSystem_.IsReserved(objectEntity);
        }
        
        bool GameplayRuntime::IsGameplayObjectReservedBy(
            const EntityHandle objectEntity,
            const EntityHandle agentEntity) const noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return objectReservationSystem_.IsReservedBy(objectEntity, agentEntity);
        }
        
        EntityHandle GameplayRuntime::GetGameplayObjectReservationOwner(
            const EntityHandle objectEntity) const noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return objectReservationSystem_.GetReservationOwner(objectEntity);
        }

        [[nodiscard]] const GameplayWorld& GameplayRuntime::GetWorld() const noexcept
        {
            return world_;
        }

        [[nodiscard]] EntityHandle GameplayRuntime::GetControlledEntity() const noexcept
        {
            return controlledEntity_;
        }

        [[nodiscard]] GameplayRuntimeMode GameplayRuntime::GetCurrentMode() const noexcept
        {
            return lastMode_;
        }

        [[nodiscard]] bool GameplayRuntime::IsCurrentLevelAsset(const LevelAsset& levelAsset) const noexcept
        {
            return &levelAsset == currentLevelAsset_;
        }

        [[nodiscard]] bool GameplayRuntime::IsCurrentLevelContext(const GameplayUpdateContext& ctx) const noexcept
        {
            return ctx.levelAsset == currentLevelAsset_ &&
                ctx.levelInstance == currentLevelInstance_ &&
                ctx.scene == currentScene_;
        }

        [[nodiscard]] const std::vector<EntityHandle>& GameplayRuntime::GetNodeBoundEntities() const noexcept
        {
            return nodeBoundEntities_;
        }

        [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIFollowRoute(
            const EntityHandle agentEntity,
            GameplayRoute route,
            const GameplayArrivalSteeringSettings& steeringSettings)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return AIFollowRouteAction::Start(
                aiSystem_,
                world_,
                traversalLinkRegistry_,
                traversalExecutorRegistry_,
                agentEntity,
                std::move(route),
                steeringSettings);
        }

        [[nodiscard]] AIActionExecutionStatus GameplayRuntime::StartAIMoveTo(
            const EntityHandle agentEntity,
            const GameplayRouteGraph& routeGraph,
            const GameplayRouteNodeId startNodeId,
            const GameplayRouteNodeId goalNodeId,
            const GameplayArrivalSteeringSettings& steeringSettings)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return AIMoveToAction::Start(
                aiSystem_,
                world_,
                traversalLinkRegistry_,
                traversalExecutorRegistry_,
                agentEntity,
                routeGraph,
                startNodeId,
                goalNodeId,
                steeringSettings);
        }

        [[nodiscard]] bool GameplayRuntime::RegisterGameplayTraversalLink(GameplayTraversalLink link)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalLinkRegistry_.Register(link);
        }

        [[nodiscard]] bool GameplayRuntime::RemoveGameplayTraversalLink(
            const GameplayTraversalLinkHandle handle) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalLinkRegistry_.Remove(handle);
        }

        [[nodiscard]] std::optional<GameplayTraversalLink> GameplayRuntime::FindGameplayTraversalLink(
            const GameplayTraversalLinkHandle handle) const noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalLinkRegistry_.Find(handle);
        }

        [[nodiscard]] bool GameplayRuntime::RegisterGameplayTraversalExecutor(
            const GameplayTraversalTypeId typeId,
            IGameplayTraversalExecutor& executor)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalExecutorRegistry_.Register(typeId, executor);
        }

        [[nodiscard]] bool GameplayRuntime::RemoveGameplayTraversalExecutor(
        const GameplayTraversalTypeId typeId) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalExecutorRegistry_.Remove(typeId);
        }

        [[nodiscard]] bool GameplayRuntime::HasGameplayTraversalExecutor(
        const GameplayTraversalTypeId typeId) const noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return traversalExecutorRegistry_.Contains(typeId);
        }

        void GameplayRuntime::CancelAIAction(const EntityHandle agentEntity)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            aiSystem_.CancelAction(agentEntity);
        }

        [[nodiscard]] AIActionExecutionStatus GameplayRuntime::GetAIActionStatus(
            const EntityHandle agentEntity) const noexcept
        {
            return aiSystem_.GetActionStatus(agentEntity);
        }

        [[nodiscard]] EntityHandle GameplayRuntime::SpawnNodeBoundEntity(
            const GameplayUpdateContext& ctx,
            const int nodeIndex,
            const bool playerControlled)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            if (ctx.levelAsset == nullptr || ctx.levelInstance == nullptr || ctx.scene == nullptr)
            {
                return kNullEntity;
            }
            
            if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= ctx.levelAsset->nodes.size())
            {
                return kNullEntity;
            }
            
            if (!ctx.levelAsset->nodes[static_cast<std::size_t>(nodeIndex)].alive)
            {
                for (auto it = nodeBoundEntities_.begin(); it != nodeBoundEntities_.end(); )
                {
                    const EntityHandle entity = *it;
                    const GameplayNodeLinkComponent* link = world_.TryGetNodeLink(entity);
                    if (link != nullptr && link->nodeIndex == nodeIndex)
                    {
                        world_.DestroyEntity(entity);
                        graphInstances_.erase(entity);
                        UnbindIntentSource(entity);
                        it = nodeBoundEntities_.erase(it);
                        if (controlledEntity_ == entity)
                        {
                            controlledEntity_ = kNullEntity;
                        }
                        continue;
                    }
                    ++it;
                }
                return kNullEntity;
            }
            
            for (const EntityHandle existingEntity : nodeBoundEntities_)
            {
                if (!world_.IsEntityValid(existingEntity))
                {
                    continue;
                }
                const GameplayNodeLinkComponent* link = world_.TryGetNodeLink(existingEntity);
                if (link != nullptr && link->nodeIndex == nodeIndex)
                {
                    return existingEntity;
                }
            }

            EntityHandle entity = SpawnGameplayNodeBoundEntity(
                world_,
                nodeBoundEntities_,
                *ctx.levelAsset,
                *ctx.levelInstance,
                nodeIndex,
                playerControlled);
            if (entity == kNullEntity)
            {
                return kNullEntity;
            }

            CreateDefaultGraphInstance_(entity);
            if (playerControlled)
            {
                controlledEntity_ = entity;
                BindKeyboardMouseIntentSource(entity);
            }
            return entity;
        }

        void GameplayRuntime::SetSkipDuplicatePostAnimationSyncEnabled(bool enabled) noexcept
        {
            skipDuplicatePostAnimationSyncEnabled_ = enabled;        
        }
    
        bool GameplayRuntime::IsSkipDuplicatePostAnimationSyncEnabled() const noexcept
        {
            return skipDuplicatePostAnimationSyncEnabled_;
        }
    
        void GameplayRuntime::RecordSyncInstrumentationSample_(
            ProfileUtils::SyncInstrumentationAggregate& aggregate,
            const std::chrono::nanoseconds duration, 
            std::size_t processedEntityCount)
        {
            ++aggregate.callCount;
             aggregate.totalProcessedEntityCount += processedEntityCount;
             aggregate.totalDuration += duration;
             aggregate.maxDuration = std::max(aggregate.maxDuration, duration);
        }
    
        void GameplayRuntime::LogSyncInstrumentationSample_() const
        {
            const auto printSummary = [](std::string_view passName, const ProfileUtils::SyncInstrumentationAggregate& aggregate)
            {
                const double totalMs = std::chrono::duration<double, std::milli>(aggregate.totalDuration).count();
                const double maxMs = std::chrono::duration<double, std::milli>(aggregate.maxDuration).count();
                const double avgMs = aggregate.callCount > 0 ? totalMs / static_cast<double>(aggregate.callCount) : 0.0;
                
                const double avgProcessedEntities = aggregate.callCount > 0
                        ? static_cast<double>(aggregate.totalProcessedEntityCount) / static_cast<double>(aggregate.callCount)
                        : 0.0;
                
                std::cerr << "[GameplayRuntime][AnimationSync][" << passName << "] "
                              << "calls=" << aggregate.callCount
                              << ", totalMs=" << totalMs
                              << ", avgMs=" << avgMs
                              << ", maxMs=" << maxMs
                              << ", totalProcessedEntities=" << aggregate.totalProcessedEntityCount
                              << ", avgProcessedEntities=" << avgProcessedEntities
                              << '\n';
                
            };
            
            printSummary("PreSyncInst", preSyncInstAggregate_);
            printSummary("PostSyncInst", postSyncInstAggregate_);
            std::cerr << "[GameplayRuntime][AnimationSync][PostSyncGuard]"
                << "skipDuplicateEnabled=" << (skipDuplicatePostAnimationSyncEnabled_ ? "true" : "false")
                << ", executedFrameCount=" << postSyncExecutedFrameCount_
                << ", skippedFrameCount=" << postSyncSkippedFrameCount_
                << '\n'; 
        }
