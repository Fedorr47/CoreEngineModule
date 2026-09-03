module;

#include <algorithm>
#include <optional>
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

export import :gameplay_goap_decision_template;
export import :gameplay_goap_observation_order;
import :gameplay_ai_decision;
import :gameplay_goap_decision_instance;
import :gameplay_goap_definition_asset;
import :gameplay;
import :level;
import :gameplay_goap_path_inspection;

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

    class ComposedContext : public IGameplayGOAPContext
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
    class InspectedContext final : public ComposedContext, public IGameplayGOAPPlannedPathProvider
    {
    public:
        using ComposedContext::ComposedContext;
        std::vector<AIActionId> actionIds;
        GameplayAIDebugPlannedPathView BuildPlannedPath(std::span<const AIPlanStep> plan,
            std::optional<std::size_t> current) const override
        {
            GameplayAIDebugPlannedPathView result;
            for (std::size_t step = current.value_or(plan.size()); step < plan.size(); ++step)
            {
                for (std::size_t index = 0; index < capabilities.size(); ++index)
                {
                    const auto* paths = dynamic_cast<const IGameplayGOAPActionPathProvider*>(capabilities[index].get());
                    if (actionIds[index] == plan[step].actionId && paths != nullptr)
                    {
                        auto route = paths->BuildDebugRoute(plan[step].contextId);
                        result.complete = result.complete && route.has_value();
                        result.routeSteps.push_back({step, plan[step].actionId, plan[step].contextId, std::move(route)});
                    }
                }
            }
            return result;
        }
    };

}

export namespace rendern
{
    // Validation and provider construction finish before a decision can start tasks.
    // Model orchestration remains in GameplayGOAPDecisionInstance.
    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromTemplate(
        const GameplayAIDecisionCreationContext& services, const GameplayGOAPDecisionTemplate& prepared)
    {
        using namespace goap_asset_composition_detail;
        const auto& behavior = prepared.Behavior();
        const auto& bindings = prepared.Bindings();
        const auto& registry = prepared.Components();
        const auto& compiled = prepared.Compiled();
        const auto* routeGraph = prepared.RouteGraph();
        const auto roles = ResolveRoles(bindings, services);
        const GameplayGOAPCompositionContext context{services, compiled, roles, behavior.source,
            behavior.observations, routeGraph, &bindings};
        std::unique_ptr<ComposedContext> domain;
        if (behavior.inspectPath)
        {
            domain = std::make_unique<InspectedContext>(services.agent);
        }
        else
        {
            domain = std::make_unique<ComposedContext>(services.agent);
        }
        std::vector<std::unique_ptr<IGameplayGOAPObservation>> pendingObservers;
        for (const auto& asset : behavior.observations)
        {
            const auto* compiler = registry.Observation(asset.type);
            if (compiler == nullptr)
            {
                context.Fail(asset.type, "unknown observation type '" + asset.type + "'");
            }
            auto observer = (*compiler)(asset, context);
            if (!observer)
            {
                context.Fail(asset.type, "observation compiler returned no provider");
            }
            pendingObservers.push_back(std::move(observer));
        }
        domain->observations = OrderGameplayGOAPObservations(std::move(pendingObservers), compiled, behavior.source);

        GameplayGOAPDecisionSetup setup;
        for (const auto& asset : behavior.reactions)
        {
            const auto* compiler = registry.Reaction(asset.type);
            if (compiler == nullptr)
            {
                context.Fail(asset.type, "unknown reaction type");
            }
            auto reaction = (*compiler)(asset, context);
            if (!reaction)
            {
                context.Fail(asset.type, "reaction compiler returned no provider");
            }
            setup.eventReactions.push_back(std::move(reaction));
        }
        std::vector<GameplayGOAPActionCostOverride> costOverrides;
        for (const auto& group : prepared.Capabilities())
        {
            const auto& type = group.type;
            const auto& assets = group.assets;
            const auto& registration = group.registration;
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
            const auto overrides = provider->CostOverrides();
            costOverrides.insert(costOverrides.end(), overrides.begin(), overrides.end());
            if (auto* inspected = dynamic_cast<InspectedContext*>(domain.get()))
            {
                inspected->actionIds.push_back(registration.actionId);
            }
            domain->capabilities.push_back(std::move(provider));
        }
        setup.definition = prepared.InstanceDefinition(costOverrides);
        setup.context = std::move(domain);
        auto decision = CreateGameplayGOAPDecision(services.agent, std::move(setup));
        if (!decision)
        {
            Invalid(behavior.source, "incomplete action bindings");
        }
        return decision;
    }

    // One-shot composition for callers editing authored data directly. Catalog
    // factories prepare once instead and use CreateGameplayGOAPDecisionFromTemplate.
    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayGOAPDecisionFromAssets(
        const GameplayAIDecisionCreationContext& services, const GameplayAIBehaviorAsset& behavior,
        const GameplayAILevelBindingsAsset& bindings, const GameplayGOAPDefinitionAsset& definition,
        const GameplayGOAPCompositionRegistry& registry, const GameplayAIRouteGraphAsset* graph = nullptr)
    {
        const GameplayGOAPDecisionTemplate prepared{behavior, bindings, definition, registry,
            graph ? std::optional{*graph} : std::nullopt};
        return CreateGameplayGOAPDecisionFromTemplate(services, prepared);
    }

    // Registration prepares a complete immutable snapshot before publishing any
    // factory. Copies of the factory registry share templates, never agent state.
    // Rebuild the registry to load asset edits; active decisions remain independent.
    void RegisterGameplayAIDecisionAssets(GameplayAIDecisionFactoryRegistry& destination,
        const GameplayAIDecisionCatalogAsset& catalog, const GameplayGOAPCompositionRegistry& components)
    {
        std::set<std::string> ids;
        for (const auto& reference : catalog.decisions)
        {
            if (reference.id.empty() || destination.Contains(reference.id) || !ids.insert(reference.id).second)
            {
                goap_asset_composition_detail::Invalid(catalog.source, "empty or duplicate decision id '" + reference.id + "'");
            }
            if (reference.behavior.empty() || reference.bindings.empty())
            {
                goap_asset_composition_detail::Invalid(catalog.source, "empty asset reference for '" + reference.id + "'");
            }
        }
        auto pending = destination;
        for (const auto& reference : catalog.decisions)
        {
            const auto diagnosticPrefix = "Decision '" + reference.id + "' (behavior '"
                + reference.behavior + "', bindings '" + reference.bindings + "'): ";
            std::shared_ptr<const GameplayGOAPDecisionTemplate> prepared;
            try
            {
                auto behavior = LoadGameplayAIBehaviorAsset(reference.behavior, components.AssetParsers());
                auto bindings = LoadGameplayAILevelBindingsAsset(reference.bindings);
                const auto definition = LoadGameplayGOAPDefinitionAsset(behavior.definition);
                auto graph = behavior.routeGraph.empty() ? std::optional<GameplayAIRouteGraphAsset>{}
                    : std::optional{LoadGameplayAIRouteGraphAsset(behavior.routeGraph)};
                prepared = std::make_shared<const GameplayGOAPDecisionTemplate>(std::move(behavior),
                    std::move(bindings), definition, components, std::move(graph));
            }
            catch (const std::exception& error)
            {
                goap_asset_composition_detail::Invalid(catalog.source, diagnosticPrefix + error.what());
            }
            const bool registered = pending.Register(reference.id,
                [prepared = std::move(prepared), diagnosticPrefix](const GameplayAIDecisionCreationContext& context)
                    -> std::unique_ptr<GameplayAIDecisionInstance>
                {
                    try
                    {
                        return CreateGameplayGOAPDecisionFromTemplate(context, *prepared);
                    }
                    catch (const std::exception& error)
                    {
                        if (context.diagnostic != nullptr)
                        {
                            *context.diagnostic = diagnosticPrefix + error.what();
                        }
                        return nullptr;
                    }
                });
            if (!registered)
            {
                goap_asset_composition_detail::Invalid(catalog.source, "duplicate decision id '" + reference.id + "'");
            }
        }
        destination = std::move(pending);
    }
}
