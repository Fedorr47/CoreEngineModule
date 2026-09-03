module;

#include <algorithm>
#include <cmath>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

export module core:gameplay_goap_asset_composition;

export import :gameplay_goap_composition_registry;
import :gameplay_ai_decision;
import :gameplay_goap_decision_instance;
import :gameplay_goap_definition_asset;
import :gameplay;
import :level;

namespace rendern::goap_asset_composition_detail
{
    [[noreturn]] void Invalid(std::string_view source, std::string_view reason)
    {
        throw std::runtime_error("AI asset '" + std::string(source) + "': " + std::string(reason));
    }

    std::vector<GameplayAIResolvedRole> ResolveRoles(const GameplayAILevelBindingsAsset& bindings,
        const GameplayAIDecisionCreationContext& context)
    {
        std::vector<GameplayAIResolvedRole> result;
        std::set<std::string> names;
        std::vector<EntityHandle> entities;
        context.world.CollectNodeLinkEntities(entities);
        for (const auto& binding : bindings.roles)
        {
            if (binding.role.empty() || !names.insert(binding.role).second)
            {
                Invalid(bindings.source, "empty or duplicate role '" + binding.role + "'");
            }
            int nodeIndex = -1;
            for (std::size_t index = 0; index < context.level.nodes.size(); ++index)
            {
                const auto& node = context.level.nodes[index];
                if (node.alive && node.name == binding.node)
                {
                    if (nodeIndex != -1)
                    {
                        Invalid(bindings.source, "ambiguous node '" + binding.node + "'");
                    }
                    nodeIndex = static_cast<int>(index);
                }
            }
            if (nodeIndex == -1)
            {
                Invalid(bindings.source, "role '" + binding.role + "': missing node '" + binding.node + "'");
            }
            const auto position = context.level.nodes[nodeIndex].transform.position;
            if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
            {
                Invalid(bindings.source, "node '" + binding.node + "': position must be finite");
            }
            EntityHandle resolved = kNullEntity;
            for (const auto entity : entities)
            {
                const auto* link = context.world.TryGetNodeLink(entity);
                if (link != nullptr && link->nodeIndex == nodeIndex)
                {
                    if (resolved != kNullEntity)
                    {
                        Invalid(bindings.source, "node '" + binding.node + "': multiple gameplay entities");
                    }
                    resolved = entity;
                }
            }
            result.push_back({binding.role, resolved, position});
        }
        return result;
    }

    class ComposedContext final : public IGameplayGOAPContext
    {
    public:
        explicit ComposedContext(EntityHandle agent) : agent_(agent) {}

        void Observe(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
            AIAgentWorldState& facts) override
        {
            for (const auto& observation : observations)
            {
                observation->Observe(world, agent_, events, facts);
            }
        }

        void ObserveActionEvents(const GameplayWorld& world,
            std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
        {
            for (const auto& observation : observations)
            {
                observation->ObserveActionEvents(world, agent_, events, facts);
            }
        }

        std::vector<std::unique_ptr<IGameplayGOAPObservation>> observations;
        std::vector<std::unique_ptr<IGameplayGOAPCapability>> capabilities;

    private:
        EntityHandle agent_;
    };
}

export namespace rendern
{
    // Validation and provider construction finish before a decision can start tasks.
    // Model orchestration remains in GameplayGOAPDecisionInstance.
    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromAssets(
        const GameplayAIDecisionCreationContext& services,
        const GameplayAIBehaviorAsset& behavior,
        const GameplayAILevelBindingsAsset& bindings,
        const GameplayGOAPDefinitionAsset& definition,
        const GameplayGOAPCompositionRegistry& registry)
    {
        using namespace goap_asset_composition_detail;
        if (behavior.model != "goap")
        {
            Invalid(behavior.source, "unknown decision model '" + behavior.model + "'");
        }
        auto compiled = CompileGameplayGOAPDefinition(definition, registry.SemanticActions());
        const auto roles = ResolveRoles(bindings, services);
        const GameplayGOAPCompositionContext context{services, compiled, roles, behavior.source};
        auto domain = std::make_unique<ComposedContext>(services.agent);
        std::set<std::string> observedFacts;
        for (const auto& asset : behavior.observations)
        {
            if (!observedFacts.insert(asset.fact).second)
            {
                context.Fail(asset.fact, "multiple observation writers");
            }
            if (!compiled.FindBooleanFact(asset.fact) && !compiled.FindIntegerFact(asset.fact))
            {
                context.Fail(asset.fact, "unknown observation fact");
            }
            const auto* compiler = registry.Observation(asset.type);
            if (compiler == nullptr)
            {
                context.Fail(asset.fact, "unknown observation type '" + asset.type + "'");
            }
            auto observer = (*compiler)(asset, context);
            if (!observer)
            {
                context.Fail(asset.type, "observation compiler returned no provider");
            }
            domain->observations.push_back(std::move(observer));
        }
        for (const auto& fact : definition.facts)
        {
            if (!observedFacts.contains(fact.name))
            {
                context.Fail(fact.name, "fact has no observation writer");
            }
        }

        std::set<std::string> boundContexts;
        std::map<std::string, std::vector<GameplayAICapabilityAsset>> groups;
        for (const auto& asset : behavior.capabilities)
        {
            const auto* registration = registry.Capability(asset.type);
            if (registration == nullptr)
            {
                context.Fail(asset.context, "unknown capability '" + asset.type + "'");
            }
            if (!boundContexts.insert(asset.context).second)
            {
                context.Fail(asset.context, "multiple capability bindings");
            }
            const auto authored = std::ranges::find_if(definition.actions, [&](const auto& action)
            {
                return action.context == asset.context;
            });
            if (authored == definition.actions.end() || authored->action != asset.type)
            {
                context.Fail(asset.context, "capability does not match a definition action/context");
            }
            groups[asset.type].push_back(asset);
        }
        for (const auto& action : definition.actions)
        {
            if (!boundContexts.contains(action.context))
            {
                context.Fail(action.context, "action context has no capability binding");
            }
        }
        GameplayGOAPDecisionSetup setup;
        for (const auto& [type, assets] : groups)
        {
            const auto& registration = *registry.Capability(type);
            auto provider = registration.compile(assets, context);
            if (!provider)
            {
                context.Fail(type, "capability compiler returned no provider");
            }
            setup.actionBindings.push_back({registration.actionId,
                [capability = provider.get()](AIAgentWorldState& facts,
                    std::vector<GameplayWorldEvent>& events)
                {
                    return capability->CreateBinding(facts, events);
                }});
            domain->capabilities.push_back(std::move(provider));
        }
        setup.definition = std::move(compiled.definition);
        setup.context = std::move(domain);
        auto decision = CreateGameplayGOAPDecision(services.agent, std::move(setup));
        if (!decision)
        {
            Invalid(behavior.source, "incomplete action bindings");
        }
        return decision;
    }

    // Registration is transactional. Each creation loads fresh content; the catalog
    // is fixed for this registry's lifetime. Captures own paths and compiler callbacks.
    void RegisterGameplayAIDecisionAssets(GameplayAIDecisionFactoryRegistry& destination,
        const GameplayAIDecisionCatalogAsset& catalog, const GameplayGOAPCompositionRegistry& components)
    {
        auto pending = destination;
        for (const auto& reference : catalog.decisions)
        {
            if (reference.behavior.empty() || reference.bindings.empty())
            {
                goap_asset_composition_detail::Invalid(catalog.source, "empty asset reference for '" + reference.id + "'");
            }
            const bool registered = pending.Register(reference.id,
                [reference, components](const GameplayAIDecisionCreationContext& context)
                    -> std::unique_ptr<GameplayAIDecisionInstance>
                {
                    try
                    {
                        const auto behavior = LoadGameplayAIBehaviorAsset(reference.behavior);
                        const auto bindings = LoadGameplayAILevelBindingsAsset(reference.bindings);
                        const auto definition = LoadGameplayGOAPDefinitionAsset(behavior.definition);
                        return CreateGameplayGOAPDecisionFromAssets(context, behavior, bindings, definition, components);
                    }
                    catch (const std::exception& error)
                    {
                        if (context.diagnostic != nullptr)
                        {
                            *context.diagnostic = "Decision '" + reference.id + "' (behavior '"
                                + reference.behavior + "', bindings '" + reference.bindings + "'): " + error.what();
                        }
                        return nullptr;
                    }
                });
            if (!registered)
            {
                goap_asset_composition_detail::Invalid(catalog.source, "empty or duplicate decision id '" + reference.id + "'");
            }
        }
        destination = std::move(pending);
    }
}
