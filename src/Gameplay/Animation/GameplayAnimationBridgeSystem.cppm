module;

#include <string>
#include <unordered_map>
#include <vector>

export module core:gameplay_animation_bridge_system;

import :gameplay;
import :gameplay_runtime_common;
import :gameplay_graph;
import :gameplay_animation_bridge;
import :level;
import :scene;
import :animation_controller;

export namespace rendern
{

    namespace detail
    {
        [[nodiscard]] inline const char* AnimationModeName_(const AnimationControllerRuntime& controller) noexcept
        {
            if (controller.currentStateUsesBlend2D)
            {
                return "Blend2D";
            }
            if (controller.currentStateUsesBlend1D)
            {
                return "Blend1D";
            }
            return "Clip";
        }

        inline void SyncAnimationRuntimeToGameplayState(
            GameplayAnimationStateComponent& state,
            GameplayGraphInstance* graph,
            const AnimationControllerRuntime& controller)
        {
            const std::string previousStateName = state.currentStateName;

            state.controllerAssetId = controller.controllerAssetId;
            state.previousStateName = previousStateName;
            state.currentStateName = controller.currentStateName;
            state.modeName = AnimationModeName_(controller);
            state.primaryClipName = controller.currentBlendPrimaryClipName;
            state.secondaryClipName = controller.currentBlendSecondaryClipName;
            state.tertiaryClipName = controller.currentBlendTertiaryClipName;
            state.blendParameterNameX = controller.currentBlendParameterName;
            state.blendParameterNameY = controller.currentBlendParameterNameY;
            state.blendParameterValueX = controller.currentBlendParameterValue;
            state.blendParameterValueY = controller.currentBlendParameterValueY;
            state.stateNormalizedTime = controller.previousStateNormalizedTime;
            state.usesBlend1D = controller.currentStateUsesBlend1D;
            state.usesBlend2D = controller.currentStateUsesBlend2D;
            state.transitionActive = controller.transitionActive;
            state.enteredThisFrame = controller.stateEnteredThisFrame;
            state.changedThisFrame = previousStateName != controller.currentStateName;

            if (graph == nullptr)
            {
                return;
            }

            SetGameplayGraphString(graph->parameters, "currentAnimationController", state.controllerAssetId);
            SetGameplayGraphString(graph->parameters, "currentAnimationState", state.currentStateName);
            SetGameplayGraphString(graph->parameters, "previousAnimationState", state.previousStateName);
            SetGameplayGraphString(graph->parameters, "currentAnimationMode", state.modeName);
            SetGameplayGraphString(graph->parameters, "currentAnimationPrimaryClip", state.primaryClipName);
            SetGameplayGraphString(graph->parameters, "currentAnimationSecondaryClip", state.secondaryClipName);
            SetGameplayGraphString(graph->parameters, "currentAnimationTertiaryClip", state.tertiaryClipName);
            SetGameplayGraphString(graph->parameters, "currentAnimationBlendParameterX", state.blendParameterNameX);
            SetGameplayGraphString(graph->parameters, "currentAnimationBlendParameterY", state.blendParameterNameY);
            SetGameplayGraphFloat(graph->parameters, "currentAnimationBlendValueX", state.blendParameterValueX);
            SetGameplayGraphFloat(graph->parameters, "currentAnimationBlendValueY", state.blendParameterValueY);
            SetGameplayGraphFloat(graph->parameters, "currentAnimationNormalizedTime", state.stateNormalizedTime);
            SetGameplayGraphBool(graph->parameters, "currentAnimationUsesBlend1D", state.usesBlend1D);
            SetGameplayGraphBool(graph->parameters, "currentAnimationUsesBlend2D", state.usesBlend2D);
            SetGameplayGraphBool(graph->parameters, "currentAnimationTransitionActive", state.transitionActive);
            SetGameplayGraphBool(graph->parameters, "currentAnimationEnteredThisFrame", state.enteredThisFrame);
            SetGameplayGraphBool(graph->parameters, "currentAnimationChangedThisFrame", state.changedThisFrame);
        }
    }
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
            const GameplayCharacterMovementStateComponent* movementState =
                world.TryGetCharacterMovementState(entity);
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
            
            if (movementState != nullptr)
            {
                WriteGameplayMovementAnimationParameters(skinnedItem->controller, *movementState);
            }

            if (action != nullptr)
            {
                WriteGameplayActionAnimationParameters(skinnedItem->controller, *action);
            }
        }
    }

    inline void SyncGameplayAnimationStateFromRuntime(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayUpdateContext& ctx,
        std::unordered_map<EntityHandle, GameplayGraphInstance>* graphInstances = nullptr,
        std::size_t* outProcessedEntityCount = nullptr)
    {
        if (outProcessedEntityCount != nullptr)
        {
            *outProcessedEntityCount = 0;
        }
        
        if (ctx.levelInstance == nullptr || ctx.scene == nullptr)
        {
            return;
        }

        for (const EntityHandle entity : entities)
        {
            const GameplayAnimationLinkComponent* animLink = world.TryGetAnimationLink(entity);
            GameplayAnimationStateComponent* animState = world.TryGetAnimationState(entity);
            if (animLink == nullptr || animState == nullptr || animLink->skinnedDrawIndex < 0)
            {
                continue;
            }

            SkinnedDrawItem* skinnedItem = ctx.levelInstance->GetSkinnedDrawItem(*ctx.scene, animLink->skinnedDrawIndex);
            if (skinnedItem == nullptr)
            {
                continue;
            }
            
            if (outProcessedEntityCount != nullptr)
            {
                ++(*outProcessedEntityCount);
            }

            GameplayGraphInstance* graph = nullptr;
            if (graphInstances != nullptr)
            {
                auto it = graphInstances->find(entity);
                if (it != graphInstances->end())
                {
                    graph = &it->second;
                }
            }

            detail::SyncAnimationRuntimeToGameplayState(*animState, graph, skinnedItem->controller);
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
                
                GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
                GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity);

                ApplyAnimationNotifyToGameplayState(*notifyState, action, movementState, motor, event);

                std::vector<std::string> gameplayEventIds{};
                CollectGameplayEventIdsForAnimationEvent(skinnedItem->controller.stateMachineAsset, event, gameplayEventIds);
                for (const std::string& gameplayEventId : gameplayEventIds)
                {
                    ApplyGameplayEventToGameplayState(*notifyState, action, movementState, motor, gameplayEventId, event);
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
