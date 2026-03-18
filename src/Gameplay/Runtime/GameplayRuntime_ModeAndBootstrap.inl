        void ResetSimulationState_()
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
                    motor->desiredMoveWorld = {};
                }

                if (GameplayCharacterMovementStateComponent* movementState = world_.TryGetCharacterMovementState(entity))
                {
                    movementState->grounded = true;
                    movementState->jumping = false;
                    movementState->falling = false;
                    movementState->desiredFacingYawDegrees = movementState->facingYawDegrees;
                    movementState->previousFacingYawDegrees = movementState->facingYawDegrees;
                }

                if (GameplayLocomotionComponent* locomotion = world_.TryGetLocomotion(entity))
                {
                    *locomotion = {};
                }

                if (GameplayActionComponent* action = world_.TryGetAction(entity))
                {
                    action->requested = GameplayActionKind::None;
                    action->current = GameplayActionKind::None;
                    action->busy = false;
                    action->requestDispatched = false;
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

        void HandleRuntimeModeChanged_(const GameplayUpdateContext& ctx)
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
                ResetSimulationState_();
                if (ctx.scene != nullptr && ctx.levelInstance != nullptr)
                {
                    PushGameplayStateToAnimation(world_, nodeBoundEntities_, ctx);
                }
            }
        }

        void EnsureBootstrapEntity_(const GameplayUpdateContext& ctx)
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

