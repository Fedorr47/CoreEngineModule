module;

#include <string>
#include <vector>

export module core:gameplay_animation_bridge_system;

import :gameplay;
import :gameplay_runtime_common;
import :gameplay_animation_bridge;
import :level;
import :scene;
import :animation_controller;

export namespace rendern
{
    inline void PushGameplayStateToAnimation(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayUpdateContext& ctx)
    {
        if (ctx.levelInstance == nullptr || ctx.scene == nullptr)
        {
            return;
        }

        for (const EntityHandle entity : entities)
        {
            const GameplayAnimationLinkComponent* animLink = world.TryGetAnimationLink(entity);
            const GameplayLocomotionComponent* locomotion = world.TryGetLocomotion(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (animLink == nullptr || animLink->skinnedDrawIndex < 0)
            {
                continue;
            }

            SkinnedDrawItem* skinnedItem = ctx.levelInstance->GetSkinnedDrawItem(*ctx.scene, animLink->skinnedDrawIndex);
            if (skinnedItem == nullptr || skinnedItem->controller.stateMachineAsset == nullptr)
            {
                continue;
            }

            if (locomotion != nullptr)
            {
                WriteGameplayLocomotionAnimationParameters(skinnedItem->controller, *locomotion);
            }

            if (action != nullptr)
            {
                WriteGameplayActionAnimationParameters(skinnedItem->controller, *action);
            }
        }
    }

    inline void ConsumeGameplayAnimationEvents(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayUpdateContext& ctx,
        std::vector<GameplayAnimationNotifyRecord>& outNotifyRecords,
        std::vector<GameplayEventRecord>& outGameplayEvents)
    {
        outNotifyRecords.clear();
        outGameplayEvents.clear();

        if (ctx.levelInstance == nullptr || ctx.scene == nullptr)
        {
            return;
        }

        for (const EntityHandle entity : entities)
        {
            const GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);
            GameplayAnimationLinkComponent* animLink = world.TryGetAnimationLink(entity);
            GameplayAnimationNotifyStateComponent* notifyState = world.TryGetAnimationNotifyState(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (nodeLink == nullptr || animLink == nullptr || notifyState == nullptr || animLink->skinnedDrawIndex < 0)
            {
                continue;
            }

            SkinnedDrawItem* skinnedItem = ctx.levelInstance->GetSkinnedDrawItem(*ctx.scene, animLink->skinnedDrawIndex);
            if (skinnedItem == nullptr)
            {
                continue;
            }

            std::vector<AnimationNotifyEvent> events = ConsumeAnimationControllerNotifyEvents(skinnedItem->controller);
            if (events.empty())
            {
                continue;
            }

            for (const AnimationNotifyEvent& event : events)
            {
                outNotifyRecords.push_back(GameplayAnimationNotifyRecord{
                    .entity = entity,
                    .nodeIndex = nodeLink->nodeIndex,
                    .skinnedDrawIndex = animLink->skinnedDrawIndex,
                    .event = event
                });

                ApplyAnimationNotifyToGameplayState(*notifyState, action, event);

                std::vector<std::string> gameplayEventIds{};
                CollectGameplayEventIdsForAnimationEvent(skinnedItem->controller.stateMachineAsset, event, gameplayEventIds);
                for (const std::string& gameplayEventId : gameplayEventIds)
                {
                    ApplyGameplayEventToGameplayState(*notifyState, action, gameplayEventId, event);
                    outGameplayEvents.push_back(GameplayEventRecord{
                        .entity = entity,
                        .nodeIndex = nodeLink->nodeIndex,
                        .skinnedDrawIndex = animLink->skinnedDrawIndex,
                        .sequence = event.sequence,
                        .animationEventId = event.id,
                        .gameplayEventId = gameplayEventId,
                        .stateName = event.stateName,
                        .clipName = event.clipName,
                        .normalizedTime = event.normalizedTime
                    });
                }
            }
        }
    }
}
