        [[nodiscard]] static bool EvaluateGraphTransition_(
            const GameplayGraphInstance& graph,
            const GameplayGraphTransitionDesc& transition)
        {
            for (const GameplayGraphConditionDesc& condition : transition.conditions)
            {
                const std::string conditionName = CanonicalizeGameplayGraphToken(condition.name);
                if (conditionName == "booltrue")
                {
                    if (!GetGameplayGraphBool(graph.parameters, condition.parameter, false))
                    {
                        return false;
                    }
                    continue;
                }
                if (conditionName == "boolfalse")
                {
                    if (GetGameplayGraphBool(graph.parameters, condition.parameter, false))
                    {
                        return false;
                    }
                    continue;
                }
                if (conditionName == "floatgreater")
                {
                    if (!(GetGameplayGraphFloat(graph.parameters, condition.parameter, 0.0f) > condition.threshold))
                    {
                        return false;
                    }
                    continue;
                }
                if (conditionName == "floatless")
                {
                    if (!(GetGameplayGraphFloat(graph.parameters, condition.parameter, 0.0f) < condition.threshold))
                    {
                        return false;
                    }
                    continue;
                }

                return false;
            }

            return true;
        }

        static void CompactEntityVector_(std::vector<EntityHandle>& entities, const GameplayWorld& world)
        {
            entities.erase(
                std::remove_if(entities.begin(),
                    entities.end(),
                    [&world](const EntityHandle entity)
                    {
                        return entity == kNullEntity || !world.IsEntityValid(entity);
                    }),
                entities.end());
        }

        void CompactTrackedState_()
        {
            CompactEntityVector_(nodeBoundEntities_, world_);

            intentBindings_.erase(
                std::remove_if(intentBindings_.begin(),
                    intentBindings_.end(),
                    [this](const GameplayIntentBinding& binding)
                    {
                        return binding.entity == kNullEntity ||
                            !binding.callback ||
                            !world_.IsEntityValid(binding.entity);
                    }),
                intentBindings_.end());

            for (auto it = graphInstances_.begin(); it != graphInstances_.end(); )
            {
                if (!world_.IsEntityValid(it->first))
                {
                    it = graphInstances_.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (controlledEntity_ != kNullEntity && !world_.IsEntityValid(controlledEntity_))
            {
                controlledEntity_ = kNullEntity;
            }
        }

        void UpsertIntentBinding_(const EntityHandle entity, GameplayIntentSourceCallback callback)
        {
            for (GameplayIntentBinding& binding : intentBindings_)
            {
                if (binding.entity == entity)
                {
                    binding.callback = std::move(callback);
                    return;
                }
            }

            intentBindings_.push_back(GameplayIntentBinding{
                .entity = entity,
                .callback = std::move(callback)
            });
        }

        void CreateDefaultGraphInstance_(const EntityHandle entity)
        {
            GameplayGraphInstance instance{};
            instance.asset = &defaultGraphAsset_;
            instance.layers.reserve(defaultGraphAsset_.layers.size());

            for (const GameplayGraphLayerDesc& layer : defaultGraphAsset_.layers)
            {
                const int defaultStateIndex = FindGameplayGraphStateIndex(layer, layer.defaultState);
                instance.layers.push_back(GameplayGraphLayerRuntimeState{
                    .activeStateIndex = defaultStateIndex,
                    .previousStateIndex = -1,
                    .stateTime = 0.0f,
                    .enterPending = true
                });
            }

            SetGameplayGraphBool(instance.parameters, "hasActionRequest", false);
            SetGameplayGraphBool(instance.parameters, "actionBusy", false);
            SetGameplayGraphInt(instance.parameters, "requestedActionKind", 0);
            SetGameplayGraphInt(instance.parameters, "currentAction", 0);

            graphInstances_.insert_or_assign(entity, std::move(instance));
        }

        void SyncActionStateToGraphParameters_(const EntityHandle entity, GameplayGraphInstance& graph)
        {
            const GameplayActionComponent* action = world_.TryGetAction(entity);
            if (action == nullptr)
            {
                SetGameplayGraphBool(graph.parameters, "hasActionRequest", false);
                SetGameplayGraphBool(graph.parameters, "actionBusy", false);
                SetGameplayGraphInt(graph.parameters, "requestedActionKind", 0);
                SetGameplayGraphInt(graph.parameters, "currentAction", 0);
                return;
            }

            SetGameplayGraphBool(graph.parameters, "hasActionRequest", action->requested != GameplayActionKind::None);
            SetGameplayGraphBool(graph.parameters, "actionBusy", action->busy);
            SetGameplayGraphInt(graph.parameters, "requestedActionKind", static_cast<int>(action->requested));
            SetGameplayGraphInt(graph.parameters, "currentAction", static_cast<int>(action->current));
        }

        void ExecuteGameplayGraphs_(const GameplayUpdateContext& ctx)
        {
            for (const EntityHandle entity : nodeBoundEntities_)
            {
                auto it = graphInstances_.find(entity);
                if (it == graphInstances_.end())
                {
                    continue;
                }

                GameplayGraphInstance& graph = it->second;
                SyncActionStateToGraphParameters_(entity, graph);

                for (std::size_t layerIndex = 0; layerIndex < graph.layers.size() && layerIndex < graph.asset->layers.size(); ++layerIndex)
                {
                    GameplayGraphLayerRuntimeState& runtimeLayer = graph.layers[layerIndex];
                    const GameplayGraphLayerDesc& assetLayer = graph.asset->layers[layerIndex];
                    ExecuteGraphLayer_(entity, graph, runtimeLayer, assetLayer, ctx);
                }

                SyncActionStateToGraphParameters_(entity, graph);
            }
        }

        void ExecuteGraphLayer_(
            const EntityHandle entity,
            GameplayGraphInstance& graph,
            GameplayGraphLayerRuntimeState& runtimeLayer,
            const GameplayGraphLayerDesc& assetLayer,
            const GameplayUpdateContext& ctx)
        {
            if (runtimeLayer.activeStateIndex < 0 ||
                static_cast<std::size_t>(runtimeLayer.activeStateIndex) >= assetLayer.states.size())
            {
                runtimeLayer.activeStateIndex = FindGameplayGraphStateIndex(assetLayer, assetLayer.defaultState);
                runtimeLayer.enterPending = true;
                runtimeLayer.stateTime = 0.0f;
            }
            if (runtimeLayer.activeStateIndex < 0)
            {
                return;
            }

            const GameplayGraphStateDesc* state = &assetLayer.states[static_cast<std::size_t>(runtimeLayer.activeStateIndex)];
            if (runtimeLayer.enterPending)
            {
                ExecuteGraphTasks_(entity, graph, *state, state->onEnter);
                runtimeLayer.enterPending = false;
            }

            ExecuteGraphTasks_(entity, graph, *state, state->onUpdate);

            for (const GameplayGraphTransitionDesc& transition : state->transitions)
            {
                if (!EvaluateGraphTransition_(graph, transition))
                {
                    continue;
                }

                ExecuteGraphTasks_(entity, graph, *state, state->onExit);

                runtimeLayer.previousStateIndex = runtimeLayer.activeStateIndex;
                runtimeLayer.activeStateIndex = FindGameplayGraphStateIndex(assetLayer, transition.toState);
                runtimeLayer.stateTime = 0.0f;
                runtimeLayer.enterPending = true;

                if (runtimeLayer.activeStateIndex >= 0 &&
                    static_cast<std::size_t>(runtimeLayer.activeStateIndex) < assetLayer.states.size())
                {
                    const GameplayGraphStateDesc& newState = assetLayer.states[static_cast<std::size_t>(runtimeLayer.activeStateIndex)];
                    ExecuteGraphTasks_(entity, graph, newState, newState.onEnter);
                    runtimeLayer.enterPending = false;
                }
                return;
            }

            runtimeLayer.stateTime += std::max(ctx.deltaSeconds, 0.0f);
        }

        void ExecuteGraphTasks_(
            const EntityHandle entity,
            GameplayGraphInstance& graph,
            const GameplayGraphStateDesc& state,
            const std::vector<GameplayGraphTaskDesc>& tasks)
        {
            for (const GameplayGraphTaskDesc& task : tasks)
            {
                const std::string taskName = CanonicalizeGameplayGraphToken(task.name);
                if (taskName == "beginactionstate")
                {
                    BeginActionState_(entity, graph);
                }
                [[maybe_unused]] const auto& unusedState = state;
            }
        }

        void BeginActionState_(const EntityHandle entity, GameplayGraphInstance& graph)
        {
            GameplayActionComponent* action = world_.TryGetAction(entity);
            if (action == nullptr || action->requested == GameplayActionKind::None)
            {
                return;
            }

            action->busy = true;
            if (action->current == GameplayActionKind::None)
            {
                action->current = action->requested;
            }

            SetGameplayGraphBool(graph.parameters, "hasActionRequest", true);
            SetGameplayGraphBool(graph.parameters, "actionBusy", action->busy);
            SetGameplayGraphInt(graph.parameters, "requestedActionKind", static_cast<int>(action->requested));
            SetGameplayGraphInt(graph.parameters, "currentAction", static_cast<int>(action->current));
        }

