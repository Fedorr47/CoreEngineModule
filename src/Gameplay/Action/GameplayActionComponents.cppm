module;

#include <array>
#include <cstdint>

export module core:gameplay_action_components;

import :gameplay_character_components;

export namespace rendern
{
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
        // TODO: change it to Tag system instead of mask
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

        const GameplayActionComponent gateSnapshot = action;

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

            if (!EvaluateGameplayActionPolicyGates(entry, movementState, gateSnapshot))
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

}