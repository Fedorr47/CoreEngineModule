        void ResetEntityFrameState_(const EntityHandle entity)
        {
            if (GameplayInputIntentComponent* intent = world_.TryGetInputIntent(entity))
            {
                *intent = {};
            }

            if (GameplayCharacterCommandComponent* command = world_.TryGetCharacterCommand(entity))
            {
                *command = {};
            }

            if (GameplayAnimationNotifyStateComponent* notifyState = world_.TryGetAnimationNotifyState(entity))
            {
                ResetGameplayAnimationNotifyFrame(*notifyState);
            }

            if (auto it = graphInstances_.find(entity); it != graphInstances_.end())
            {
                ClearGameplayGraphFrameState(it->second);
            }
        }

        void UpdateFollowCamera_(const GameplayUpdateContext& ctx, const bool consumeInput)
        {
            if (ctx.scene == nullptr)
            {
                return;
            }

            GameplayUpdateContext cameraCtx = ctx;
            if (!consumeInput)
            {
                cameraCtx.input = nullptr;
            }

            followCameraController_.Update(world_, controlledEntity_, cameraCtx);
        }

