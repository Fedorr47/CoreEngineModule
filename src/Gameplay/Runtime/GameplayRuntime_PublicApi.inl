        void GameplayRuntime::Initialize(LevelAsset& levelAsset, LevelInstance& levelInstance, Scene& scene)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            defaultGraphAsset_ = MakeDefaultHumanoidGameplayGraphAsset();
            currentLevelAsset_ = &levelAsset;
            currentLevelInstance_ = &levelInstance;
            currentScene_ = &scene;
            actionDefinitions_ = levelAsset.gameplayActions.empty()
                ? MakeDefaultGameplayActionDefinitions() : levelAsset.gameplayActions;
            actionAnimationBindings_ = levelAsset.gameplayActionAnimationBindings.empty()
                ? MakeDefaultGameplayActionAnimationBindings() : levelAsset.gameplayActionAnimationBindings;
            keyboardMouseBindings_ = levelAsset.gameplayKeyboardMouseBindings;
            
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
            CancelAllAIDecisions_();
            aiSystem_.Reset();
            traversalLinkRegistry_.Reset();
            traversalExecutorRegistry_.ResetExternalRegistrations();
            objectReservationSystem_.Reset();
            
            world_.Clear();
            currentLevelAsset_ = nullptr;
            currentLevelInstance_ = nullptr;
            currentScene_ = nullptr;
            recentNotifyEvents_.clear();
            recentGameplayEvents_.clear();
            currentWorldEvents_.clear();
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

        bool GameplayRuntime::ApplyKeyboardMouseBindings(const GameplayKeyboardMouseBindings& bindings)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            if (!IsSupportedGameplayKeyboardKey(bindings.moveX.negativeKey) ||
                !IsSupportedGameplayKeyboardKey(bindings.moveX.positiveKey) ||
                !IsSupportedGameplayKeyboardKey(bindings.moveY.negativeKey) ||
                !IsSupportedGameplayKeyboardKey(bindings.moveY.positiveKey) ||
                !IsSupportedGameplayKeyboardKey(bindings.run.key))
            {
                return false;
            }
            const bool containsReservedKey = std::any_of(bindings.actions.begin(), bindings.actions.end(),
                [](const GameplayActionKeyBinding& binding)
                {
                    return IsGameplayActionBindingKeyReserved(binding.key);
                });
            if (containsReservedKey)
            {
                return false;
            }
            for (std::size_t i = 0; i < bindings.actions.size(); ++i)
            {
                if (!IsSupportedGameplayActionInput(bindings.actions[i].key))
                {
                    return false;
                }
                for (std::size_t j = i + 1; j < bindings.actions.size(); ++j)
                {
                    if (bindings.actions[i].key == bindings.actions[j].key)
                    {
                        return false;
                    }
                }
            }
            std::string diagnostic;
            for (const GameplayActionKeyBinding& binding : bindings.actions)
            {
                if (!ValidateGameplayInputAction(actionDefinitions_, binding.action, diagnostic))
                {
                    return false;
                }
            }
            keyboardMouseBindings_ = bindings;
            if (currentLevelAsset_ != nullptr)
            {
                currentLevelAsset_->gameplayKeyboardMouseBindings = bindings;
                if (!currentLevelAsset_->sourcePath.empty())
                {
                    SaveLevelAssetToJson(currentLevelAsset_->sourcePath, *currentLevelAsset_);
                }
            }
            if (controlledEntity_ == kNullEntity || !world_.IsEntityValid(controlledEntity_))
            {
                return true; // Valid configuration is stored for the next controlled entity.
            }
            BindKeyboardMouseIntentSource(controlledEntity_, keyboardMouseBindings_);
            return true;
        }

        const GameplayKeyboardMouseBindings& GameplayRuntime::GetKeyboardMouseBindings() const noexcept
        {
            return keyboardMouseBindings_;
        }

        const GameplayActionDefinitions& GameplayRuntime::GetGameplayActionDefinitions() const noexcept
        {
            return actionDefinitions_;
        }

        const GameplayActionAnimationBindings& GameplayRuntime::GetGameplayActionAnimationBindings() const noexcept
        {
            return actionAnimationBindings_;
        }

        bool GameplayRuntime::ApplyGameplayActionConfiguration(
            const GameplayActionDefinitions& definitions,
            const GameplayActionAnimationBindings& animationBindings,
            std::string& diagnostic)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            if (!ValidateGameplayActionDefinitions(definitions, diagnostic) ||
                !ValidateGameplayActionAnimationBindings(definitions, animationBindings, diagnostic))
            {
                return false;
            }
            for (const GameplayActionKeyBinding& binding : keyboardMouseBindings_.actions)
            {
                if (!ValidateGameplayInputAction(definitions, binding.action, diagnostic))
                {
                    return false;
                }
            }
            actionDefinitions_ = definitions;
            actionAnimationBindings_ = animationBindings;
            if (currentLevelAsset_ != nullptr)
            {
                currentLevelAsset_->gameplayActions = definitions;
                currentLevelAsset_->gameplayActionAnimationBindings = animationBindings;
                if (!currentLevelAsset_->sourcePath.empty())
                {
                    SaveLevelAssetToJson(currentLevelAsset_->sourcePath, *currentLevelAsset_);
                }
            }
            return true;
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
            currentWorldEvents_.clear();
            CompactTrackedState_();

            for (const EntityHandle entity : nodeBoundEntities_)
            {
                ResetEntityFrameState_(entity);
            }
        }

        void GameplayRuntime::PrePhysicsUpdate(const GameplayUpdateContext& ctx)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            RemoveDeadNodeBoundEntities_(ctx);
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
            std::vector<EntityHandle> pickupCollectors;
            world_.CollectAIEntities(pickupCollectors);
            pickupSystem_.Update(world_, pickupCollectors, currentWorldEvents_);
            const auto hideWorldEventSubjects = [&](const std::size_t firstEvent)
            {
                for (std::size_t eventIndex = firstEvent;
                     eventIndex < currentWorldEvents_.size(); ++eventIndex)
                {
                    const GameplayWorldEvent& event = currentWorldEvents_[eventIndex];
                    if ((event.type != GameplayWorldEventType::PickupCollected &&
                            event.type != GameplayWorldEventType::AccessKeyPurchased) ||
                        ctx.levelAsset == nullptr || ctx.levelInstance == nullptr ||
                        ctx.scene == nullptr)
                    {
                        continue;
                    }
                    if (const GameplayNodeLinkComponent* link = world_.TryGetNodeLink(event.subject))
                    {
                        (void)ctx.levelInstance->SetNodeRuntimeVisible(
                            *ctx.levelAsset, *ctx.scene, link->nodeIndex, false);
                    }
                }
            };
            hideWorldEventSubjects(0u);
            const std::size_t eventCountBeforeAIDecisions = currentWorldEvents_.size();
            UpdateActiveAIDecisions_();
            hideWorldEventSubjects(eventCountBeforeAIDecisions);
            aiSystem_.Update(world_, ctx.deltaSeconds);
            UpdateGameplayCombatRequests(world_, nodeBoundEntities_, actionDefinitions_);
            UpdateGameplayInteractionRequests(world_, nodeBoundEntities_, actionDefinitions_);
            ExecuteGameplayGraphs_(ctx);
            UpdateGameplayCharacterMovement(world_, nodeBoundEntities_, ctx.deltaSeconds);
        }

        void GameplayRuntime::PostPhysicsUpdate(const GameplayUpdateContext& ctx)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            if (ctx.mode != GameplayRuntimeMode::Game)
            {
                return;
            }
            
            UpdateGameplayCharacterLocomotion(world_, nodeBoundEntities_);
            SyncGameplayTransformsToRuntime(world_, nodeBoundEntities_, ctx);
            UpdateFollowCamera_(ctx, false);
            PushGameplayStateToAnimation(world_, nodeBoundEntities_, ctx, actionAnimationBindings_);
            
            const auto syncStartedAt = std::chrono::steady_clock::now();
            std::size_t processedEntityCount = 0;
            SyncGameplayAnimationStateFromRuntime(
                world_,
                nodeBoundEntities_,
                ctx,
                actionAnimationBindings_,
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
                    actionAnimationBindings_,
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

        [[nodiscard]] std::span<const GameplayWorldEvent> GameplayRuntime::GetCurrentWorldEvents() const noexcept
        {
            return currentWorldEvents_;
        }

        void GameplayRuntime::ClearCurrentWorldEvents() noexcept
        {
            currentWorldEvents_.clear();
        }

        void GameplayRuntime::SetObstacleQuery(const IGameplayObstacleQuery* query) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            obstacleQuery_ = query;
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
                steeringSettings,
                obstacleQuery_);
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
                steeringSettings,
                obstacleQuery_);
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

        void GameplayRuntime::ClearAIAction(const EntityHandle agentEntity)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            aiSystem_.ClearAction(agentEntity);
        }

        [[nodiscard]] AIActionExecutionStatus GameplayRuntime::GetAIActionStatus(
            const EntityHandle agentEntity) const noexcept
        {
            return aiSystem_.GetActionStatus(agentEntity);
        }

        [[nodiscard]] std::unique_ptr<AIMoveToActionBinding>
            GameplayRuntime::CreateAIMoveToActionBinding(
            IAIMoveToActionRequestProvider& requestProvider)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return std::make_unique<AIMoveToActionBinding>(
                world_, traversalLinkRegistry_, traversalExecutorRegistry_, requestProvider,
                obstacleQuery_);
        }

        [[nodiscard]] AIPlanExecutionStatus GameplayRuntime::UpdateAIDecision(
            AIDecisionRuntime& decision,
            const AIAgentWorldState& observedState,
            const std::span<const AIGoalSelectionCandidate> candidates,
            const std::span<const AIActionDefinition> actions,
            const AIActionBindingRegistry& bindings)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            return decision.Update(observedState, candidates, actions, bindings, aiSystem_, world_);
        }

        void GameplayRuntime::CancelAIDecision(AIDecisionRuntime& decision) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            decision.Cancel(aiSystem_);
        }

        bool GameplayRuntime::StartAIDecision(
            const EntityHandle agentEntity, const std::string_view definitionId)
        {
            CORE_ASSERT_RUNTIME_THREAD();
            if (lastMode_ != GameplayRuntimeMode::Game || activeAIDecisions_.contains(agentEntity) ||
                !world_.IsEntityValid(agentEntity) || !world_.HasAI(agentEntity) ||
                !world_.HasTransform(agentEntity) || !world_.HasCharacterCommand(agentEntity) ||
                !world_.HasCharacterMotor(agentEntity) ||
                !world_.HasCharacterMovementState(agentEntity) || currentLevelAsset_ == nullptr)
            {
                return false;
            }
            std::unique_ptr<GameplayAIDecisionInstance> decision =
                aiDecisionFactoryRegistry_.Create(definitionId,
                    GameplayAIDecisionCreationContext{agentEntity, *currentLevelAsset_, world_,
                        traversalLinkRegistry_, traversalExecutorRegistry_,
                        objectReservationSystem_});
            if (!decision)
            {
                return false;
            }
            
            // StartAIDecision is a synchronous start boundary. Perform the
            // initial observation/planning pass here so a successful start
            // never exposes NotStarted to callers before the next gameplay tick.
            decision->Update(aiSystem_, GameplayAIObservationContext{
                 world_, currentWorldEvents_, &currentWorldEvents_});
            const GameplayAIDecisionStatus status = decision->GetStatus();
            if (status == GameplayAIDecisionStatus::NotStarted ||
                status == GameplayAIDecisionStatus::Failed ||
                status == GameplayAIDecisionStatus::Cancelled)
            {
                decision->Cancel(aiSystem_);
                return false;
            }
            
            activeAIDecisions_.emplace(agentEntity, std::move(decision));
            return true;
        }

        void GameplayRuntime::CancelAIDecision(const EntityHandle agentEntity) noexcept
        {
            CORE_ASSERT_RUNTIME_THREAD();
            const auto found = activeAIDecisions_.find(agentEntity);
            if (found == activeAIDecisions_.end())
            {
                return;
            }
            found->second->Cancel(aiSystem_);
            activeAIDecisions_.erase(found);
        }

        AIPlanExecutionStatus GameplayRuntime::GetAIDecisionStatus(const EntityHandle agentEntity) const noexcept
        {
            const auto found = activeAIDecisions_.find(agentEntity);
            if (found == activeAIDecisions_.end())
            {
                return AIPlanExecutionStatus::NotStarted;
            }
            const auto* inspection =
                dynamic_cast<const IGameplayGOAPInspection*>(found->second.get());
            return inspection == nullptr
                ? AIPlanExecutionStatus::NotStarted : inspection->GetGOAPStatus();
        }

        const AIAgentWorldState* GameplayRuntime::GetAIDecisionObservedState(
            const EntityHandle agentEntity) const noexcept
        {
            const auto found = activeAIDecisions_.find(agentEntity);
            if (found == activeAIDecisions_.end())
            {
                return nullptr;
            }
            const auto* inspection =
                dynamic_cast<const IGameplayGOAPInspection*>(found->second.get());
            return inspection == nullptr ? nullptr : &inspection->GetObservedState();
        }

        std::vector<GameplayAIDebugAgentView> GameplayRuntime::BuildAIDebugAgentViews() const
        {
            std::vector<GameplayAIDebugAgentView> result;
            result.reserve(activeAIDecisions_.size());
            for (const auto& [agent, decision] : activeAIDecisions_)
            {
                const auto* inspection =
                    dynamic_cast<const IGameplayGOAPInspection*>(decision.get());
                if (inspection != nullptr)
                {
                    result.push_back({agent, inspection->BuildDebugViewModel()});
                }
            }
            std::ranges::sort(result, {}, &GameplayAIDebugAgentView::agent);
            return result;
        }

        std::vector<GameplayAIPlannedPathDebugAgentView>
            GameplayRuntime::BuildAIPlannedPathDebugAgentViews() const
        {
            std::vector<GameplayAIPlannedPathDebugAgentView> result;
            result.reserve(activeAIDecisions_.size());
            for (const auto& [agent, decision] : activeAIDecisions_)
            {
                const auto* inspection =
                    dynamic_cast<const IGameplayGOAPPathInspection*>(decision.get());
                if (inspection != nullptr)
                {
                    result.push_back({agent, inspection->BuildPlannedPathDebugView()});
                }
            }
            std::ranges::sort(result, {}, &GameplayAIPlannedPathDebugAgentView::agent);
            return result;
        }

        bool GameplayRuntime::HasAIDecisionDefinition(const std::string_view definitionId) const noexcept
        {
            return aiDecisionFactoryRegistry_.Contains(definitionId);
        }

        void GameplayRuntime::UpdateActiveAIDecisions_()
        {
            std::vector<EntityHandle> orderedAgents;
            orderedAgents.reserve(activeAIDecisions_.size());
            for (const auto& [entity, decision] : activeAIDecisions_)
            {
                (void)decision;
                if (world_.IsEntityValid(entity))
                {
                    orderedAgents.push_back(entity);
                }
            }
            std::ranges::sort(orderedAgents);
            for (const EntityHandle entity : orderedAgents)
            {
                const auto found = activeAIDecisions_.find(entity);
                if (found != activeAIDecisions_.end())
                {
                    found->second->Update(aiSystem_, GameplayAIObservationContext{
                        world_, currentWorldEvents_, &currentWorldEvents_});
                }
            }
        }

        void GameplayRuntime::CancelAllAIDecisions_() noexcept
        {
            for (auto& [entity, decision] : activeAIDecisions_)
            {
                (void)entity;
                decision->Cancel(aiSystem_);
            }
            activeAIDecisions_.clear();
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
                RemoveDeadNodeBoundEntities_(ctx);
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
                BindKeyboardMouseIntentSource(entity, keyboardMouseBindings_);
            }
            return entity;
        }

        [[nodiscard]] bool GameplayRuntime::DestroyNodeBoundEntity(const EntityHandle entity)
        {
            CORE_ASSERT_RUNTIME_THREAD();

            const auto tracked = std::find(
                nodeBoundEntities_.begin(), nodeBoundEntities_.end(), entity);
            if (tracked == nodeBoundEntities_.end())
            {
                return false;
            }

            CancelAIDecision(entity);
            ClearAIAction(entity);
            UnbindIntentSource(entity);
            graphInstances_.erase(entity);
            nodeBoundEntities_.erase(
                std::remove(nodeBoundEntities_.begin(), nodeBoundEntities_.end(), entity),
                nodeBoundEntities_.end());
            if (controlledEntity_ == entity)
            {
                controlledEntity_ = kNullEntity;
            }
            world_.DestroyEntity(entity);
            objectReservationSystem_.CleanupInvalidReservations(world_);
            return true;
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
