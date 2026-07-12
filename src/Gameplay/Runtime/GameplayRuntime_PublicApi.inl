        void GameplayRuntime::Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene)
        {
            CORE_ASSERT_RUNTIME_THREAD();

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
            CORE_ASSERT_RUNTIME_THREAD();

            LogSyncInstrumentationSample_();
            
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
            
            aiSystem_.Update(world_);

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

        [[nodiscard]] const std::vector<EntityHandle>& GameplayRuntime::GetNodeBoundEntities() const noexcept
        {
            return nodeBoundEntities_;
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
