module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_ai_access_key_composition;
import :gameplay_ai_access_key_observation;
import :gameplay_ai_access_key_contracts;
import :gameplay_ai_access_key_navigation;
import :gameplay_ai_buy_key_action;

import :ai_action_contracts;
import :ai_move_to_action_binding;
import :gameplay;
import :gameplay_ai_decision_contracts;
import :gameplay_goap_decision_instance;
import :gameplay_goap_path_inspection;
import :gameplay_goap_definition_asset;
import :gameplay_goap_definition_compiler;
import :gameplay_object_reservation_system;
import :gameplay_route;
import :gameplay_route_search;
import :gameplay_traversal_executor_registry;
import :gameplay_traversal_link;
import :gameplay_traversal_link_registry;
import :level;

export namespace rendern
{
    namespace ai_access_key_detail
    {
        [[nodiscard]] std::optional<AccessKeyFactBindings> ResolveFactBindings(
            const GameplayGOAPCompiledDefinition& compiled)
        {
            const auto boolean = [&](const std::string_view name)
            {
                return compiled.FindBooleanFact(name);
            };
            const auto hasKey=boolean("hasAccessKey"), destination=boolean("atDestination");
            const std::array collected{boolean("coinACollected"), boolean("coinBCollected"), boolean("coinCCollected")};
            const std::array available{boolean("coinAAvailable"), boolean("coinBAvailable"), boolean("coinCAvailable")};
            const std::array spatial{boolean("atStart"), boolean("atCoinA"), boolean("atCoinB"),
                boolean("atCoinC"), boolean("atAccessKeyShop"), boolean("atGoal")};
            const auto coins=compiled.FindIntegerFact("coins");
            if (!hasKey || !destination || !coins ||
                std::ranges::any_of(collected, [](const auto& value){ return !value.has_value(); }) ||
                std::ranges::any_of(available, [](const auto& value){ return !value.has_value(); }) ||
                std::ranges::any_of(spatial, [](const auto& value){ return !value.has_value(); }))
            {
                return std::nullopt;
            }
            AccessKeyFactBindings result{.hasAccessKey=*hasKey,.atDestination=*destination,.coins=*coins};
            for (std::size_t index = 0; index < collected.size(); ++index)
            {
                result.collected[index] = *collected[index];
                result.available[index] = *available[index];
            }
            for (std::size_t index = 0; index < spatial.size(); ++index)
            {
                result.spatial[index] = *spatial[index];
            }
            return result;
        }

        [[nodiscard]] std::optional<GameplayGOAPCompiledDefinition> LoadCompiledDefinition(
            const GameplayRouteGraph& routeGraph)
        {
            try
            {
                const GameplayGOAPDefinitionAsset asset =
                    LoadGameplayGOAPDefinitionAsset("ai/goap/access_key.goap.json");
                const std::array semanticActions{
                    GameplayGOAPSemanticAction{"move_to", kAIMoveToActionId},
                    GameplayGOAPSemanticAction{"buy_key", kAIBuyKeyActionId}};
                std::vector<GameplayGOAPActionCostOverride> costs;
                costs.reserve(kMoveTransitions.size());
                for (const MoveTransition& transition : kMoveTransitions)
                {
                    const GameplayRouteSearchResult route = FindWeightedGameplayRoute(
                         routeGraph, transition.startNodeId, transition.goalNodeId);
                    if (!route.Succeeded() || !route.totalCost.has_value() ||
                        !std::isfinite(*route.totalCost) || *route.totalCost < 0.0f)
                    {
                        return std::nullopt;
                    }
                    costs.push_back(GameplayGOAPActionCostOverride{
                        .action = "move_to",
                        .context = std::string{transition.contextName},
                        .cost = *route.totalCost
                    });
                }
                GameplayGOAPCompiledDefinition compiled =
                   CompileGameplayGOAPDefinition(asset, semanticActions, costs);
                if (!ResolveMoveTransitions(compiled).has_value() ||
                    !compiled.FindActionContext("buy_key").has_value())
                {
                    return std::nullopt;
                }
                return compiled;
            }
            catch (const std::exception&)
            {
                return std::nullopt;
            }
        }

        // A successful setup contains every scenario resource resolved from authored
        // level/world data. Runtime orchestration therefore has no discovery path.
        struct AccessKeyDecisionSetup
        {
            GameplayGOAPDecisionDefinition definition{};
            AccessKeyFactBindings factBindings{};
            GameplayRouteGraph routeGraph{};
            std::vector<ResolvedMoveTransition> moveTransitions{};
            std::array<EntityHandle, 3> coinEntities{};
            EntityHandle keyEntity{kNullEntity};
            std::array<mathUtils::Vec3, kSpatialLocationCount> spatialPositions{};
            bool reservationsEnabled{};
        };

        class AccessKeyGOAPContext final :
            public IGameplayGOAPContext,
            public IGameplayGOAPPlannedPathProvider
        {
        public:
            AccessKeyGOAPContext(const EntityHandle agent, const AccessKeyDecisionSetup& setup,
                GameplayWorld& world, GameplayObjectReservationSystem* reservations,
                GameplayRouteGraph routeGraph, std::vector<ResolvedMoveTransition> transitions)
                : moveToRequests_(world, std::move(routeGraph), std::move(transitions)),
                  observer_(agent, setup.coinEntities, setup.keyEntity, setup.factBindings,
                      setup.spatialPositions, setup.reservationsEnabled ? reservations : nullptr)
            {
                moveToRequests_.SetCoinEntities(
                    setup.coinEntities[0], setup.coinEntities[1], setup.coinEntities[2]);
            }

            void Observe(const GameplayWorld& world, std::span<const GameplayWorldEvent> events,
                AIAgentWorldState& facts) override
            {
                observer_.Observe(events, world, facts);
            }

            void ObserveActionEvents(const GameplayWorld& world,
                std::span<const GameplayWorldEvent> events, AIAgentWorldState& facts) override
            {
                observer_.Observe(events, world, facts);
            }

            [[nodiscard]] GameplayAIDebugPlannedPathView BuildPlannedPath(
                const std::span<const AIPlanStep> selectedPlan,
                const std::optional<std::size_t> currentStepIndex) const override
            {
                return BuildPlannedPathDebugView(selectedPlan, currentStepIndex, moveToRequests_);
            }

            [[nodiscard]] AccessKeyMoveToRequestProvider& MoveToRequests() noexcept
            {
                return moveToRequests_;
            }

        private:
            AccessKeyMoveToRequestProvider moveToRequests_;
            AccessKeyObservationAdapter observer_;
        };

        [[nodiscard]] int FindNode(const LevelAsset& level, const std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < level.nodes.size(); ++index)
            {
                if (level.nodes[index].alive && level.nodes[index].name == name)
                {
                    return static_cast<int>(index);
                }
            }
            return -1;
        }

        [[nodiscard]] EntityHandle FindNodeEntity(const GameplayWorld& world, const int nodeIndex)
        {
            std::vector<EntityHandle> entities;
            world.CollectNodeLinkEntities(entities);
            for (const EntityHandle entity : entities)
            {
                const GameplayNodeLinkComponent* link = world.TryGetNodeLink(entity);
                if (link != nullptr && link->nodeIndex == nodeIndex)
                {
                    return entity;
                }
            }
            return kNullEntity;
        }

        [[nodiscard]] std::optional<ai_access_key_detail::AccessKeyDecisionSetup>
       BuildAccessKeyDecisionSetup(const LevelAsset& level, const GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinkRegistry,
           const GameplayObjectReservationSystem* reservationSystem = nullptr)
        {
            using namespace ai_access_key_detail;
            constexpr std::array nodeNames{"GOAP_Start", "GOAP_Coin_A", "GOAP_Coin_B",
                "GOAP_Coin_C", "GOAP_Access_Key", "GOAP_Final_Goal"};
            std::array<int, nodeNames.size()> nodes{};
            std::array<mathUtils::Vec3, kSpatialLocationCount> positions{};
            for (std::size_t index = 0; index < nodeNames.size(); ++index)
            {
                nodes[index] = FindNode(level, nodeNames[index]);
                if (nodes[index] < 0)
                {
                    return std::nullopt;
                }
                positions[index] = level.nodes[static_cast<std::size_t>(nodes[index])].transform.position;
            }

            const std::array coinEntities{FindNodeEntity(world, nodes[1]),
                FindNodeEntity(world, nodes[2]), FindNodeEntity(world, nodes[3])};
            const EntityHandle keyEntity = FindNodeEntity(world, nodes[4]);
            if (keyEntity == kNullEntity ||
                std::ranges::any_of(coinEntities, [&](const EntityHandle coin)
                {
                    return coin == kNullEntity || !world.HasPickup(coin);
                }))
            {
                return std::nullopt;
            }
            const int jumpTakeoffNode = FindNode(level, "GOAP_Jump_Takeoff");
            const int jumpLandingNode = FindNode(level, "GOAP_Jump_Landing");
            const bool goalRequiresJump = jumpTakeoffNode >= 0 || jumpLandingNode >= 0;
            if (goalRequiresJump)
            {
                if (jumpTakeoffNode < 0 || jumpLandingNode < 0)
                {
                    return std::nullopt;
                }

                const EntityHandle landingEntity = FindNodeEntity(world, jumpLandingNode);
                const std::optional<GameplayTraversalLink> jumpLink =
                    traversalLinkRegistry.Find(kAccessKeyGoalJumpTraversalLink);
                if (landingEntity == kNullEntity || !jumpLink.has_value() ||
                    jumpLink->targetEntity != landingEntity ||
                    jumpLink->traversalTypeId != kJumpTraversalTypeId)
                {
                    return std::nullopt;
                }
            }
            const std::optional<mathUtils::Vec3> jumpTakeoff = goalRequiresJump
                ? std::optional{level.nodes[static_cast<std::size_t>(jumpTakeoffNode)].transform.position}
            : std::nullopt;
            const std::optional<mathUtils::Vec3> jumpLanding = goalRequiresJump
                ? std::optional{level.nodes[static_cast<std::size_t>(jumpLandingNode)].transform.position}
            : std::nullopt;
            GameplayRouteGraph routeGraph = BuildRouteGraph(positions[0], positions[1],
            positions[2], positions[3], positions[4], positions[5],
                jumpTakeoff, jumpLanding);
            std::optional<GameplayGOAPCompiledDefinition> compiled =
                LoadCompiledDefinition(routeGraph);
            if (!compiled.has_value())
            {
                return std::nullopt;
            }
            const std::optional<AccessKeyFactBindings> factBindings =
               ResolveFactBindings(*compiled);
            std::optional<std::vector<ResolvedMoveTransition>> transitions =
                ResolveMoveTransitions(*compiled);
            if (!factBindings.has_value() || !transitions.has_value())
            {
                return std::nullopt;
            }

            const bool reservationsEnabled = reservationSystem != nullptr &&
                std::ranges::all_of(coinEntities, [&](const EntityHandle coin)
                {
                    return world.HasInteractionPoint(coin);
                });
            return AccessKeyDecisionSetup{
                .definition = std::move(compiled->definition),
                .factBindings = *factBindings,
                .routeGraph = std::move(routeGraph),
                .moveTransitions = std::move(*transitions),
                .coinEntities = coinEntities,
                .keyEntity = keyEntity,
                .spatialPositions = positions,
                .reservationsEnabled = reservationsEnabled};
        }

        [[nodiscard]] std::optional<ai_access_key_detail::AccessKeyDecisionSetup>
        BuildAccessKeyDecisionSetup(const LevelAsset& level, const GameplayWorld& world)
        {
            const GameplayTraversalLinkRegistry emptyTraversalLinkRegistry{};
            return BuildAccessKeyDecisionSetup(level, world, emptyTraversalLinkRegistry);
        }
    }

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateAccessKeyAIDecision(
        const EntityHandle agent, LevelAsset& level, GameplayWorld& world,
        const GameplayTraversalLinkRegistry& traversalLinkRegistry,
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
        GameplayObjectReservationSystem* reservationSystem = nullptr)
    {
        std::optional<ai_access_key_detail::AccessKeyDecisionSetup> setup =
            ai_access_key_detail::BuildAccessKeyDecisionSetup(
                level, world, traversalLinkRegistry, reservationSystem);
        if (!setup.has_value())
        {
            return nullptr;
        }
        if (setup->reservationsEnabled && reservationSystem == nullptr)
        {
            return nullptr;
        }
        auto domain = std::make_unique<ai_access_key_detail::AccessKeyGOAPContext>(
            agent, *setup, world, reservationSystem,
            std::move(setup->routeGraph), std::move(setup->moveTransitions));
        auto* requests = &domain->MoveToRequests();
        GameplayGOAPDecisionSetup goapSetup{
            .definition = std::move(setup->definition),
            .context = std::move(domain)};
        goapSetup.actionBindings.push_back({kAIMoveToActionId,
            [&world, &traversalLinkRegistry, &traversalExecutorRegistry, requests,
                reservationSystem, enabled = setup->reservationsEnabled]
            (AIAgentWorldState&, std::vector<GameplayWorldEvent>&)
                -> std::unique_ptr<IAIActionBinding>
            {
                if (enabled)
                {
                    return std::make_unique<AIMoveToActionBinding>(world,
                        traversalLinkRegistry, traversalExecutorRegistry, *requests,
                        *reservationSystem, *requests);
                }
                return std::make_unique<AIMoveToActionBinding>(world,
                    traversalLinkRegistry, traversalExecutorRegistry, *requests);
            }});
        goapSetup.actionBindings.push_back({kAIBuyKeyActionId,
            [&world, keyEntity = setup->keyEntity,
                keyPosition = setup->spatialPositions[
                    static_cast<std::size_t>(ai_access_key_detail::SpatialLocation::AccessKeyShop)],
                facts = setup->factBindings]
            (AIAgentWorldState& observed, std::vector<GameplayWorldEvent>& events)
            {
                return std::make_unique<ai_access_key_detail::BuyKeyActionBinding>(
                    observed, world, events, keyEntity, keyPosition, facts.hasAccessKey, facts.coins);
            }});
        return CreateGameplayGOAPDecision(agent, std::move(goapSetup));
    }
}
