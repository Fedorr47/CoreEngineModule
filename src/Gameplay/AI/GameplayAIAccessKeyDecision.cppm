module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <span>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_ai_access_key_decision;

import :ai_move_to_action_binding;
import :ai_system;
import :gameplay;
import :gameplay_ai_decision_contracts;
import :gameplay_goap_decision;
import :gameplay_goap_definition_asset;
import :gameplay_object_reservation_system;
import :gameplay_route;
import :gameplay_route_search;
import :gameplay_traversal_executor_registry;
import :gameplay_traversal_link_registry;
import :level;

export namespace rendern
{
    inline constexpr std::string_view kAccessKeyAIDecisionId{"access_key"};
    inline constexpr AIWorldFactId kGOAPHasAccessKeyFact{0u};
    inline constexpr AIWorldFactId kGOAPAtDestinationFact{1u};
    inline constexpr AIWorldFactId kGOAPCoinACollectedFact{2u};
    inline constexpr AIWorldFactId kGOAPCoinBCollectedFact{3u};
    inline constexpr AIWorldFactId kGOAPCoinCCollectedFact{4u};
    inline constexpr AIWorldFactId kGOAPAtAccessKeyShopFact{5u};
    inline constexpr AIWorldFactId kGOAPAtStartFact{6u};
    inline constexpr AIWorldFactId kGOAPAtCoinAFact{7u};
    inline constexpr AIWorldFactId kGOAPAtCoinBFact{8u};
    inline constexpr AIWorldFactId kGOAPAtCoinCFact{9u};
    inline constexpr AIWorldFactId kGOAPAtGoalFact{10u};
    inline constexpr AIWorldFactId kGOAPCoinAAvailableFact{11u};
    inline constexpr AIWorldFactId kGOAPCoinBAvailableFact{12u};
    inline constexpr AIWorldFactId kGOAPCoinCAvailableFact{13u};
    inline constexpr AIWorldIntegerFactId kGOAPCoinCountFact{0u};
    inline constexpr std::int32_t kAccessKeyPrice = 2;
    inline constexpr AIActionId kAIBuyKeyActionId{3u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{0u};

    namespace ai_access_key_detail
    {
        enum class SpatialLocation : std::uint8_t
        {
            Start,
            CoinA,
            CoinB,
            CoinC,
            AccessKeyShop,
            Goal,
            Count
        };

        inline constexpr std::size_t kSpatialLocationCount =
            static_cast<std::size_t>(SpatialLocation::Count);
        inline constexpr float kSpatialArrivalRadius = 0.6f;
        inline constexpr float kSpatialArrivalRadiusSquared = kSpatialArrivalRadius * kSpatialArrivalRadius;
        inline constexpr GameplayRouteNodeId kStartNode{1u};
        inline constexpr GameplayRouteNodeId kCoinANode{2u};
        inline constexpr GameplayRouteNodeId kCoinBNode{3u};
        inline constexpr GameplayRouteNodeId kCoinCNode{4u};
        inline constexpr GameplayRouteNodeId kAccessKeyNode{5u};
        inline constexpr GameplayRouteNodeId kFinalGoalNode{6u};
        
        [[nodiscard]] constexpr GameplayRouteNodeId SpatialNode(
            const SpatialLocation location) noexcept
        {
            constexpr std::array nodes{kStartNode, kCoinANode, kCoinBNode, kCoinCNode,
                kAccessKeyNode, kFinalGoalNode};
            return nodes[static_cast<std::size_t>(location)];
        }
        

        struct MoveTransition
        {
            std::string_view contextName{};
            SpatialLocation source{};
            SpatialLocation target{};
            GameplayRouteNodeId startNodeId{};
            GameplayRouteNodeId goalNodeId{};
        };
        
        [[nodiscard]] constexpr MoveTransition Transition(
        const std::string_view contextName, const SpatialLocation source, const SpatialLocation target) noexcept
        {
            return MoveTransition{
                .contextName = contextName,
                .source = source,
                .target = target,
                .startNodeId = SpatialNode(source),
                .goalNodeId = SpatialNode(target)
            };
        }

        inline constexpr std::array kMoveTransitions{
            Transition("start_to_coin_a", SpatialLocation::Start, SpatialLocation::CoinA),
            Transition("start_to_coin_b", SpatialLocation::Start, SpatialLocation::CoinB),
            Transition("start_to_coin_c", SpatialLocation::Start, SpatialLocation::CoinC),
            Transition("coin_a_to_coin_b", SpatialLocation::CoinA, SpatialLocation::CoinB),
            Transition("coin_a_to_coin_c", SpatialLocation::CoinA, SpatialLocation::CoinC),
            Transition("coin_a_to_access_key_shop", SpatialLocation::CoinA, SpatialLocation::AccessKeyShop),
            Transition("coin_b_to_coin_a", SpatialLocation::CoinB, SpatialLocation::CoinA),
            Transition("coin_b_to_coin_c", SpatialLocation::CoinB, SpatialLocation::CoinC),
            Transition("coin_b_to_access_key_shop", SpatialLocation::CoinB, SpatialLocation::AccessKeyShop),
            Transition("coin_c_to_coin_a", SpatialLocation::CoinC, SpatialLocation::CoinA),
            Transition("coin_c_to_coin_b", SpatialLocation::CoinC, SpatialLocation::CoinB),
            Transition("coin_c_to_access_key_shop", SpatialLocation::CoinC, SpatialLocation::AccessKeyShop),
            Transition("access_key_shop_to_goal", SpatialLocation::AccessKeyShop, SpatialLocation::Goal)};

        struct ResolvedMoveTransition
        {
            AIActionContextId contextId{};
            SpatialLocation source{};
            SpatialLocation target{};
            GameplayRouteNodeId startNodeId{};
            GameplayRouteNodeId goalNodeId{};
        };

        [[nodiscard]] std::optional<std::vector<ResolvedMoveTransition>>
            ResolveMoveTransitions(const GameplayGOAPCompiledDefinition& compiled)
        {
            std::vector<ResolvedMoveTransition> resolved;
            resolved.reserve(kMoveTransitions.size());
            for (const MoveTransition& transition : kMoveTransitions)
            {
                const std::optional<AIActionContextId> context =
                    compiled.FindActionContext(transition.contextName);
                if (!context.has_value())
                {
                    return std::nullopt;
                }
                resolved.push_back({*context, transition.source, transition.target,
                    transition.startNodeId, transition.goalNodeId});
            }
            return resolved;
        }

        [[nodiscard]] std::optional<AIActionContextId> FindMoveContext(
            const GameplayGOAPCompiledDefinition& compiled,
            const SpatialLocation source, const SpatialLocation target)
        {
            const auto transition = std::ranges::find_if(kMoveTransitions,
                [&](const MoveTransition& candidate)
                {
                    return candidate.source == source && candidate.target == target;
                });
            return transition == kMoveTransitions.end()
                ? std::nullopt : compiled.FindActionContext(transition->contextName);
        }

        struct AccessKeyFactBindings
        {
            AIWorldFactId hasAccessKey{};
            AIWorldFactId atDestination{};
            std::array<AIWorldFactId, 3> collected{};
            std::array<AIWorldFactId, 3> available{};
            std::array<AIWorldFactId, kSpatialLocationCount> spatial{};
            AIWorldIntegerFactId coins{};
        };

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
            for (std::size_t index=0; index<3; ++index) { result.collected[index]=*collected[index]; result.available[index]=*available[index]; }
            for (std::size_t index=0; index<spatial.size(); ++index) { result.spatial[index]=*spatial[index]; }
            return result;
        }

        class AccessKeyMoveToRequestProvider final : 
            public IAIMoveToActionRequestProvider,
            public IAIActionReservationTargetProvider
        {
        public:
            AccessKeyMoveToRequestProvider(GameplayWorld& world, GameplayRouteGraph routeGraph,
                std::vector<ResolvedMoveTransition> transitions)
                 : world_(world), routeGraph_(std::move(routeGraph)),
                   transitions_(std::move(transitions))
            {
            }
            
            void SetCoinEntities(const EntityHandle coinA, const EntityHandle coinB,
                const EntityHandle coinC) noexcept
            {
                coinEntities_ = {coinA, coinB, coinC};
            }

            EntityHandle ResolveReservationTarget(
                const AIActionRuntimeContext& context) override
            {
                const auto transition = std::ranges::find_if(transitions_,
                    [&](const ResolvedMoveTransition& candidate)
                    {
                        return candidate.contextId == context.contextId;
                    });
                if (transition == transitions_.end() ||
                    transition->target < SpatialLocation::CoinA ||
                    transition->target > SpatialLocation::CoinC)
                {
                    return kNullEntity;
                }
                return coinEntities_[static_cast<std::size_t>(transition->target) - 1u];
            }
            
            std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                const auto transition = std::ranges::find_if(transitions_,
                    [&](const ResolvedMoveTransition& candidate)
                     {
                         return candidate.contextId == context.contextId;
                     });
                if (transition == transitions_.end())
                {
                    return std::nullopt;
                }
                const auto* transform = world_.TryGetTransform(context.agentEntity);
                if (transform == nullptr)
                {
                    return std::nullopt;
                }
                GameplayRouteNodeId nearestNode{};
                float nearestDistanceSquared = std::numeric_limits<float>::max();
                for (const GameplayRouteGraphNode& node : routeGraph_.nodes)
                {
                    const mathUtils::Vec3 delta = transform->position - node.worldPosition;
                    const float distanceSquared = mathUtils::Dot(delta, delta);
                    if (distanceSquared < nearestDistanceSquared)
                    {
                        nearestDistanceSquared = distanceSquared;
                        nearestNode = node.nodeId;
                    }
                }
                if (nearestNode != transition->startNodeId)
                {
                    return std::nullopt;
                }
                
                const auto sourceNode = std::ranges::find_if(
                    routeGraph_.nodes,
                    [&](const GameplayRouteGraphNode& node)
                    {
                        return node.nodeId == transition->startNodeId;
                    });
                if (sourceNode == routeGraph_.nodes.end())
                {
                    return std::nullopt;
                }
                
                const mathUtils::Vec3 sourceDelta =
                    transform->position - sourceNode->worldPosition;
                if (mathUtils::Dot(sourceDelta, sourceDelta) >
                    kSpatialArrivalRadiusSquared)
                {
                    return std::nullopt;
                }
                
                GameplayArrivalSteeringSettings steering{};
                steering.acceptanceRadius = 0.35f;
                steering.slowingRadius = 1.25f;
                return AIMoveToActionRequest{&routeGraph_, transition->startNodeId,
                    transition->goalNodeId, steering};
            }

        private:
            GameplayWorld& world_;
            GameplayRouteGraph routeGraph_{};
            std::vector<ResolvedMoveTransition> transitions_{};
            std::array<EntityHandle, 3> coinEntities_{};
        };

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

        [[nodiscard]] GameplayRouteGraph BuildRouteGraph(
            const mathUtils::Vec3 start, const mathUtils::Vec3 coinA,
            const mathUtils::Vec3 coinB, const mathUtils::Vec3 coinC,
            const mathUtils::Vec3 key, const mathUtils::Vec3 goal)
        {
            GameplayRouteGraph graph{};
            graph.nodes = {{kStartNode, start}, {kCoinANode, coinA}, {kCoinBNode, coinB},
                {kCoinCNode, coinC}, {kAccessKeyNode, key}, {kFinalGoalNode, goal}};
            for (const MoveTransition& transition : kMoveTransitions)
            {
                const mathUtils::Vec3 delta =
                    graph.nodes[static_cast<std::size_t>(transition.target)].worldPosition -
                    graph.nodes[static_cast<std::size_t>(transition.source)].worldPosition;
                graph.edges.push_back({transition.startNodeId, transition.goalNodeId,
                    std::sqrt(mathUtils::Dot(delta, delta))});
            }
            return graph;
        }
        
        [[nodiscard]] AccessKeyMoveToRequestProvider BuildMoveToProvider(
            GameplayWorld& world, GameplayRouteGraph routeGraph,
            std::vector<ResolvedMoveTransition> transitions)
        {
            return AccessKeyMoveToRequestProvider{
                world, std::move(routeGraph), std::move(transitions)};
        }
        
        class BuyKeyActionRuntime final : public IAIActionRuntime
        {
        public:
            BuyKeyActionRuntime(AIAgentWorldState& facts, const GameplayWorld& world,
                std::vector<GameplayWorldEvent>& events, const EntityHandle keyEntity,
                const mathUtils::Vec3 keyPosition, const AIWorldFactId hasAccessKeyFact,
                const AIWorldIntegerFactId coinsFact) noexcept
                : facts_(facts), world_(world), events_(events), keyEntity_(keyEntity),
                    keyPosition_(keyPosition), hasAccessKeyFact_(hasAccessKeyFact),
                    coinsFact_(coinsFact)
            {
            }

            [[nodiscard]] AIActionRuntimeResult Start(
                const AIActionRuntimeContext& context) override
            {
                if (context.actionId != kAIBuyKeyActionId || context.agentEntity == kNullEntity ||
                    keyEntity_ == kNullEntity || facts_.IsFactSet(hasAccessKeyFact_) ||
                    facts_.GetIntegerFact(coinsFact_) < kAccessKeyPrice)
                {
                    return AIActionRuntimeResult::Failed;
                }
                const GameplayTransformComponent* transform =
                    world_.TryGetTransform(context.agentEntity);
                if (transform == nullptr)
                {
                    return AIActionRuntimeResult::Failed;
                }
                const mathUtils::Vec3 delta = transform->position - keyPosition_;
                if (mathUtils::Dot(delta, delta) > 0.36f)
                {
                    return AIActionRuntimeResult::Failed;
                }
                committed_ = true;
                events_.push_back({GameplayWorldEventType::AccessKeyPurchased,
                    context.agentEntity, keyEntity_});
                return AIActionRuntimeResult::Succeeded;
            }

            [[nodiscard]] AIActionRuntimeResult Tick(
                const AIActionRuntimeContext&, float) override
            {
                return committed_ ? AIActionRuntimeResult::Succeeded
                                  : AIActionRuntimeResult::Failed;
            }

            void Cancel(const AIActionRuntimeContext&) noexcept override
            {
                // Start commits atomically; cancellation cannot partially purchase the key.
            }

        private:
            AIAgentWorldState& facts_;
            const GameplayWorld& world_;
            std::vector<GameplayWorldEvent>& events_;
            EntityHandle keyEntity_{kNullEntity};
            mathUtils::Vec3 keyPosition_{};
            AIWorldFactId hasAccessKeyFact_{};
            AIWorldIntegerFactId coinsFact_{};
            bool committed_{};
        };

        class BuyKeyActionBinding final : public IAIActionBinding
        {
        public:
            BuyKeyActionBinding(AIAgentWorldState& facts, const GameplayWorld& world,
                const EntityHandle keyEntity, const mathUtils::Vec3 keyPosition,
                 const AIWorldFactId hasAccessKeyFact,
                const AIWorldIntegerFactId coinsFact) noexcept
            : facts_(facts), world_(world), keyEntity_(keyEntity), keyPosition_(keyPosition),
                hasAccessKeyFact_(hasAccessKeyFact), coinsFact_(coinsFact)
            {
            }

            void SetEventOutput(std::vector<GameplayWorldEvent>* events) noexcept
            {
                events_ = events;
            }

            [[nodiscard]] std::unique_ptr<IAIActionRuntime> CreateRuntime(
                const AIActionRuntimeContext& context) override
            {
                if (context.actionId != kAIBuyKeyActionId || events_ == nullptr)
                {
                    return nullptr;
                }
                return std::make_unique<BuyKeyActionRuntime>(
                    facts_, world_, *events_, keyEntity_, keyPosition_,
                    hasAccessKeyFact_, coinsFact_);
            }

        private:
            AIAgentWorldState& facts_;
            const GameplayWorld& world_;
            EntityHandle keyEntity_{kNullEntity};
            mathUtils::Vec3 keyPosition_{};
            AIWorldFactId hasAccessKeyFact_{};
            AIWorldIntegerFactId coinsFact_{};
            std::vector<GameplayWorldEvent>* events_{};
        };

        class AccessKeyDecision final : public GameplayAIDecisionInstance
        {
        public:
            AccessKeyDecision(const EntityHandle agent, GameplayGOAPDecisionDefinition definition,
                AccessKeyFactBindings factBindings, AccessKeyMoveToRequestProvider moveToRequests,
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                GameplayObjectReservationSystem* reservationSystem,
                const EntityHandle coinAEntity, const EntityHandle coinBEntity,
                const EntityHandle coinCEntity, const EntityHandle keyEntity,
                const std::array<mathUtils::Vec3, kSpatialLocationCount>& spatialPositions)
                : moveToRequests_(std::move(moveToRequests)), factBindings_(factBindings),
                    goap_(agent, std::move(definition)), agent_(agent), coinAEntity_(coinAEntity),
                    coinBEntity_(coinBEntity), coinCEntity_(coinCEntity), keyEntity_(keyEntity),
                    spatialPositions_(spatialPositions)
            {
                moveToRequests_.SetCoinEntities(coinAEntity, coinBEntity, coinCEntity);
                const bool bReservationsEnabled = reservationSystem != nullptr &&
                    world.HasInteractionPoint(coinAEntity) &&
                    world.HasInteractionPoint(coinBEntity) &&
                    world.HasInteractionPoint(coinCEntity);
                reservationSystem_ = bReservationsEnabled ? reservationSystem : nullptr;
                std::unique_ptr<AIMoveToActionBinding> binding;
                if (bReservationsEnabled)
                {
                    binding = std::make_unique<AIMoveToActionBinding>(world,
                        traversalLinkRegistry, traversalExecutorRegistry, moveToRequests_,
                        *reservationSystem, moveToRequests_);
                }
                else
                {
                    binding = std::make_unique<AIMoveToActionBinding>(world,
                        traversalLinkRegistry, traversalExecutorRegistry, moveToRequests_);
                }
                bindingsInstalled_ = goap_.InstallActionBinding(
                    kAIMoveToActionId, std::move(binding));
                auto buyKeyBinding = std::make_unique<BuyKeyActionBinding>(
                goap_.GetObservedState(), world, keyEntity_,
                    spatialPositions_[static_cast<std::size_t>(SpatialLocation::AccessKeyShop)],
                    factBindings_.hasAccessKey, factBindings_.coins);
                buyKeyBinding_ = buyKeyBinding.get();
                bindingsInstalled_ = bindingsInstalled_ && goap_.InstallActionBinding(
                    kAIBuyKeyActionId, std::move(buyKeyBinding));
            }

            [[nodiscard]] bool IsConfigured() const noexcept
            {
                return bindingsInstalled_;
            }

            void Update(AISystem& aiSystem, const GameplayAIObservationContext& observation) override
            {
                {
                    // Consume input before a runtime can append to and reallocate the same buffer.
                    const std::span<const GameplayWorldEvent> inputEvents = observation.events;
                    Observe_(inputEvents, observation.world, goap_.GetObservedState());
                }
                const std::size_t eventCountBeforeRuntime = observation.eventOutput == nullptr
                    ? 0u : observation.eventOutput->size();
                buyKeyBinding_->SetEventOutput(observation.eventOutput);
                goap_.Update(aiSystem, observation.world);
                if (observation.eventOutput != nullptr &&
                    observation.eventOutput->size() > eventCountBeforeRuntime)
                {
                    Observe_(std::span<const GameplayWorldEvent>{*observation.eventOutput}.subspan(
                        eventCountBeforeRuntime), observation.world, goap_.GetObservedState());
                }
            }

            void Cancel(AISystem& aiSystem) noexcept override { goap_.Cancel(aiSystem); }
            [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept override
            {
                return goap_.GetStatus();
            }
            [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept override
            {
                return goap_.GetObservedState();
            }

        private:
            void Observe_(const std::span<const GameplayWorldEvent> events,
                const GameplayWorld& world, AIAgentWorldState& facts)
            {
                const auto collectCoin = [&](const EntityHandle subject,
                    const EntityHandle coin, const AIWorldFactId collectedFact)
                {
                    if (subject == coin && !facts.IsFactSet(collectedFact))
                    {
                        facts.SetFact(collectedFact, true);
                        facts.SetIntegerFact(factBindings_.coins,
                            facts.GetIntegerFact(factBindings_.coins) + 1);
                    }
                };
                for (const GameplayWorldEvent& event : events)
                {
                    if (event.instigator != agent_)
                    {
                        continue;
                    }
                    if (event.type == GameplayWorldEventType::PickupCollected)
                    {
                        collectCoin(event.subject, coinAEntity_, factBindings_.collected[0]);
                        collectCoin(event.subject, coinBEntity_, factBindings_.collected[1]);
                        collectCoin(event.subject, coinCEntity_, factBindings_.collected[2]);
                    }
                    else if (event.type == GameplayWorldEventType::AccessKeyPurchased &&
                        event.subject == keyEntity_ &&
                        !facts.IsFactSet(factBindings_.hasAccessKey))
                    {
                        const std::int32_t coins = facts.GetIntegerFact(factBindings_.coins);
                        if (coins >= kAccessKeyPrice)
                        {
                            facts.SetIntegerFact(factBindings_.coins, coins - kAccessKeyPrice);
                            facts.SetFact(factBindings_.hasAccessKey, true);
                        }
                    }
                }
                
                const auto observeAvailability = [&](const EntityHandle coin,
                    const AIWorldFactId availableFact)
                {
                    const GameplayPickupComponent* pickup = world.TryGetPickup(coin);
                    const bool bReservedByOther = reservationSystem_ != nullptr &&
                        reservationSystem_->IsReserved(coin) &&
                        !reservationSystem_->IsReservedBy(coin, agent_);
                    facts.SetFact(availableFact,
                    world.IsEntityValid(coin) && pickup != nullptr && !pickup->collected && !bReservedByOther);
                };
                observeAvailability(coinAEntity_, factBindings_.available[0]);
                observeAvailability(coinBEntity_, factBindings_.available[1]);
                observeAvailability(coinCEntity_, factBindings_.available[2]);

                const auto* transform = world.TryGetTransform(agent_);
                if (transform == nullptr)
                {
                    return;
                }
                std::optional<SpatialLocation> confirmedLocation;
                float confirmedDistanceSquared = kSpatialArrivalRadiusSquared;
                for (std::size_t index = 0; index < kSpatialLocationCount; ++index)
                {
                    const mathUtils::Vec3 delta =
                       transform->position - spatialPositions_[index];
                    const float distanceSquared = mathUtils::Dot(delta, delta);
                    if (distanceSquared <= confirmedDistanceSquared)
                    {
                        confirmedDistanceSquared = distanceSquared;
                        confirmedLocation = static_cast<SpatialLocation>(index);
                    }
                }
                if (confirmedLocation.has_value())
                {
                    for (std::size_t index = 0; index < kSpatialLocationCount; ++index)
                    {
                        facts.SetFact(factBindings_.spatial[index],
                            index == static_cast<std::size_t>(*confirmedLocation));
                    }
                    if (*confirmedLocation == SpatialLocation::Goal &&
                        facts.IsFactSet(factBindings_.hasAccessKey))
                    {
                        facts.SetFact(factBindings_.atDestination, true);
                    }
                }
            }

            AccessKeyMoveToRequestProvider moveToRequests_;
            const AccessKeyFactBindings factBindings_;
            GameplayGOAPDecision goap_;
            EntityHandle agent_{};
            EntityHandle coinAEntity_{ kNullEntity };
            EntityHandle coinBEntity_{ kNullEntity };
            EntityHandle coinCEntity_{ kNullEntity };
            EntityHandle keyEntity_{ kNullEntity };
            std::array<mathUtils::Vec3, kSpatialLocationCount> spatialPositions_{};
            BuyKeyActionBinding* buyKeyBinding_{};
            GameplayObjectReservationSystem* reservationSystem_{};
            mathUtils::Vec3 finalGoalPosition_{};
            bool bindingsInstalled_{};
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
    }

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateAccessKeyAIDecision(
        const EntityHandle agent, LevelAsset& level, GameplayWorld& world,
        const GameplayTraversalLinkRegistry& traversalLinkRegistry,
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
        GameplayObjectReservationSystem* reservationSystem = nullptr)
    {
        const int start = ai_access_key_detail::FindNode(level, "GOAP_Start");
        const int coinA = ai_access_key_detail::FindNode(level, "GOAP_Coin_A");
        const int coinB = ai_access_key_detail::FindNode(level, "GOAP_Coin_B");
        const int coinC = ai_access_key_detail::FindNode(level, "GOAP_Coin_C");
        const int key = ai_access_key_detail::FindNode(level, "GOAP_Access_Key");
        const int goal = ai_access_key_detail::FindNode(level, "GOAP_Final_Goal");
        if (start < 0 || coinA < 0 || coinB < 0 || coinC < 0 || key < 0 || goal < 0)
        {
            return nullptr;
        }
        const mathUtils::Vec3 startPosition =
            level.nodes[static_cast<std::size_t>(start)].transform.position;
        const mathUtils::Vec3 coinAPosition =
            level.nodes[static_cast<std::size_t>(coinA)].transform.position;
        const mathUtils::Vec3 coinBPosition =
            level.nodes[static_cast<std::size_t>(coinB)].transform.position;
        const mathUtils::Vec3 coinCPosition =
            level.nodes[static_cast<std::size_t>(coinC)].transform.position;
        const mathUtils::Vec3 keyPosition =
            level.nodes[static_cast<std::size_t>(key)].transform.position;
        const mathUtils::Vec3 goalPosition =
            level.nodes[static_cast<std::size_t>(goal)].transform.position;
        const EntityHandle coinAEntity = ai_access_key_detail::FindNodeEntity(world, coinA);
        const EntityHandle coinBEntity = ai_access_key_detail::FindNodeEntity(world, coinB);
        const EntityHandle coinCEntity = ai_access_key_detail::FindNodeEntity(world, coinC);
        const EntityHandle keyEntity = ai_access_key_detail::FindNodeEntity(world, key);

        if (coinAEntity == kNullEntity
            || coinBEntity == kNullEntity
            || coinCEntity == kNullEntity
            || keyEntity == kNullEntity
            || !world.HasPickup(coinAEntity)
            || !world.HasPickup(coinBEntity)
            || !world.HasPickup(coinCEntity))
        {
            return nullptr;
        }
        GameplayRouteGraph routeGraph = ai_access_key_detail::BuildRouteGraph(startPosition,
            coinAPosition, coinBPosition, coinCPosition, keyPosition, goalPosition);
        std::optional<GameplayGOAPCompiledDefinition> compiled =
             ai_access_key_detail::LoadCompiledDefinition(routeGraph);
        if (!compiled.has_value())
        {
            return nullptr;
        }
        const std::optional<ai_access_key_detail::AccessKeyFactBindings> factBindings =
            ai_access_key_detail::ResolveFactBindings(*compiled);
        std::optional<std::vector<ai_access_key_detail::ResolvedMoveTransition>> transitions =
            ai_access_key_detail::ResolveMoveTransitions(*compiled);
        if (!factBindings.has_value() || !transitions.has_value())
        {
            return nullptr;
        }
        auto decision = std::make_unique<ai_access_key_detail::AccessKeyDecision>(agent,
        std::move(compiled->definition), *factBindings,
         ai_access_key_detail::BuildMoveToProvider(
             world, std::move(routeGraph), std::move(*transitions)),
         world, traversalLinkRegistry, traversalExecutorRegistry, reservationSystem,
          coinAEntity, coinBEntity, coinCEntity, keyEntity, 
          std::array{startPosition, coinAPosition, coinBPosition, coinCPosition, keyPosition, goalPosition});
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}