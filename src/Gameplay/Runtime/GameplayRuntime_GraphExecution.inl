        [[nodiscard]] static bool EvaluateGraphTransition_(
            const GameplayGraphInstance& graph,
            const GameplayGraphTransitionDesc& transition)
        {
            for (const GameplayGraphConditionDesc& condition : transition.conditions)
            {
                switch (condition.opcode)
                {
                case GameplayGraphConditionOpcode::BoolTrue:
                    if (!GetGameplayGraphBool(graph.parameters, condition.parameter, false))
                    {
                        return false;
                    }
                    break;

                case GameplayGraphConditionOpcode::BoolFalse:
                    if (GetGameplayGraphBool(graph.parameters, condition.parameter, false))
                    {
                        return false;
                    }
                    break;

                case GameplayGraphConditionOpcode::FloatGreater:
                    if (!(GetGameplayGraphFloat(graph.parameters, condition.parameter, 0.0f) > condition.threshold))
                    {
                        return false;
                    }
                    break;

                case GameplayGraphConditionOpcode::FloatLess:
                    if (!(GetGameplayGraphFloat(graph.parameters, condition.parameter, 0.0f) < condition.threshold))
                    {
                        return false;
                    }
                    break;

                case GameplayGraphConditionOpcode::Unknown:
                default:
                    return false;
                }
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
            SetGameplayGraphBool(instance.parameters, "hasBufferedAction", false);
            SetGameplayGraphBool(instance.parameters, "actionBusy", false);
            SetGameplayGraphInt(instance.parameters, "requestedActionKind", 0);
            SetGameplayGraphInt(instance.parameters, "bufferedActionKind", 0);
            SetGameplayGraphInt(instance.parameters, "currentAction", 0);
            SetGameplayGraphString(instance.parameters, "currentAnimationController", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationState", {});
            SetGameplayGraphString(instance.parameters, "previousAnimationState", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationMode", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationPrimaryClip", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationSecondaryClip", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationTertiaryClip", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationBlendParameterX", {});
            SetGameplayGraphString(instance.parameters, "currentAnimationBlendParameterY", {});
            SetGameplayGraphFloat(instance.parameters, "currentAnimationBlendValueX", 0.0f);
            SetGameplayGraphFloat(instance.parameters, "currentAnimationBlendValueY", 0.0f);
            SetGameplayGraphFloat(instance.parameters, "currentAnimationNormalizedTime", 0.0f);
            SetGameplayGraphBool(instance.parameters, "currentAnimationUsesBlend1D", false);
            SetGameplayGraphBool(instance.parameters, "currentAnimationUsesBlend2D", false);
            SetGameplayGraphBool(instance.parameters, "currentAnimationTransitionActive", false);
            SetGameplayGraphBool(instance.parameters, "currentAnimationEnteredThisFrame", false);
            SetGameplayGraphBool(instance.parameters, "currentAnimationChangedThisFrame", false);

            graphInstances_.insert_or_assign(entity, std::move(instance));
        }

        void SyncActionStateToGraphParameters_(const EntityHandle entity, GameplayGraphInstance& graph)
        {
            const GameplayActionComponent* action = world_.TryGetAction(entity);
            WriteActionStateToGraphParameters_(graph, action);
        }

        void WriteActionStateToGraphParameters_(
            GameplayGraphInstance& graph,
            const GameplayActionComponent* action)
        {
            bool hasActionRequest = false;
            bool hasBufferedAction = false;
            bool actionBusy = false;
            int requestedActionKind = 0;
            int bufferedActionKind = 0;
            int currentAction = 0;

            if (action != nullptr)
            {
                hasActionRequest = HasGameplayPendingActionRequest(*action);
                hasBufferedAction = HasGameplayBufferedActionRequest(*action);
                actionBusy = action->busy;
                requestedActionKind = static_cast<int>(GetGameplayRequestedActionKind(*action));
                bufferedActionKind = static_cast<int>(GetGameplayBufferedActionKind(*action));
                currentAction = static_cast<int>(action->current);
            }

            SetGameplayGraphBool(graph.parameters, "hasActionRequest", hasActionRequest);
            SetGameplayGraphBool(graph.parameters, "hasBufferedAction", hasBufferedAction);
            SetGameplayGraphBool(graph.parameters, "actionBusy", actionBusy);
            SetGameplayGraphInt(graph.parameters, "requestedActionKind", requestedActionKind);
            SetGameplayGraphInt(graph.parameters, "bufferedActionKind", bufferedActionKind);
            SetGameplayGraphInt(graph.parameters, "currentAction", currentAction);
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
                ExecuteGraphTasks_(entity, graph, state->onEnter);
                runtimeLayer.enterPending = false;
            }

            ExecuteGraphTasks_(entity, graph, state->onUpdate);

            for (const GameplayGraphTransitionDesc& transition : state->transitions)
            {
                if (!EvaluateGraphTransition_(graph, transition))
                {
                    continue;
                }

                ExecuteGraphTasks_(entity, graph, state->onExit);

                runtimeLayer.previousStateIndex = runtimeLayer.activeStateIndex;
                runtimeLayer.activeStateIndex = FindGameplayGraphStateIndex(assetLayer, transition.toState);
                runtimeLayer.stateTime = 0.0f;
                runtimeLayer.enterPending = true;

                if (runtimeLayer.activeStateIndex >= 0 &&
                    static_cast<std::size_t>(runtimeLayer.activeStateIndex) < assetLayer.states.size())
                {
                    const GameplayGraphStateDesc& newState = assetLayer.states[static_cast<std::size_t>(runtimeLayer.activeStateIndex)];
                    ExecuteGraphTasks_(entity, graph, newState.onEnter);
                    runtimeLayer.enterPending = false;
                }
                return;
            }

            runtimeLayer.stateTime += std::max(ctx.deltaSeconds, 0.0f);
        }

        void ExecuteGraphTasks_(
            const EntityHandle entity,
            GameplayGraphInstance& graph,
            const std::vector<GameplayGraphTaskDesc>& tasks)
        {
            for (const GameplayGraphTaskDesc& task : tasks)
            {
                switch (task.opcode)
                {
                case GameplayGraphTaskOpcode::BeginActionState:
                    BeginActionState_(entity, graph);
                    break;
                case GameplayGraphTaskOpcode::Unknown:
                    [[fallthrough]];
                default:
                    break;
                }

            }
        }

        void BeginActionState_(const EntityHandle entity, GameplayGraphInstance& graph)
        {
            GameplayActionComponent* action = world_.TryGetAction(entity);
            if (action == nullptr || !HasGameplayPendingActionRequest(*action))
            {
                return;
            }

            PrimeGameplayActionState(*action);

            WriteActionStateToGraphParameters_(graph, action);
        }

