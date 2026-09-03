#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <any>

// Include after import core. This fixture exercises production asset compilers.
namespace goap_asset_test
{
    using namespace rendern;

    struct Fixture
    {
        GameplayWorld world;
        LevelAsset level;
        GameplayTraversalLinkRegistry links;
        GameplayTraversalExecutorRegistry executors;
        GameplayObjectReservationSystem reservations;
        EntityHandle agent{world.CreateEntity()};
        GameplayAIDecisionCreationContext services{agent, level, world, links, executors, reservations};
        GameplayGOAPCompositionRegistry components{MakeDefaultGameplayGOAPComponents()};
        GameplayAIBehaviorAsset behavior;
        GameplayAILevelBindingsAsset bindings;
        GameplayGOAPDefinitionAsset definition;
        GameplayAIRouteGraphAsset graph;
        GameplayGOAPCompiledDefinition compiled;
        std::vector<GameplayAIResolvedRole> roles;
        AISystem ai;

        explicit Fixture(std::string_view id = "access_key")
        {
            const auto catalog = LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json");
            const auto reference = std::ranges::find(catalog.decisions, id, &GameplayAIDecisionAssetReference::id);
            if (reference == catalog.decisions.end())
            {
                throw std::runtime_error("Missing fixture catalog entry");
            }
            behavior = LoadGameplayAIBehaviorAsset(reference->behavior, components.AssetParsers());
            bindings = LoadGameplayAILevelBindingsAsset(reference->bindings);
            definition = LoadGameplayGOAPDefinitionAsset(behavior.definition);
            graph = LoadGameplayAIRouteGraphAsset(behavior.routeGraph);
            const std::map<std::string, mathUtils::Vec3> positions{
                {"start", {0, 0, 0}}, {"coinA", {-6, .35f, -3}}, {"coinB", {6, .35f, 1}},
                {"coinC", {-5, .35f, 6}}, {"shop", {0, .35f, -7}}, {"goal", {0, .08f, 13.5f}},
                {"takeoff", {0, .03f, 8.7f}}, {"landing", {0, .03f, 10.8f}}};
            for (const auto& binding : bindings.roles)
            {
                LevelNode node{};
                node.name = binding.node;
                node.transform.position = positions.at(binding.role);
                const auto entity = world.CreateEntity();
                world.AddNodeLink(entity, {.nodeIndex = static_cast<int>(level.nodes.size())});
                world.AddTransform(entity, {.position = node.transform.position});
                world.AddInteractionPoint(entity, {});
                roles.push_back({binding.role, entity, node.transform.position});
                level.nodes.push_back(node);
            }
            for (const auto& observation : behavior.observations)
            {
                if (const auto* ledger = std::any_cast<GameplayAIResourceLedgerAsset>(&observation.parameters))
                {
                    for (const auto& pickup : ledger->pickups)
                    {
                        world.AddPickup(Role(pickup.target).entity);
                    }
                }
            }
            world.AddAI(agent);
            world.AddTransform(agent, {.position = Role("start").position});
            world.AddCharacterCommand(agent, {});
            world.AddCharacterMotor(agent, {});
            world.AddCharacterMovementState(agent, {});
            Refresh();
        }
        const GameplayAIResolvedRole& Role(std::string_view name) const
        {
            const auto found = std::ranges::find(roles, name, &GameplayAIResolvedRole::role);
            if (found == roles.end())
            {
                throw std::runtime_error("Missing fixture role");
            }
            return *found;
        }
        void Refresh()
        {
            compiled = CompileGameplayGOAPDefinition(definition, components.SemanticActions());
        }
        GameplayGOAPCompositionContext Context() const
        {
            return {services, compiled, roles, behavior.source, behavior.observations, &graph, &bindings};
        }
        std::unique_ptr<IGameplayGOAPCapability> Capability(std::string_view type)
        {
            std::vector<GameplayAICapabilityAsset> assets;
            for (const auto& asset : behavior.capabilities)
            {
                if (asset.type == type)
                {
                    assets.push_back(asset);
                }
            }
            return components.Capability(type)->compile(assets, Context());
        }
        std::unique_ptr<GameplayAIDecisionInstance> Create()
        {
            return CreateGameplayGOAPDecisionFromAssets(services, behavior, bindings, definition, components, &graph);
        }
        std::vector<std::unique_ptr<IGameplayGOAPObservation>> Observers()
        {
            std::vector<std::unique_ptr<IGameplayGOAPObservation>> result;
            for (const auto& asset : behavior.observations)
            {
                result.push_back((*components.Observation(asset.type))(asset, Context()));
            }
            return result;
        }
        GameplayGOAPCompiledDefinition WithRouteCosts()
        {
            auto movement = Capability("move_to");
            return CompileGameplayGOAPDefinition(definition, components.SemanticActions(), movement->CostOverrides());
        }
    };

    inline std::unique_ptr<GameplayAIDecisionInstance> CreateDecision(std::string_view id, EntityHandle agent,
        LevelAsset& level, GameplayWorld& world, const GameplayTraversalLinkRegistry& links,
        const GameplayTraversalExecutorRegistry& executors, GameplayObjectReservationSystem& reservations)
    {
        return MakeDefaultGameplayAIDecisionFactories().Create(id, {agent, level, world, links, executors, reservations});
    }
}
