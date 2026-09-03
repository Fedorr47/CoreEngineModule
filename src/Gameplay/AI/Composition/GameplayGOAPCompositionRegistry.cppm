module;

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <any>
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
        [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanOutputs() const = 0;
        [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerOutputs() const { return {}; }
        [[nodiscard]] virtual std::vector<AIWorldFactId> BooleanInputs() const { return {}; }
        [[nodiscard]] virtual std::vector<AIWorldIntegerFactId> IntegerInputs() const { return {}; }
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
        [[nodiscard]] virtual std::vector<GameplayGOAPActionCostOverride> CostOverrides() const { return {}; }
        [[nodiscard]] virtual std::unique_ptr<IAIActionBinding> CreateBinding(
            AIAgentWorldState& facts, std::vector<GameplayWorldEvent>& events) = 0;
    };

    struct GameplayGOAPCompositionContext
    {
        const GameplayAIDecisionCreationContext& services;
        const GameplayGOAPCompiledDefinition& definition;
        std::span<const GameplayAIResolvedRole> roles;
        std::string_view source;
        std::span<const GameplayAIObservationAsset> observations{};
        const GameplayAIRouteGraphAsset* routeGraph{};
        const GameplayAILevelBindingsAsset* levelBindings{};

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

        [[nodiscard]] AIWorldIntegerFactId IntegerFact(std::string_view name) const
        {
            const auto fact = definition.FindIntegerFact(name);
            if (!fact)
            {
                Fail(name, "unknown fact or expected int");
            }
            return *fact;
        }
        template<class T, class TAsset>
        [[nodiscard]] const T& Parameters(const TAsset& asset) const
        {
            const auto* parameters = std::any_cast<T>(&asset.parameters);
            if (parameters == nullptr)
            {
                Fail(asset.type, "incorrect typed parameters");
            }
            return *parameters;
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

    using GameplayGOAPReactionCompiler = std::function<
        std::unique_ptr<IGameplayGOAPEventReaction>(const GameplayAIReactionAsset&,
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
        template<class T>
        [[nodiscard]] bool RegisterObservation(std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPObservationCompiler compiler)
        {
            if (type.empty() || !parse || !compiler || observations_.contains(type))
            {
                return false;
            }
            auto parsers = parsers_;
            if (!parsers.Register<T>(GameplayAIComponentKind::Observation, type, std::move(parse)))
            {
                return false;
            }
            observations_.emplace(std::string(type), std::move(compiler));
            parsers_ = std::move(parsers);
            return true;
        }

        template<class T>
        [[nodiscard]] bool RegisterReaction(std::string_view type,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPReactionCompiler compiler)
        {
            if (type.empty() || !parse || !compiler || reactions_.contains(type))
            {
                return false;
            }
            auto parsers = parsers_;
            if (!parsers.Register<T>(GameplayAIComponentKind::Reaction, type, std::move(parse)))
            {
                return false;
            }
            reactions_.emplace(std::string(type), std::move(compiler));
            parsers_ = std::move(parsers);
            return true;
        }

        [[nodiscard]] const GameplayGOAPReactionCompiler* Reaction(std::string_view type) const
        {
            const auto found = reactions_.find(type);
            return found == reactions_.end() ? nullptr : &found->second;
        }

        template<class T>
        [[nodiscard]] bool RegisterCapability(std::string_view type, AIActionId actionId,
            std::function<T(const GameplayAIComponentParseContext&)> parse,
            GameplayGOAPCapabilityCompiler compiler)
        {
            if (type.empty() || !actionId.IsValid() || !parse || !compiler || capabilities_.contains(type))
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
            auto parsers = parsers_;
            if (!parsers.Register<T>(GameplayAIComponentKind::Capability, type, std::move(parse)))
            {
                return false;
            }
            capabilities_.emplace(std::string(type),
                GameplayGOAPCapabilityRegistration{actionId, std::move(compiler)});
            parsers_ = std::move(parsers);
            return true;
        }

        [[nodiscard]] const GameplayAIComponentParsers& AssetParsers() const noexcept { return parsers_; }

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
        GameplayAIComponentParsers parsers_;
        std::map<std::string, GameplayGOAPObservationCompiler, std::less<>> observations_;
        std::map<std::string, GameplayGOAPReactionCompiler, std::less<>> reactions_;
        std::map<std::string, GameplayGOAPCapabilityRegistration, std::less<>> capabilities_;
    };
}
