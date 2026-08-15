module;

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <initializer_list>

export module core:gameplay_animation_bridge;

import :gameplay;
import :animation_controller;

export namespace rendern
{
    namespace detail
    {
        [[nodiscard]] inline std::string CanonicalizeAnimationNameToken_(std::string_view name)
        {
            std::string canonical;
            canonical.reserve(name.size());
            for (const unsigned char ch : name)
            {
                if (std::isalnum(ch))
                {
                    canonical.push_back(static_cast<char>(std::tolower(ch)));
                }
            }
            return canonical;
        }

        [[nodiscard]] inline bool NameMatchesAnyAlias_(
            const std::string_view candidate,
            const std::initializer_list<std::string_view> aliases)
        {
            const std::string canonicalCandidate = CanonicalizeAnimationNameToken_(candidate);
            for (const std::string_view alias : aliases)
            {
                if (canonicalCandidate == CanonicalizeAnimationNameToken_(alias))
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] inline const AnimationParameterDesc* FindAnimationParameterByAliases_(
            const AnimationControllerAsset& asset,
            const AnimationParameterType expectedType,
            const std::initializer_list<std::string_view> aliases)
        {
            for (const AnimationParameterDesc& desc : asset.parameters)
            {
                if (desc.defaultValue.type != expectedType)
                {
                    continue;
                }
                if (NameMatchesAnyAlias_(desc.name, aliases))
                {
                    return &desc;
                }
            }
            return nullptr;
        }

        inline void SetAnimationFloatParameterByAliases_(
            AnimationControllerRuntime& controller,
            const std::initializer_list<std::string_view> aliases,
            const float value)
        {
            if (controller.stateMachineAsset == nullptr)
            {
                return;
            }

            if (const AnimationParameterDesc* desc = FindAnimationParameterByAliases_(
                *controller.stateMachineAsset,
                AnimationParameterType::Float,
                aliases))
            {
                SetAnimationParameter(controller.parameters, desc->name, value);
            }
        }

        inline void SetAnimationBoolParameterByAliases_(
            AnimationControllerRuntime& controller,
            const std::initializer_list<std::string_view> aliases,
            const bool value)
        {
            if (controller.stateMachineAsset == nullptr)
            {
                return;
            }

            if (const AnimationParameterDesc* desc = FindAnimationParameterByAliases_(
                *controller.stateMachineAsset,
                AnimationParameterType::Bool,
                aliases))
            {
                SetAnimationParameter(controller.parameters, desc->name, value);
            }
        }

        inline void SetAnimationIntParameterByAliases_(
            AnimationControllerRuntime& controller,
            const std::initializer_list<std::string_view> aliases,
            const int value)
        {
            if (controller.stateMachineAsset == nullptr)
            {
                return;
            }

            if (const AnimationParameterDesc* desc = FindAnimationParameterByAliases_(
                *controller.stateMachineAsset,
                AnimationParameterType::Int,
                aliases))
            {
                SetAnimationParameter(controller.parameters, desc->name, value);
            }
        }

        inline void SetAnimationNumericParameterByAliases_(
            AnimationControllerRuntime& controller,
            const std::initializer_list<std::string_view> aliases,
            const int value)
        {
            SetAnimationIntParameterByAliases_(controller, aliases, value);
            SetAnimationFloatParameterByAliases_(controller, aliases, static_cast<float>(value));
        }

        inline void FireAnimationTriggerByAliases_(
            AnimationControllerRuntime& controller,
            const std::initializer_list<std::string_view> aliases)
        {
            if (controller.stateMachineAsset == nullptr)
            {
                return;
            }

            if (const AnimationParameterDesc* desc = FindAnimationParameterByAliases_(
                *controller.stateMachineAsset,
                AnimationParameterType::Trigger,
                aliases))
            {
                FireAnimationTrigger(controller.parameters, desc->name);
            }
        }

        [[nodiscard]] inline GameplayActionId InferGameplayActionIdFromNotifyId_(const std::string_view notifyId) noexcept
        {
            if (NameMatchesAnyAlias_(
                notifyId,
                { "LightAttackBegin", "LightAttackStart", "AttackBegin", "AttackStart", "Attack" }))
            {
                return kGameplayActionLightAttack;
            }

            if (NameMatchesAnyAlias_(
                notifyId,
                { "InteractBegin", "InteractStart", "UseBegin", "UseStart", "Use" }))
            {
                return kGameplayActionInteract;
            }

            if (NameMatchesAnyAlias_(
                notifyId,
                { "JumpBegin", "JumpStart", "Jump" }))
            {
                return kGameplayActionJump;
            }

            return GameplayActionId{};
        }
    }

    inline void ResetGameplayAnimationNotifyFrame(GameplayAnimationNotifyStateComponent& notifyState)
    {
        notifyState.anyThisFrame = false;
        notifyState.footstepThisFrame = false;
        notifyState.interactionPointThisFrame = false;
        notifyState.actionStartedThisFrame = false;
        notifyState.actionFinishedThisFrame = false;
        notifyState.jumpTakeoffThisFrame = false;
        notifyState.hitWindowOpenedThisFrame = false;
        notifyState.hitWindowClosedThisFrame = false;
    }

    inline void ApplyGameplayEventToGameplayState(
        GameplayAnimationNotifyStateComponent& notifyState,
        GameplayActionComponent* action,
        GameplayCharacterMovementStateComponent*,
        GameplayCharacterMotorComponent*,
        std::string_view gameplayEventId,
        const AnimationNotifyEvent& sourceEvent)
    {
        notifyState.anyThisFrame = true;
        notifyState.lastSequence = sourceEvent.sequence;
        notifyState.lastNormalizedTime = sourceEvent.normalizedTime;
        notifyState.lastNotifyId = std::string(gameplayEventId);
        notifyState.lastStateName = sourceEvent.stateName;
        notifyState.lastClipName = sourceEvent.clipName;

        if (detail::NameMatchesAnyAlias_(gameplayEventId, { "Footstep", "Step", "FootStep", "CharacterFootstep" }))
        {
            notifyState.footstepThisFrame = true;
        }

        if (detail::NameMatchesAnyAlias_(gameplayEventId, { "Interact", "Interaction", "InteractionPoint", "UsePoint", "InteractionEvent" }))
        {
            notifyState.interactionPointThisFrame = true;
        }
        
        if (detail::NameMatchesAnyAlias_(gameplayEventId, { "JumpTakeoff" }))
        {
            notifyState.jumpTakeoffThisFrame = true;
        }

        if (detail::NameMatchesAnyAlias_(
            gameplayEventId,
            { "HitWindowBegin", "HitWindowStart", "AttackWindowBegin", "AttackWindowStart", "HitEnable", "EnableHit", "DamageWindowBegin", "DamageOn", "CombatHitWindowOpen" }))
        {
            notifyState.hitWindowOpenedThisFrame = true;
            notifyState.hitWindowActive = true;
        }

        if (detail::NameMatchesAnyAlias_(
            gameplayEventId,
            { "HitWindowEnd", "HitWindowStop", "AttackWindowEnd", "AttackWindowStop", "HitDisable", "DisableHit", "DamageWindowEnd", "DamageOff", "CombatHitWindowClose" }))
        {
            notifyState.hitWindowClosedThisFrame = true;
            notifyState.hitWindowActive = false;
        }

        if (detail::NameMatchesAnyAlias_(
            gameplayEventId,
            { "ActionBegin", "ActionStart", "AttackBegin", "AttackStart", "LightAttackBegin", "LightAttackStart", "InteractBegin", "InteractStart", "JumpBegin", "JumpStart", "GameplayActionBegin" }))
        {
            notifyState.actionStartedThisFrame = true;
            if (action != nullptr)
            {
                GameplayActionId startedId = GetGameplayRequestedActionId(*action);
                if (!startedId.IsValid())
                {
                    startedId = detail::InferGameplayActionIdFromNotifyId_(gameplayEventId);
                }
                CommitGameplayActionState(*action, startedId);
            }
        }

        if (detail::NameMatchesAnyAlias_(
            gameplayEventId,
            { "ActionEnd", "ActionFinish", "ActionFinished", "AttackEnd", "AttackFinish", "AttackFinished", "LightAttackEnd", "InteractEnd", "InteractFinish", "JumpEnd", "JumpFinish", "GameplayActionEnd" }))
        {
            notifyState.actionFinishedThisFrame = true;
            if (action != nullptr)
            {
                FinishGameplayActionState(*action);
            }
        }
    }
    
    inline void ApplyAnimationNotifyToGameplayState(
        GameplayAnimationNotifyStateComponent& notifyState,
        GameplayActionComponent* action,
        GameplayCharacterMovementStateComponent* movementState,
        GameplayCharacterMotorComponent* motor,
        const AnimationNotifyEvent& event)
    {
        ApplyGameplayEventToGameplayState(notifyState, action, movementState, motor, event.id, event);
    }

    // Compatibility overloads keep non-character event consumers independent
    // from character movement while character bridge systems pass full state.
    inline void ApplyGameplayEventToGameplayState(
        GameplayAnimationNotifyStateComponent& notifyState,
        GameplayActionComponent* action,
        std::string_view gameplayEventId,
        const AnimationNotifyEvent& sourceEvent)
    {
        ApplyGameplayEventToGameplayState(
            notifyState,
            action,
            nullptr,
            nullptr,
            gameplayEventId,
            sourceEvent);
    }


    inline void ApplyAnimationNotifyToGameplayState(
        GameplayAnimationNotifyStateComponent& notifyState,
        GameplayActionComponent* action,
        const AnimationNotifyEvent& event)
    {
        ApplyAnimationNotifyToGameplayState(notifyState, action, nullptr, nullptr, event);
    }

    inline void CollectGameplayEventIdsForAnimationEvent(
        const AnimationControllerAsset* asset,
        const AnimationNotifyEvent& event,
        std::vector<std::string>& outGameplayEventIds)
    {
        outGameplayEventIds.clear();
        if (asset != nullptr)
        {
            for (const AnimationEventBindingDesc& binding : asset->eventBindings)
            {
                if (binding.animationEventId.empty() || binding.gameplayEventId.empty())
                {
                    continue;
                }
                if (detail::NameMatchesAnyAlias_(event.id, { binding.animationEventId }))
                {
                    outGameplayEventIds.push_back(binding.gameplayEventId);
                }
            }
        }

        if (outGameplayEventIds.empty())
        {
            outGameplayEventIds.push_back(event.id);
        }
    }

    inline void WriteGameplayLocomotionAnimationParameters(
        AnimationControllerRuntime& controller,
        const GameplayLocomotionComponent& locomotion)
    {
        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "MoveX", "InputX", "LocomotionX", "DirectionX", "Strafe", "StrafeX" },
            locomotion.moveX);

        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "MoveY", "InputY", "LocomotionY", "DirectionY", "Forward", "ForwardY" },
            locomotion.moveY);

        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "Speed", "MoveSpeed", "PlanarSpeed", "GroundSpeed", "LocomotionSpeed" },
            locomotion.planarSpeed);

        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "ForwardSpeed", "MoveForward", "SignedForwardSpeed", "LocomotionForwardSpeed" },
            locomotion.forwardSpeed);

        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "RightSpeed", "MoveRight", "SignedRightSpeed", "LocomotionRightSpeed" },
            locomotion.rightSpeed);

        detail::SetAnimationFloatParameterByAliases_(
            controller,
            { "TurnDeltaYaw", "TurnYawDelta", "DeltaYaw", "AimYawDelta" },
            locomotion.turnDeltaYawDegrees);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "TurnInPlaceLeft", "WantsTurnLeft", "bTurnInPlaceLeft" },
            locomotion.wantsTurnInPlaceLeft);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "TurnInPlaceRight", "WantsTurnRight", "bTurnInPlaceRight" },
            locomotion.wantsTurnInPlaceRight);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsMoving", "Moving", "bIsMoving" },
            locomotion.isMoving);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsRunning", "Running", "bIsRunning" },
            locomotion.isRunning);
    }
    
    inline void WriteGameplayMovementAnimationParameters(
        AnimationControllerRuntime& controller,
        const GameplayCharacterMovementStateComponent& movementState)
    {
        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsGrounded", "Grounded", "bIsGrounded" },
            movementState.grounded);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsFalling", "Falling", "bIsFalling" },
            movementState.falling);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsJumping", "Jumping", "bIsJumping" },
            movementState.jumping);
    }

    inline void WriteGameplayActionAnimationParameters(
        AnimationControllerRuntime& controller,
        GameplayActionComponent& action,
        const GameplayActionAnimationBindings& animationBindings)
    {
        const GameplayActionId& requestedId = GetGameplayRequestedActionId(action);
        const GameplayActionId& bufferedId = GetGameplayBufferedActionId(action);
        const bool hasRequest = requestedId.IsValid();
        const bool hasBufferedRequest = bufferedId.IsValid();
        const bool requestAttack = requestedId == kGameplayActionLightAttack;
        const bool requestInteract = requestedId == kGameplayActionInteract;
        const bool requestJump = requestedId == kGameplayActionJump;
        const bool isAttacking = action.busy && action.current == kGameplayActionLightAttack;
        const bool isInteracting = action.busy && action.current == kGameplayActionInteract;

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "ActionRequested", "HasActionRequest", "WantsAction", "bActionRequested" },
            hasRequest);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "ActionBusy", "Busy", "IsBusy", "bBusy" },
            action.busy);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "HasBufferedAction", "BufferedActionRequested", "bHasBufferedAction" },
            hasBufferedRequest);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "LightAttackRequested", "AttackRequested", "WantsAttack", "RequestAttack" },
            requestAttack);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "InteractRequested", "UseRequested", "WantsInteract", "RequestInteract" },
            requestInteract);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "JumpRequested", "WantsJump", "RequestJump" },
            requestJump);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsAttacking", "Attacking", "bIsAttacking" },
            isAttacking);

        detail::SetAnimationBoolParameterByAliases_(
            controller,
            { "IsInteracting", "Interacting", "bIsInteracting" },
            isInteracting);

        if (!action.pendingDispatched && hasRequest)
        {
            if (const GameplayActionAnimationBinding* binding = FindGameplayActionAnimationBinding(animationBindings, requestedId);
                binding != nullptr && !binding->triggerParameter.empty())
            {
                FireAnimationTrigger(controller.parameters, binding->triggerParameter);
            }

            action.pendingDispatched = true;
        }
    }
}
