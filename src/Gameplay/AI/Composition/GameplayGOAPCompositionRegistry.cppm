module;

#include <functional>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_goap_composition_registry;

export import :gameplay_ai_decision_asset;
export import :gameplay_goap_decision_setup;
export import :gameplay_goap_definition_compiler;
export import :gameplay_ai_decision_creation_context;
import :math_utils;
import :ai_action_binding;

export namespace rendern
{
    struct GameplayAIResolvedRole
    {
        std::string role;
        EntityHandle entity{kNullEntity};
        mathUtils::Vec3 position{};
    };

    // Observers and capability providers belong to a single decision. Compile inputs
    // are borrowed only during construction; providers copy configuration they need.
    class IGameplayGOAPObservation
    {
    public:
        virtual ~IGameplayGOAPObservation() = default;
        virtual void Observe(const GameplayWorld& world, EntityHandle agent,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) = 0;

        virtual void ObserveActionEvents(const GameplayWorld&, EntityHandle,
            std::span<const GameplayWorldEvent>, AIAgentWorldState&)
        {
        }
    };

    class IGameplayGOAPCapability
    {
    public:
        virtual ~IGameplayGOAPCapability() = default;
        [[nodiscard]] virtual std::unique_ptr<IAIActionBinding> CreateBinding(
            AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& events) = 0;
    };

    struct GameplayGOAPCompositionContext
    {
        const GameplayAIDecisionCreationContext& services;
        const GameplayGOAPCompiledDefinition& definition;
        std::span<const GameplayAIResolvedRole> roles;
        std::string_view source;

        [[noreturn]] void Fail(std::string_view component, std::string_view reason) const
        {
            throw std::runtime_error("AI behavior '" + std::string(source) + "', "
                + std::string(component) + ": " + std::string(reason));
        }

        [[nodiscard]] const GameplayAIResolvedRole& Role(std::string_view name) const
        {
            for (const auto& role : roles)
            {
                if (role.role == name)
                {
                    return role;
                }
            }
            Fail("role '" + std::string(name) + "'", "no level binding");
        }

        [[nodiscard]] AIWorldFactId BooleanFact(std::string_view name) const
        {
            const auto fact = definition.FindBooleanFact(name);
            if (!fact)
            {
                Fail("fact '" + std::string(name) + "'", "unknown fact or expected bool");
            }
            return *fact;
        }
    };

    using GameplayGOAPObservationCompiler = std::function<
        std::unique_ptr<IGameplayGOAPObservation>(const GameplayAIObservationAsset&,
            const GameplayGOAPCompositionContext&)>;
    using GameplayGOAPCapabilityCompiler = std::function<
        std::unique_ptr<IGameplayGOAPCapability>(std::span<const GameplayAICapabilityAsset>,
            const GameplayGOAPCompositionContext&)>;

    struct GameplayGOAPCapabilityRegistration
    {
        AIActionId actionId;
        GameplayGOAPCapabilityCompiler compile;
    };

    // C++ registration describes reusable semantics, never scenario identity.
    // One provider owns all authored contexts of a semantic action.
    class GameplayGOAPCompositionRegistry
    {
    public:
        [[nodiscard]] bool RegisterObservation(std::string_view type,
            GameplayGOAPObservationCompiler compiler)
        {
            if (type.empty() || !compiler)
            {
                return false;
            }
            return observations_.emplace(std::string(type), std::move(compiler)).second;
        }

        [[nodiscard]] bool RegisterCapability(std::string_view type, AIActionId actionId,
            GameplayGOAPCapabilityCompiler compiler)
        {
            if (type.empty() || !actionId.IsValid() || !compiler)
            {
                return false;
            }
            for (const auto& [name, entry] : capabilities_)
            {
                if (entry.actionId == actionId)
                {
                    return false;
                }
            }
            return capabilities_.emplace(std::string(type),
                GameplayGOAPCapabilityRegistration{actionId, std::move(compiler)}).second;
        }

        [[nodiscard]] const GameplayGOAPObservationCompiler* Observation(std::string_view type) const
        {
            const auto found = observations_.find(type);
            return found == observations_.end() ? nullptr : &found->second;
        }

        [[nodiscard]] const GameplayGOAPCapabilityRegistration* Capability(std::string_view type) const
        {
            const auto found = capabilities_.find(type);
            return found == capabilities_.end() ? nullptr : &found->second;
        }

        [[nodiscard]] std::vector<GameplayGOAPSemanticAction> SemanticActions() const
        {
            std::vector<GameplayGOAPSemanticAction> result;
            for (const auto& [name, entry] : capabilities_)
            {
                result.push_back({name, entry.actionId});
            }
            return result;
        }

    private:
        std::map<std::string, GameplayGOAPObservationCompiler, std::less<>> observations_;
        std::map<std::string, GameplayGOAPCapabilityRegistration, std::less<>> capabilities_;
    };
}
