module;

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

export module core:gameplay_action_components;

export namespace rendern
{
    struct GameplayActionId
    {
        std::string value{};

        GameplayActionId() = default;
        GameplayActionId(std::string text) : value(std::move(text)) {}
        GameplayActionId(const char* text) : value(text != nullptr ? text : "") {}

        [[nodiscard]] bool IsValid() const noexcept { return !value.empty(); }
        friend bool operator==(const GameplayActionId&, const GameplayActionId&) = default;
    };

    inline const GameplayActionId kGameplayActionJump{ "Movement.Jump" };
    inline const GameplayActionId kGameplayActionLightAttack{ "Combat.LightAttack" };
    inline const GameplayActionId kGameplayActionInteract{ "Interaction.Interact" };

    enum class GameplayActionRequestSource : std::uint8_t
    {
        None = 0, Input, Combat, Interaction, AnimationEvent, Script
    };

    enum class GameplayActionPolicyGroup : std::uint8_t
    {
        None = 0, Input, Combat, Interaction, Any
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

    enum class GameplayActionExecutorKind : std::uint8_t
    {
        None = 0, Jump, CombatAttack, Interact
    };

    struct GameplayActionDefinition
    {
        GameplayActionId id{};
        GameplayActionPolicyGroup group{ GameplayActionPolicyGroup::None };
        GameplayActionRequestSource source{ GameplayActionRequestSource::None };
        GameplayActionExecutorKind executor{ GameplayActionExecutorKind::None };
        int priority{ 0 };
        std::uint32_t gates{ 0u };
    };

    struct GameplayActionAnimationBinding
    {
        GameplayActionId actionId{};
        std::string triggerParameter{};
    };

    using GameplayActionDefinitions = std::vector<GameplayActionDefinition>;
    using GameplayActionAnimationBindings = std::vector<GameplayActionAnimationBinding>;
    using GameplayActionPolicyEntry = GameplayActionDefinition;

    [[nodiscard]] constexpr std::uint32_t GameplayActionPolicyGateMask(
        const GameplayActionPolicyGate gate) noexcept
    {
        return static_cast<std::uint32_t>(gate);
    }

    [[nodiscard]] constexpr bool HasGameplayActionPolicyGate(
        const std::uint32_t mask,
        const GameplayActionPolicyGate gate) noexcept
    {
        const std::uint32_t bit = GameplayActionPolicyGateMask(gate);
        return bit != 0u && (mask & bit) != 0u;
    }

    [[nodiscard]] inline GameplayActionDefinitions MakeDefaultGameplayActionDefinitions()
    {
        using Gate = GameplayActionPolicyGate;
        return {
            { kGameplayActionJump, GameplayActionPolicyGroup::Combat,
              GameplayActionRequestSource::Combat, GameplayActionExecutorKind::Jump,
              200, GameplayActionPolicyGateMask(Gate::RequireGrounded) },
            { kGameplayActionLightAttack, GameplayActionPolicyGroup::Combat,
              GameplayActionRequestSource::Combat, GameplayActionExecutorKind::CombatAttack,
              10, GameplayActionPolicyGateMask(Gate::RequireGrounded) },
            { kGameplayActionInteract, GameplayActionPolicyGroup::Interaction,
              GameplayActionRequestSource::Interaction, GameplayActionExecutorKind::Interact,
              50, GameplayActionPolicyGateMask(Gate::RequireGrounded) |
                  GameplayActionPolicyGateMask(Gate::RequireNotBusy) |
                  GameplayActionPolicyGateMask(Gate::RequireNoPending) |
                  GameplayActionPolicyGateMask(Gate::RequireNoBuffered) }
        };
    }

    [[nodiscard]] inline GameplayActionAnimationBindings MakeDefaultGameplayActionAnimationBindings()
    {
        return {
            { kGameplayActionJump, "Jump" },
            { kGameplayActionLightAttack, "LightAttack" },
            { kGameplayActionInteract, "Interact" }
        };
    }

    [[nodiscard]] inline const GameplayActionDefinition* FindGameplayActionDefinition(
        const GameplayActionDefinitions& definitions,
        const GameplayActionId& id) noexcept
    {
        const auto it = std::find_if(definitions.begin(), definitions.end(),
            [&id](const GameplayActionDefinition& definition) { return definition.id == id; });
        return it != definitions.end() ? &*it : nullptr;
    }

    [[nodiscard]] inline const GameplayActionAnimationBinding* FindGameplayActionAnimationBinding(
        const GameplayActionAnimationBindings& bindings,
        const GameplayActionId& id) noexcept
    {
        const auto it = std::find_if(bindings.begin(), bindings.end(),
            [&id](const GameplayActionAnimationBinding& binding) { return binding.actionId == id; });
        return it != bindings.end() ? &*it : nullptr;
    }

    [[nodiscard]] inline bool IsRequiredGameplayAction(const GameplayActionId& id) noexcept
    {
        return id == kGameplayActionJump;
    }

    [[nodiscard]] inline bool ValidateGameplayActionDefinitions(
        const GameplayActionDefinitions& definitions,
        std::string& diagnostic)
    {
        std::unordered_set<std::string> ids;
        for (const GameplayActionDefinition& definition : definitions)
        {
            if (!definition.id.IsValid())
            {
                diagnostic = "Gameplay action ID cannot be empty.";
                return false;
            }
            if (!ids.insert(definition.id.value).second)
            {
                diagnostic = "Duplicate gameplay action '" + definition.id.value + "'.";
                return false;
            }
            if (HasGameplayActionPolicyGate(definition.gates, GameplayActionPolicyGate::RequireGrounded) &&
                HasGameplayActionPolicyGate(definition.gates, GameplayActionPolicyGate::RequireAirborne))
            {
                diagnostic = "Gameplay action '" + definition.id.value +
                    "' cannot require both grounded and airborne.";
                return false;
            }
            if (HasGameplayActionPolicyGate(definition.gates, GameplayActionPolicyGate::RequireBusy) &&
                HasGameplayActionPolicyGate(definition.gates, GameplayActionPolicyGate::RequireNotBusy))
            {
                diagnostic = "Gameplay action '" + definition.id.value +
                    "' cannot require both busy and not busy.";
                return false;
            }
        }
        if (FindGameplayActionDefinition(definitions, kGameplayActionJump) == nullptr)
        {
            diagnostic = "Required gameplay action 'Movement.Jump' is missing.";
            return false;
        }
        diagnostic.clear();
        return true;
    }

    [[nodiscard]] inline bool ValidateGameplayActionAnimationBindings(
        const GameplayActionDefinitions& definitions,
        const GameplayActionAnimationBindings& bindings,
        std::string& diagnostic)
    {
        std::unordered_set<std::string> ids;
        for (const GameplayActionAnimationBinding& binding : bindings)
        {
            if (FindGameplayActionDefinition(definitions, binding.actionId) == nullptr)
            {
                diagnostic = "Animation binding references unknown gameplay action '" +
                    binding.actionId.value + "'.";
                return false;
            }
            if (binding.triggerParameter.empty())
            {
                diagnostic = "Animation trigger for gameplay action '" + binding.actionId.value +
                    "' cannot be empty.";
                return false;
            }
            if (!ids.insert(binding.actionId.value).second)
            {
                diagnostic = "Duplicate animation binding for gameplay action '" +
                    binding.actionId.value + "'.";
                return false;
            }
        }
        diagnostic.clear();
        return true;
    }

    [[nodiscard]] inline bool ValidateGameplayInputAction(
        const GameplayActionDefinitions& definitions,
        const GameplayActionId& id,
        std::string& diagnostic)
    {
        if (!id.IsValid() || FindGameplayActionDefinition(definitions, id) == nullptr)
        {
            diagnostic = "Input binding references unknown gameplay action '" + id.value + "'.";
            return false;
        }
        diagnostic.clear();
        return true;
    }

    inline void AddGameplayActionIntent(
        std::vector<GameplayActionId>& intents,
        const GameplayActionId& id)
    {
        if (id.IsValid() && std::find(intents.begin(), intents.end(), id) == intents.end())
        {
            intents.push_back(id);
        }
    }

    [[nodiscard]] inline bool HasGameplayActionIntent(
        const std::vector<GameplayActionId>& intents,
        const GameplayActionId& id) noexcept
    {
        return id.IsValid() && std::find(intents.begin(), intents.end(), id) != intents.end();
    }

    struct GameplayActionRequest
    {
        GameplayActionId id{};
        GameplayActionRequestSource source{ GameplayActionRequestSource::None };
        int priority{ 0 };
    };

    struct GameplayActionComponent
    {
        GameplayActionId current{};
        GameplayActionRequest pending{};
        GameplayActionRequest buffered{};
        bool busy{ false };
        bool pendingDispatched{ false };
    };

    [[nodiscard]] inline bool HasGameplayActionRequest(const GameplayActionRequest& request) noexcept
    {
        return request.id.IsValid();
    }

    inline void ClearGameplayActionRequest(GameplayActionRequest& request) noexcept { request = {}; }

    [[nodiscard]] inline const GameplayActionId& GetGameplayRequestedActionId(
        const GameplayActionComponent& action) noexcept
    {
        return action.pending.id;
    }

    [[nodiscard]] inline const GameplayActionId& GetGameplayBufferedActionId(
        const GameplayActionComponent& action) noexcept
    {
        return action.buffered.id;
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
        const auto* movementState,
        const GameplayActionComponent& action) noexcept
    {
        const std::uint32_t gates = entry.gates;
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireGrounded) &&
            (movementState == nullptr || !movementState->grounded))
        {
            return false;
        }
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireAirborne) &&
            (movementState == nullptr || movementState->grounded))
        {
            return false;
        }
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNotBusy) && action.busy)
        {
            return false;
        }
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireBusy) && !action.busy)
        {
            return false;
        }
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNoPending) &&
            HasGameplayPendingActionRequest(action))
        {
            return false;
        }
        if (HasGameplayActionPolicyGate(gates, GameplayActionPolicyGate::RequireNoBuffered) &&
            HasGameplayBufferedActionRequest(action))
        {
            return false;
        }
        return true;
    }

    [[nodiscard]] inline const GameplayActionPolicyEntry* FindGameplayActionPolicy(
        const GameplayActionDefinitions& definitions,
        const GameplayActionPolicyGroup group,
        const GameplayActionId& id) noexcept
    {
        const GameplayActionDefinition* definition = FindGameplayActionDefinition(definitions, id);
        if (definition == nullptr)
        {
            return nullptr;
        }
        const bool groupMatches = group == GameplayActionPolicyGroup::Any
            ? definition->group != GameplayActionPolicyGroup::None
            : definition->group == group;
        return groupMatches ? definition : nullptr;
    }

    inline bool QueueGameplayActionRequest(GameplayActionComponent& action, GameplayActionRequest request) noexcept;

    [[nodiscard]] inline bool QueueGameplayActionRequestsFromPolicies(
        GameplayActionComponent& action,
        const auto* movementState,
        const std::vector<GameplayActionId>& intents,
        const GameplayActionPolicyGroup group,
        const GameplayActionDefinitions& definitions) noexcept
    {
        bool queuedAny = false;
        const GameplayActionComponent gateSnapshot = action;
        for (const GameplayActionId& intent : intents)
        {
            const GameplayActionDefinition* entry = FindGameplayActionPolicy(definitions, group, intent);
            if (entry == nullptr ||
                !EvaluateGameplayActionPolicyGates(*entry, movementState, gateSnapshot))
            {
                continue;
            }
            queuedAny |= QueueGameplayActionRequest(action, { entry->id, entry->source, entry->priority });
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
            action.pending = std::move(request);
            action.pendingDispatched = false;
            return true;
        }
        if (request.priority > action.pending.priority)
        {
            if (!action.busy && (!HasGameplayBufferedActionRequest(action) ||
                action.pending.priority >= action.buffered.priority)) action.buffered = action.pending;
            action.pending = std::move(request);
            action.pendingDispatched = false;
            return true;
        }
        if (!HasGameplayBufferedActionRequest(action) || request.priority >= action.buffered.priority)
        {
            action.buffered = std::move(request);
            return !action.busy;
        }
        return false;
    }

    inline void PrimeGameplayActionState(GameplayActionComponent& action, const GameplayActionId& startedId = {}) noexcept
    {
        if (HasGameplayPendingActionRequest(action))
        {
            action.current = startedId.IsValid() ? startedId : action.pending.id;
            action.busy = true;
        }
        else if (startedId.IsValid())
        {
            action.current = startedId;
            action.busy = true;
        }
    }

    inline void CommitGameplayActionState(GameplayActionComponent& action, const GameplayActionId& startedId = {}) noexcept
    {
        if (HasGameplayPendingActionRequest(action))
        {
            action.current = startedId.IsValid() ? startedId : action.pending.id;
            action.busy = true;
            ClearGameplayActionRequest(action.pending);
            action.pendingDispatched = false;
        }
        else if (startedId.IsValid())
        {
            action.current = startedId;
            action.busy = true;
            action.pendingDispatched = false;
        }
    }

    inline void FinishGameplayActionState(GameplayActionComponent& action) noexcept
    {
        action.busy = false;
        action.current = {};
        action.pendingDispatched = false;
        if (!HasGameplayPendingActionRequest(action) && HasGameplayBufferedActionRequest(action))
        {
            action.pending = std::move(action.buffered);
            ClearGameplayActionRequest(action.buffered);
        }
    }

    inline void ResetGameplayActionState(GameplayActionComponent& action) noexcept
    {
        action = {};
    }
}
