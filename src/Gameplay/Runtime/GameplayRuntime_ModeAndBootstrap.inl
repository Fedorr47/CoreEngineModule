        void GameplayRuntime::ResetSimulationState_()
        {
            for (const EntityHandle entity : nodeBoundEntities_)
            {
                if (GameplayInputIntentComponent* intent = world_.TryGetInputIntent(entity))
                {
                    *intent = {};
                }

                if (GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(entity))
                {
                    *command = {};
                }

                if (GameplayCharacterMotorComponent* motor = world_.TryGetCharacterMotor(entity))
                {
                    motor->velocity = {};
                    motor->desiredVelocity = {};
                    motor->desiredMoveWorld = {};
                }

                if (GameplayCharacterMovementStateComponent* movementState = world_.TryGetCharacterMovementState(entity))
                {
                    movementState->grounded = true;
                    movementState->jumping = false;
                    movementState->falling = false;
                    movementState->physicallyBlocked = false;
                    movementState->physicalBlockedSeconds = 0.0f;
                    movementState->jumpPhase = GameplayJumpPhase::None;
                    movementState->jumpRequestConsumed = false;
                    movementState->jumpAirbornePhysicallyObserved = false;
                    movementState->turningInPlace = false;
                    movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                    movementState->previousFacingYawDegrees = movementState->facingYawDegrees;
                    movementState->cameraFacingYawDegrees = movementState->facingYawDegrees;
                    movementState->jumpLockedVelocity = {};
                }

                if (GameplayLocomotionComponent* locomotion = world_.TryGetLocomotion(entity))
                {
                    *locomotion = {};
                }

                if (GameplayActionComponent* action = world_.TryGetAction(entity))
                {
                    ResetGameplayActionState(*action);
                }

                if (GameplayAnimationNotifyStateComponent* notifyState = world_.TryGetAnimationNotifyState(entity))
                {
                    *notifyState = {};
                }

                if (auto it = graphInstances_.find(entity); it != graphInstances_.end())
                {
                    ClearGameplayGraphFrameState(it->second);
                    SyncActionStateToGraphParameters_(entity, it->second);
                }
            }
        }

        void GameplayRuntime::RemoveDeadNodeBoundEntities_(const GameplayUpdateContext& ctx)
        {
            if (ctx.levelAsset == nullptr)
            {
                return;
            }

            for (auto it = nodeBoundEntities_.begin(); it != nodeBoundEntities_.end(); )
            {
                const EntityHandle entity = *it;
                const GameplayNodeLinkComponent* link = world_.TryGetNodeLink(entity);
                const bool bHasValidNodeIndex = link != nullptr && link->nodeIndex >= 0 &&
                    static_cast<std::size_t>(link->nodeIndex) < ctx.levelAsset->nodes.size();
                const bool bIsNodeAlive = bHasValidNodeIndex &&
                    ctx.levelAsset->nodes[static_cast<std::size_t>(link->nodeIndex)].alive;
                if (bIsNodeAlive)
                {
                    ++it;
                    continue;
                }

                world_.DestroyEntity(entity);
                graphInstances_.erase(entity);
                UnbindIntentSource(entity);
                it = nodeBoundEntities_.erase(it);
                if (controlledEntity_ == entity)
                {
                    controlledEntity_ = kNullEntity;
                }
            }
        }

        void GameplayRuntime::HandleRuntimeModeChanged_(const GameplayUpdateContext& ctx)
        {
            recentNotifyEvents_.clear();
            recentGameplayEvents_.clear();

            for (const EntityHandle entity : nodeBoundEntities_)
            {
                if (GameplayFollowCameraComponent* followCamera = world_.TryGetFollowCamera(entity))
                {
                    followCamera->initialized = false;
                }
            }

            if (ctx.mode == GameplayRuntimeMode::Editor)
            {
                aiSystem_.Reset();
                objectReservationSystem_.Reset();
                // Runtime-registered traversal services belong to one simulation session.
                traversalLinkRegistry_.Reset();
                traversalExecutorRegistry_.Reset();
                ResetSimulationState_();
                if (ctx.scene != nullptr && ctx.levelInstance != nullptr)
                {
                    PushGameplayStateToAnimation(world_, nodeBoundEntities_, ctx);
                }
            }
            else if (ctx.mode == GameplayRuntimeMode::Game)
            {
                RegisterBuiltInTraversalExecutors_();
            }
        }

        void GameplayRuntime::RegisterBuiltInTraversalExecutors_()
        {
            if (traversalExecutorRegistry_.Contains(kDoorTraversalTypeId))
            {
                return;
            }
            const bool bRegistered = traversalExecutorRegistry_.Register(kDoorTraversalTypeId, doorTraversalExecutor_);
            assert(bRegistered && "Built-in Door traversal executor must register exactly once.");
        }

        void GameplayRuntime::EnsureBootstrapEntity_(const GameplayUpdateContext& ctx)
        {
            if (controlledEntity_ != kNullEntity && world_.IsEntityValid(controlledEntity_))
            {
                return;
            }
            if (ctx.levelAsset == nullptr || ctx.levelInstance == nullptr || ctx.scene == nullptr)
            {
                return;
            }

            const int bootstrapNodeIndex = FindGameplayBootstrapNodeIndex(*ctx.levelAsset, *ctx.levelInstance);
            if (bootstrapNodeIndex < 0)
            {
                return;
            }

            SpawnNodeBoundEntity(ctx, bootstrapNodeIndex, true);
        }

