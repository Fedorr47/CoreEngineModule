        void GameplayRuntime::Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene)
        {
            defaultGraphAsset_ = MakeDefaultHumanoidGameplayGraphAsset();

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
            LogSyncInstumentationSample_();
            
            world_.Clear();
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

        void GameplayRuntime::BindIntentSource(const EntityHandle entity, GameplayIntentSourceCallback callback)
        {
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
            RecordSyncInstumentationSample_(
                preSyncInstAggregate_,
                std::chrono::duration_cast<std::chrono::nanoseconds>(syncEndedAt - syncStartedAt),
                processedEntityCount);
        }

        void GameplayRuntime::PostAnimationUpdate(const GameplayUpdateContext& ctx)
        {
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
                RecordSyncInstumentationSample_(
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

        [[nodiscard]] const GameplayWorld& GameplayRuntime::GetWorld() const noexcept
        {
            return world_;
        }

        [[nodiscard]] EntityHandle GameplayRuntime::GetControlledEntity() const noexcept
        {
            return controlledEntity_;
        }

        [[nodiscard]] const std::vector<EntityHandle>& GameplayRuntime::GetNodeBoundEntities() const noexcept
        {
            return nodeBoundEntities_;
        }

        [[nodiscard]] EntityHandle GameplayRuntime::SpawnNodeBoundEntity(
            const GameplayUpdateContext& ctx,
            const int nodeIndex,
            const bool playerControlled)
        {
            if (ctx.levelAsset == nullptr || ctx.levelInstance == nullptr || ctx.scene == nullptr)
            {
                return kNullEntity;
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
    
        void GameplayRuntime::RecordSyncInstumentationSample_(
            ProfileUtils::SyncInststrumentionAggregate& aggregate,
            const std::chrono::nanoseconds duration, 
            std::size_t processedEntityCount)
        {
            ++aggregate.callCount;
             aggregate.totalProcessedEntityCount += processedEntityCount;
             aggregate.totalDuration += duration;
             aggregate.maxDuration = std::max(aggregate.maxDuration, duration);
        }
    
        void GameplayRuntime::LogSyncInstumentationSample_() const
        {
            const auto printSummary = [](std::string_view passName, const ProfileUtils::SyncInststrumentionAggregate& aggregate)
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
