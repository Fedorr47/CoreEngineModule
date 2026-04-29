module;

#include <array>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

export module core:gameplay;

import :math_utils;
import :scene;
import :level_ecs;
import :EnTTHelpers;

// TODO: Slice nodes to 
// node-bound gameplay entity
// pure gameplay entity
// transient gameplay entity

export namespace rendern
{
    using namespace EnTT_helpers;

    struct GameplayTransformComponent
    {
        mathUtils::Vec3 position{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 rotationDegrees{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 scale{ 1.0f, 1.0f, 1.0f };
    };

    struct GameplayNodeLinkComponent
    {
        int nodeIndex{ -1 };
        EntityHandle levelEntity{ kNullEntity };
    };

    struct GameplayAnimationLinkComponent
    {
        int skinnedDrawIndex{ -1 };
        std::string controllerAssetId{};
    };

    struct GameplayPlayerControlledComponent
    {
        bool isPrimary{ true };
    };

    struct GameplayInputIntentComponent
    {
        float moveX{ 0.0f };
        float moveY{ 0.0f };
        bool runHeld{ false };
        std::uint32_t actionIntentMask{ 0u };
    };

    struct GameplayCharacterCommandComponent
    {
        float moveInputX{ 0.0f };
        float moveInputY{ 0.0f };
        mathUtils::Vec3 moveWorld{ 0.0f, 0.0f, 0.0f };
        float moveMagnitude{ 0.0f };
        bool wantsRun{ false };
        std::uint32_t actionIntentMask{ 0u };
    };

    struct GameplayCharacterMotorComponent
    {
        mathUtils::Vec3 velocity{ 0.0f, 0.0f, 0.0f };
        mathUtils::Vec3 desiredMoveWorld{ 0.0f, 0.0f, 0.0f };
        float maxWalkSpeed{ 2.0f };
        float maxRunSpeed{ 4.5f };
        float acceleration{ 12.0f };
        float deceleration{ 16.0f };
        float backwardSpeedScale{ 0.72f };
        float airDeceleration{ 2.5f };
    };

    struct GameplayCharacterMovementStateComponent
    {
        bool grounded{ true };
        bool jumping{ false };
        bool falling{ false };
        bool jumpMovementLocked{ false };
        bool turningInPlace{ false };
        float facingYawDegrees{ 0.0f };
        float desiredFacingYawDegrees{ 0.0f };
        float previousFacingYawDegrees{ 0.0f };
        float cameraFacingYawDegrees{ 0.0f };
        mathUtils::Vec3 jumpLockedVelocity{ 0.0f, 0.0f, 0.0f };
    };

    struct GameplayFollowCameraComponent
    {
        float yawRad{ 0.0f };
        float pitchRad{ mathUtils::DegToRad(-18.0f) };
        float distance{ 3.75f };
        mathUtils::Vec3 focusOffset{ 0.0f, 1.55f, 0.0f };
        float mouseSensitivity{ 0.0025f };
        float maxPitchRad{ mathUtils::DegToRad(80.0f) };
        bool initialized{ false };
        bool consumeMouseLook{ true };
    };

    struct GameplayLocomotionComponent
    {
        float moveX{ 0.0f };
        float moveY{ 0.0f };
        float forwardSpeed{ 0.0f };
        float rightSpeed{ 0.0f };
        float planarSpeed{ 0.0f };
        float turnDeltaYawDegrees{ 0.0f };
        bool isMoving{ false };
        bool isRunning{ false };
        bool wantsTurnInPlaceLeft{ false };
        bool wantsTurnInPlaceRight{ false };
    };

    enum class GameplayActionKind : std::uint8_t
    {
        None = 0,
        LightAttack,
        Interact,
        Jump
    };

    enum class GameplayActionRequestSource : std::uint8_t
    {
        None = 0,
        Input,
        Combat,
        Interaction,
        AnimationEvent,
        Script
    };

    enum class GameplayActionPolicyGroup : std::uint8_t
    {
        None = 0,
        Input,
        Combat,
        Interaction,
        Any
    };

    enum class GameplayActionPolicyGate : std::uint32_t
    {
        None = 0u,
        RequireGrounded = 1u << 0u,
        RequireAirborne = 1u << 1u,
        RequireNotBusy = 1u << 2u,
        RequireBusy = 1u << 3u,
        RequireNoPending = 1u << 4u,
        RequireNoBuffered = 1u << 5u
    };

    struct GameplayActionRequest
    {
        GameplayActionKind kind{ GameplayActionKind::None };
        GameplayActionRequestSource source{ GameplayActionRequestSource::None };
        int priority{ 0 };
    };

    struct GameplayActionComponent
    {
        GameplayActionKind current{ GameplayActionKind::None };
        GameplayActionRequest pending{};
        GameplayActionRequest buffered{};
        bool busy{ false };
        bool pendingDispatched{ false };
    };

    struct GameplayActionPolicyEntry
    {
        GameplayActionKind intentKind{ GameplayActionKind::None };
        GameplayActionRequest request{};
        GameplayActionPolicyGroup group{ GameplayActionPolicyGroup::None };
        std::uint32_t gates{ 0u };
    };

    namespace detail
    {
        [[nodiscard]] constexpr bool GameplayActionPolicyGroupMatches_(const GameplayActionPolicyGroup requestedGroup, const GameplayActionPolicyGroup entryGroup) noexcept
        {
            if (requestedGroup == GameplayActionPolicyGroup::Any)
            {
                return entryGroup != GameplayActionPolicyGroup::None;
            }

            return requestedGroup == entryGroup;
        }

        [[nodiscard]] constexpr auto MakeGameplayActionPolicyTable_() noexcept
        {
            using Entry = GameplayActionPolicyEntry;
            using Gate = GameplayActionPolicyGate;
            using Kind = GameplayActionKind;
            using Group = GameplayActionPolicyGroup;
            using Source = GameplayActionRequestSource;

            return std::array<Entry, 3>{ {
                Entry{
                    .intentKind = Kind::Jump,
                    .request = GameplayActionRequest{
                        .kind = Kind::Jump,
                        .source = Source::Combat,
                        .priority = 200
                    },
                    .group = Group::Combat,
                    .gates = static_cast<std::uint32_t>(Gate::RequireGrounded)
                },
                Entry{
                    .intentKind = Kind::LightAttack,
                    .request = GameplayActionRequest{
                        .kind = Kind::LightAttack,
                        .source = Source::Combat,
                        .priority = 10
                    },
                    .group = Group::Combat,
                    .gates = static_cast<std::uint32_t>(Gate::RequireGrounded)
                },
                Entry{
                    .intentKind = Kind::Interact,
                    .request = GameplayActionRequest{
                        .kind = Kind::Interact,
                        .source = Source::Interaction,
                        .priority = 50
                    },
                    .group = Group::Interaction,
                    .gates = static_cast<std::uint32_t>(Gate::RequireGrounded) |
                        static_cast<std::uint32_t>(Gate::RequireNotBusy) |
                        static_cast<std::uint32_t>(Gate::RequireNoPending) |
                        static_cast<std::uint32_t>(Gate::RequireNoBuffered)
                }
            } };
        }

        // TODO: move it to data-driven asset/config
        inline constexpr auto kGameplayActionPolicyTable = MakeGameplayActionPolicyTable_();
    }

    [[nodiscard]] constexpr std::uint32_t GameplayActionIntentMask(const GameplayActionKind kind) noexcept
    {
        switch (kind)
        {
        case GameplayActionKind::LightAttack:
            return 1u << 0u;
        case GameplayActionKind::Interact:
            return 1u << 1u;
        case GameplayActionKind::Jump:
            return 1u << 2u;
        case GameplayActionKind::None:
        default:
            return 0u;
        }
    }

    [[nodiscard]] constexpr std::uint32_t GameplayActionPolicyGateMask(const GameplayActionPolicyGate gate) noexcept
    {
        return static_cast<std::uint32_t>(gate);
    }

    [[nodiscard]] constexpr bool HasGameplayActionPolicyGate(const std::uint32_t gateMask, const GameplayActionPolicyGate gate) noexcept
    {
        const std::uint32_t bit = GameplayActionPolicyGateMask(gate);
        return bit != 0u && (gateMask & bit) != 0u;
    }

    inline void AddGameplayActionIntent(std::uint32_t& intentMask, const GameplayActionKind kind) noexcept
    {
        // TODO: change it to Tag systeme instead of mask
        intentMask |= GameplayActionIntentMask(kind);
    }

    [[nodiscard]] constexpr bool HasGameplayActionIntent(const std::uint32_t intentMask, const GameplayActionKind kind) noexcept
    {
        const std::uint32_t mask = GameplayActionIntentMask(kind);
        return mask != 0u && (intentMask & mask) != 0u;
    }

    [[nodiscard]] constexpr bool HasGameplayActionRequest(const GameplayActionRequest& request) noexcept
    {
        return request.kind != GameplayActionKind::None;
    }

    inline void ClearGameplayActionRequest(GameplayActionRequest& request) noexcept
    {
        request = {};
    }

    [[nodiscard]] inline GameplayActionKind GetGameplayRequestedActionKind(const GameplayActionComponent& action) noexcept
    {
        return action.pending.kind;
    }

    [[nodiscard]] inline GameplayActionKind GetGameplayBufferedActionKind(const GameplayActionComponent& action) noexcept
    {
        return action.buffered.kind;
    }

    [[nodiscard]] inline bool HasGameplayPendingActionRequest(const GameplayActionComponent& action) noexcept
    {
        return HasGameplayActionRequest(action.pending);
    }

    [[nodiscard]] inline bool HasGameplayBufferedActionRequest(const GameplayActionComponent& action) noexcept
    {
        return HasGameplayActionRequest(action.buffered);
    }

    [[nodiscard]] inline bool EvaluateGameplayActionPolicyGates(
        const GameplayActionPolicyEntry& entry,
        const GameplayCharacterMovementStateComponent* movementState,
        const GameplayActionComponent& action) noexcept
    {
        const std::uint32_t gates = entry.gates;

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireGrounded))
        {
            if (movementState == nullptr || !movementState->grounded)
            {
                return false;
            }
        }

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireAirborne))
        {
            if (movementState == nullptr || movementState->grounded)
            {
                return false;
            }
        }

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNotBusy) && action.busy)
        {
            return false;
        }

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireBusy) && !action.busy)
        {
            return false;
        }

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNoPending) && HasGameplayPendingActionRequest(action))
        {
            return false;
        }

        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNoBuffered) && HasGameplayBufferedActionRequest(action))
        {
            return false;
        }

        return true;
    }

    inline bool QueueGameplayActionRequest(GameplayActionComponent& action, GameplayActionRequest request) noexcept;

    [[nodiscard]] inline const GameplayActionPolicyEntry* FindGameplayActionPolicy(const GameplayActionPolicyGroup group, const GameplayActionKind kind) noexcept
    {
        for (const GameplayActionPolicyEntry& entry : detail::kGameplayActionPolicyTable)
        {
            if (entry.intentKind != kind)
            {
                continue;
            }

            if (!detail::GameplayActionPolicyGroupMatches_(group, entry.group))
            {
                continue;
            }

            return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] bool QueueGameplayActionRequestsFromPolicies(
        GameplayActionComponent& action,
        const GameplayCharacterMovementStateComponent* movementState,
        const std::uint32_t intentMask,
        const GameplayActionPolicyGroup group) noexcept
    {
        bool queuedAny = false;
        for (const GameplayActionPolicyEntry& entry : detail::kGameplayActionPolicyTable)
        {
            if (!detail::GameplayActionPolicyGroupMatches_(group, entry.group))
            {
                continue;
            }

            if (!HasGameplayActionIntent(intentMask, entry.intentKind))
            {
                continue;
            }

            if (!EvaluateGameplayActionPolicyGates(entry, movementState, action))
            {
                continue;
            }

            queuedAny |= QueueGameplayActionRequest(action, entry.request);
        }

        return queuedAny;
    }

    inline bool QueueGameplayActionRequest(GameplayActionComponent& action, GameplayActionRequest request) noexcept
    {
        if (!HasGameplayActionRequest(request))
        {
            return false;
        }

        if (!HasGameplayPendingActionRequest(action))
        {
            action.pending = request;
            action.pendingDispatched = false;
            return true;
        }

        if (request.priority > action.pending.priority)
        {
            if (!action.busy)
            {
                if (!HasGameplayBufferedActionRequest(action) || action.pending.priority >= action.buffered.priority)
                {
                    action.buffered = action.pending;
                }
            }
            action.pending = request;
            action.pendingDispatched = false;
            return true;
        }

        if (!HasGameplayBufferedActionRequest(action) || request.priority >= action.buffered.priority)
        {
            action.buffered = request;
            return !action.busy;
        }

        return false;
    }

    inline void PrimeGameplayActionState(GameplayActionComponent& action, const GameplayActionKind startedKind = GameplayActionKind::None) noexcept
    {
        if (HasGameplayPendingActionRequest(action))
        {
            action.current = startedKind != GameplayActionKind::None ? startedKind : action.pending.kind;
            action.busy = true;
            return;
        }

        if (startedKind != GameplayActionKind::None)
        {
            action.current = startedKind;
            action.busy = true;
        }
    }

    inline void CommitGameplayActionState(GameplayActionComponent& action, const GameplayActionKind startedKind = GameplayActionKind::None) noexcept
    {
        if (HasGameplayPendingActionRequest(action))
        {
            action.current = startedKind != GameplayActionKind::None ? startedKind : action.pending.kind;
            action.busy = true;
            ClearGameplayActionRequest(action.pending);
            action.pendingDispatched = false;
            return;
        }

        if (startedKind != GameplayActionKind::None)
        {
            action.current = startedKind;
            action.busy = true;
            action.pendingDispatched = false;
        }
    }

    inline void FinishGameplayActionState(GameplayActionComponent& action) noexcept
    {
        action.busy = false;
        action.current = GameplayActionKind::None;
        action.pendingDispatched = false;

        if (!HasGameplayPendingActionRequest(action) && HasGameplayBufferedActionRequest(action))
        {
            action.pending = action.buffered;
            ClearGameplayActionRequest(action.buffered);
        }
    }

    inline void ResetGameplayActionState(GameplayActionComponent& action) noexcept
    {
        action.current = GameplayActionKind::None;
        ClearGameplayActionRequest(action.pending);
        ClearGameplayActionRequest(action.buffered);
        action.busy = false;
        action.pendingDispatched = false;
    }

    struct GameplayAnimationNotifyStateComponent
    {
        bool anyThisFrame{ false };
        bool footstepThisFrame{ false };
        bool interactionPointThisFrame{ false };
        bool actionStartedThisFrame{ false };
        bool actionFinishedThisFrame{ false };
        bool hitWindowOpenedThisFrame{ false };
        bool hitWindowClosedThisFrame{ false };
        bool hitWindowActive{ false };
        std::uint64_t lastSequence{ 0 };
        float lastNormalizedTime{ 0.0f };
        std::string lastNotifyId{};
        std::string lastStateName{};
        std::string lastClipName{};
    };

    struct GameplayAnimationStateComponent
    {
        std::string controllerAssetId{};
        std::string currentStateName{};
        std::string previousStateName{};
        std::string modeName{};
        std::string primaryClipName{};
        std::string secondaryClipName{};
        std::string tertiaryClipName{};
        std::string blendParameterNameX{};
        std::string blendParameterNameY{};
        float blendParameterValueX{ 0.0f };
        float blendParameterValueY{ 0.0f };
        float stateNormalizedTime{ 0.0f };
        bool usesBlend1D{ false };
        bool usesBlend2D{ false };
        bool transitionActive{ false };
        bool enteredThisFrame{ false };
        bool changedThisFrame{ false };
    };

    class GameplayWorld
    {
    public:
        GameplayWorld();
        ~GameplayWorld();

        GameplayWorld(GameplayWorld&& other) noexcept;
        GameplayWorld& operator=(GameplayWorld&& other) noexcept;

        GameplayWorld(const GameplayWorld&) = delete;
        GameplayWorld& operator=(const GameplayWorld&) = delete;

        [[nodiscard]] EntityHandle CreateEntity();
        void DestroyEntity(EntityHandle entity);
        void Clear() noexcept;
        [[nodiscard]] bool IsEntityValid(EntityHandle entity) const noexcept;
        [[nodiscard]] std::size_t GetAliveCount() const noexcept;

        void AddTransform(EntityHandle entity, const GameplayTransformComponent& value);
        void SetTransform(EntityHandle entity, const GameplayTransformComponent& value);
        [[nodiscard]] GameplayTransformComponent* TryGetTransform(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayTransformComponent* TryGetTransform(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasTransform(EntityHandle entity) const noexcept;
        void RemoveTransform(EntityHandle entity);

        void AddNodeLink(EntityHandle entity, const GameplayNodeLinkComponent& value);
        void SetNodeLink(EntityHandle entity, const GameplayNodeLinkComponent& value);
        [[nodiscard]] GameplayNodeLinkComponent* TryGetNodeLink(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayNodeLinkComponent* TryGetNodeLink(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasNodeLink(EntityHandle entity) const noexcept;
        void RemoveNodeLink(EntityHandle entity);

        void AddAnimationLink(EntityHandle entity, const GameplayAnimationLinkComponent& value);
        void SetAnimationLink(EntityHandle entity, const GameplayAnimationLinkComponent& value);
        [[nodiscard]] GameplayAnimationLinkComponent* TryGetAnimationLink(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationLinkComponent* TryGetAnimationLink(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationLink(EntityHandle entity) const noexcept;
        void RemoveAnimationLink(EntityHandle entity);

        void AddPlayerControlled(EntityHandle entity, const GameplayPlayerControlledComponent& value = {});
        void SetPlayerControlled(EntityHandle entity, const GameplayPlayerControlledComponent& value);
        [[nodiscard]] GameplayPlayerControlledComponent* TryGetPlayerControlled(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayPlayerControlledComponent* TryGetPlayerControlled(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasPlayerControlled(EntityHandle entity) const noexcept;
        void RemovePlayerControlled(EntityHandle entity);

        void AddInputIntent(EntityHandle entity, const GameplayInputIntentComponent& value = {});
        void SetInputIntent(EntityHandle entity, const GameplayInputIntentComponent& value);
        [[nodiscard]] GameplayInputIntentComponent* TryGetInputIntent(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayInputIntentComponent* TryGetInputIntent(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasInputIntent(EntityHandle entity) const noexcept;
        void RemoveInputIntent(EntityHandle entity);

        void AddCharacterCommand(EntityHandle entity, const GameplayCharacterCommandComponent& value = {});
        void SetCharacterCommand(EntityHandle entity, const GameplayCharacterCommandComponent& value);
        [[nodiscard]] GameplayCharacterCommandComponent* TryGetCharacterCommand(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterCommandComponent* TryGetCharacterCommand(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterCommand(EntityHandle entity) const noexcept;
        void RemoveCharacterCommand(EntityHandle entity);

        void AddCharacterMotor(EntityHandle entity, const GameplayCharacterMotorComponent& value = {});
        void SetCharacterMotor(EntityHandle entity, const GameplayCharacterMotorComponent& value);
        [[nodiscard]] GameplayCharacterMotorComponent* TryGetCharacterMotor(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterMotorComponent* TryGetCharacterMotor(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterMotor(EntityHandle entity) const noexcept;
        void RemoveCharacterMotor(EntityHandle entity);

        void AddCharacterMovementState(EntityHandle entity, const GameplayCharacterMovementStateComponent& value = {});
        void SetCharacterMovementState(EntityHandle entity, const GameplayCharacterMovementStateComponent& value);
        [[nodiscard]] GameplayCharacterMovementStateComponent* TryGetCharacterMovementState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayCharacterMovementStateComponent* TryGetCharacterMovementState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasCharacterMovementState(EntityHandle entity) const noexcept;
        void RemoveCharacterMovementState(EntityHandle entity);

        void AddFollowCamera(EntityHandle entity, const GameplayFollowCameraComponent& value = {});
        void SetFollowCamera(EntityHandle entity, const GameplayFollowCameraComponent& value);
        [[nodiscard]] GameplayFollowCameraComponent* TryGetFollowCamera(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayFollowCameraComponent* TryGetFollowCamera(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasFollowCamera(EntityHandle entity) const noexcept;
        void RemoveFollowCamera(EntityHandle entity);

        void AddLocomotion(EntityHandle entity, const GameplayLocomotionComponent& value = {});
        void SetLocomotion(EntityHandle entity, const GameplayLocomotionComponent& value);
        [[nodiscard]] GameplayLocomotionComponent* TryGetLocomotion(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayLocomotionComponent* TryGetLocomotion(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasLocomotion(EntityHandle entity) const noexcept;
        void RemoveLocomotion(EntityHandle entity);

        void AddAction(EntityHandle entity, const GameplayActionComponent& value = {});
        void SetAction(EntityHandle entity, const GameplayActionComponent& value);
        [[nodiscard]] GameplayActionComponent* TryGetAction(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayActionComponent* TryGetAction(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAction(EntityHandle entity) const noexcept;
        void RemoveAction(EntityHandle entity);

        void AddAnimationNotifyState(EntityHandle entity, const GameplayAnimationNotifyStateComponent& value = {});
        void SetAnimationNotifyState(EntityHandle entity, const GameplayAnimationNotifyStateComponent& value);
        [[nodiscard]] GameplayAnimationNotifyStateComponent* TryGetAnimationNotifyState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationNotifyStateComponent* TryGetAnimationNotifyState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationNotifyState(EntityHandle entity) const noexcept;
        void RemoveAnimationNotifyState(EntityHandle entity);
        
        void AddAnimationState(EntityHandle entity, const GameplayAnimationStateComponent& value = {});
        void SetAnimationState(EntityHandle entity, const GameplayAnimationStateComponent& value);
        [[nodiscard]] GameplayAnimationStateComponent* TryGetAnimationState(EntityHandle entity) noexcept;
        [[nodiscard]] const GameplayAnimationStateComponent* TryGetAnimationState(EntityHandle entity) const noexcept;
        [[nodiscard]] bool HasAnimationState(EntityHandle entity) const noexcept;
        void RemoveAnimationState(EntityHandle entity);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_{};
    };

    inline void UpdateGameplayActionRequestsFromPolicies(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayActionPolicyGroup group)
    {
        for (const EntityHandle entity : entities)
        {
            const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            const GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (command == nullptr || action == nullptr)
            {
                continue;
            }

            QueueGameplayActionRequestsFromPolicies(*action, movementState, command->actionIntentMask, group);
        }
    }
}
