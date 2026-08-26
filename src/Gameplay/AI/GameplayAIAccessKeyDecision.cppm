#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

export module core:gameplay_ai_access_key_decision;

import :ai_move_to_action_binding;
import :ai_system;
import :gameplay;
import :gameplay_ai_decision_contracts;
import :gameplay_goap_decision;
import :gameplay_route;
import :gameplay_route_search;
import :gameplay_traversal_executor_registry;
import :gameplay_traversal_link_registry;
import :level;

export namespace rendern
{
    inline constexpr std::string_view kAccessKeyAIDecisionId{"access_key"};
    inline constexpr AIWorldFactId kGOAPHasAccessKeyFact{40u};
    inline constexpr AIWorldFactId kGOAPAtDestinationFact{41u};
    inline constexpr AIWorldFactId kGOAPCoinACollectedFact{42u};
    inline constexpr AIWorldFactId kGOAPCoinBCollectedFact{43u};
    inline constexpr AIWorldFactId kGOAPCoinCCollectedFact{44u};
    inline constexpr AIWorldFactId kGOAPAtAccessKeyShopFact{45u};
    inline constexpr AIWorldFactId kGOAPAtStartFact{46u};
    inline constexpr AIWorldFactId kGOAPAtCoinAFact{47u};
    inline constexpr AIWorldFactId kGOAPAtCoinBFact{48u};
    inline constexpr AIWorldFactId kGOAPAtCoinCFact{49u};
    inline constexpr AIWorldFactId kGOAPAtGoalFact{50u};
    inline constexpr AIWorldFactId kGOAPCoinAAvailableFact{51u};
    inline constexpr AIWorldFactId kGOAPCoinBAvailableFact{52u};
    inline constexpr AIWorldFactId kGOAPCoinCAvailableFact{53u};
    inline constexpr AIWorldIntegerFactId kGOAPCoinCountFact{0u};
    inline constexpr std::int32_t kAccessKeyPrice = 2;
    inline constexpr AIActionId kAIBuyKeyActionId{3u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{50u};

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

        [[nodiscard]] constexpr AIWorldFactId SpatialFact(const SpatialLocation location) noexcept
        {
            constexpr std::array facts{kGOAPAtStartFact, kGOAPAtCoinAFact, kGOAPAtCoinBFact,
                kGOAPAtCoinCFact, kGOAPAtAccessKeyShopFact, kGOAPAtGoalFact};
            return facts[static_cast<std::size_t>(location)];
        }

        [[nodiscard]] constexpr GameplayRouteNodeId SpatialNode(
            const SpatialLocation location) noexcept
        {
            constexpr std::array nodes{kStartNode, kCoinANode, kCoinBNode, kCoinCNode,
                kAccessKeyNode, kFinalGoalNode};
            return nodes[static_cast<std::size_t>(location)];
        }

        [[nodiscard]] constexpr AIActionContextId MoveContext(
            const SpatialLocation source, const SpatialLocation target) noexcept
        {
            if (source == target ||
                source == SpatialLocation::Count ||
                target == SpatialLocation::Count)
            {
                return {};
            }

            const auto sourceIndex = static_cast<std::uint32_t>(source);
            const auto targetIndex = static_cast<std::uint32_t>(target);

            const auto contextValue = static_cast<AIActionContextId::ValueType>(
                100u +
                sourceIndex * static_cast<std::uint32_t>(kSpatialLocationCount) +
                targetIndex);

            return AIActionContextId{contextValue};
        }

        struct MoveTransition
        {
            AIActionContextId contextId{};
            SpatialLocation source{};
            SpatialLocation target{};
            GameplayRouteNodeId startNodeId{};
            GameplayRouteNodeId goalNodeId{};
        };
        
        [[nodiscard]] constexpr MoveTransition Transition(
            const SpatialLocation source, const SpatialLocation target) noexcept
        {
            return MoveTransition{
                .contextId = MoveContext(source, target),
                .source = source,
                .target = target,
                .startNodeId = SpatialNode(source),
                .goalNodeId = SpatialNode(target)
            };
        }

        inline constexpr std::array kMoveTransitions{
            Transition(SpatialLocation::Start, SpatialLocation::CoinA),
            Transition(SpatialLocation::Start, SpatialLocation::CoinB),
            Transition(SpatialLocation::Start, SpatialLocation::CoinC),
            Transition(SpatialLocation::CoinA, SpatialLocation::CoinB),
            Transition(SpatialLocation::CoinA, SpatialLocation::CoinC),
            Transition(SpatialLocation::CoinA, SpatialLocation::AccessKeyShop),
            Transition(SpatialLocation::CoinB, SpatialLocation::CoinA),
            Transition(SpatialLocation::CoinB, SpatialLocation::CoinC),
            Transition(SpatialLocation::CoinB, SpatialLocation::AccessKeyShop),
            Transition(SpatialLocation::CoinC, SpatialLocation::CoinA),
            Transition(SpatialLocation::CoinC, SpatialLocation::CoinB),
            Transition(SpatialLocation::CoinC, SpatialLocation::AccessKeyShop),
            Transition(SpatialLocation::AccessKeyShop, SpatialLocation::Goal)};

        class AccessKeyMoveToRequestProvider final :
            public IAIMoveToActionRequestProvider
        {
        public:
            AccessKeyMoveToRequestProvider(GameplayWorld& world, GameplayRouteGraph routeGraph)
                 : world_(world), routeGraph_(std::move(routeGraph))
            {
            }

            std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                const auto transition = std::ranges::find_if(kMoveTransitions,
                     [&](const MoveTransition& candidate)
                     {
                         return candidate.contextId == context.contextId;
                     });
                if (transition == kMoveTransitions.end())
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
        };

        [[nodiscard]] std::optional<GameplayGOAPDecisionDefinition> BuildDefinition(
            const GameplayRouteGraph& routeGraph)
        {
            GameplayGOAPDecisionDefinition definition{};
            definition.goals = {AIGoalSelectionCandidate{
                .goal=AIGoalDefinition{kGOAPReachDestinationGoal,
                    {{kGOAPAtDestinationFact, true}}}, .baseScore=1.0f}};
            for (const MoveTransition& transition : kMoveTransitions)
            {
                const GameplayRouteSearchResult route = FindWeightedGameplayRoute(
                    routeGraph, transition.startNodeId, transition.goalNodeId);
                if (!route.Succeeded() || !route.totalCost.has_value() ||
                    !std::isfinite(*route.totalCost) || *route.totalCost < 0.0f)
                {
                    return std::nullopt;
                }
                AIActionDefinition action{.actionId=kAIMoveToActionId,
                    .preconditions={{SpatialFact(transition.source), true}},
                    .effects={{SpatialFact(transition.source), false},
                        {SpatialFact(transition.target), true}},
                    .contextId=transition.contextId, .baseCost=*route.totalCost};
                if (transition.target == SpatialLocation::CoinA ||
                    transition.target == SpatialLocation::CoinB ||
                    transition.target == SpatialLocation::CoinC)
                {
                    const std::size_t coinIndex =
                        static_cast<std::size_t>(transition.target) - 1u;
                    constexpr std::array collectedFacts{kGOAPCoinACollectedFact,
                        kGOAPCoinBCollectedFact, kGOAPCoinCCollectedFact};
                    constexpr std::array availableFacts{kGOAPCoinAAvailableFact,
                        kGOAPCoinBAvailableFact, kGOAPCoinCAvailableFact};
                    action.preconditions.push_back({collectedFacts[coinIndex], false});
                    action.preconditions.push_back({availableFacts[coinIndex], true});
                    action.effects.push_back({collectedFacts[coinIndex], true});
                    action.numericEffects.push_back(
                        {kGOAPCoinCountFact, AINumericEffectOperation::Add, 1});
                }
                else if (transition.target == SpatialLocation::AccessKeyShop)
                {
                    action.preconditions.push_back({kGOAPHasAccessKeyFact, false});
                    action.numericPreconditions.push_back({kGOAPCoinCountFact,
                        AINumericConditionOperator::GreaterOrEqual, kAccessKeyPrice});
                }
                else if (transition.target == SpatialLocation::Goal)
                {
                    action.preconditions.push_back({kGOAPHasAccessKeyFact, true});
                    action.preconditions.push_back({kGOAPAtDestinationFact, false});
                    action.effects.push_back({kGOAPAtDestinationFact, true});
                }
                definition.actions.push_back(std::move(action));
            }
            definition.actions.push_back(AIActionDefinition{.actionId=kAIBuyKeyActionId,
                .preconditions={AIFactCondition{kGOAPHasAccessKeyFact, false},
                    AIFactCondition{kGOAPAtAccessKeyShopFact, true}},
                .effects={AIFactEffect{kGOAPHasAccessKeyFact, true}},
                .baseCost=1.0f,
                .numericPreconditions={{kGOAPCoinCountFact,
                    AINumericConditionOperator::GreaterOrEqual, kAccessKeyPrice}},
                .numericEffects={{kGOAPCoinCountFact,
                    AINumericEffectOperation::Add, -kAccessKeyPrice}}});
            return definition;
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
            GameplayWorld& world, GameplayRouteGraph routeGraph)
        {
            return AccessKeyMoveToRequestProvider{world, std::move(routeGraph)};
        }
        
        class BuyKeyActionRuntime final : public IAIActionRuntime
        {
        public:
            BuyKeyActionRuntime(AIAgentWorldState& facts, const GameplayWorld& world,
                std::vector<GameplayWorldEvent>& events, const EntityHandle keyEntity,
                const mathUtils::Vec3 keyPosition) noexcept
                : facts_(facts), world_(world), events_(events), keyEntity_(keyEntity),
                  keyPosition_(keyPosition)
            {
            }

            [[nodiscard]] AIActionRuntimeResult Start(
                const AIActionRuntimeContext& context) override
            {
                if (context.actionId != kAIBuyKeyActionId || context.agentEntity == kNullEntity ||
                    keyEntity_ == kNullEntity || facts_.IsFactSet(kGOAPHasAccessKeyFact) ||
                    facts_.GetIntegerFact(kGOAPCoinCountFact) < kAccessKeyPrice)
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
            bool committed_{};
        };

        class BuyKeyActionBinding final : public IAIActionBinding
        {
        public:
            BuyKeyActionBinding(AIAgentWorldState& facts, const GameplayWorld& world,
                const EntityHandle keyEntity, const mathUtils::Vec3 keyPosition) noexcept
                : facts_(facts), world_(world), keyEntity_(keyEntity), keyPosition_(keyPosition)
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
                    facts_, world_, *events_, keyEntity_, keyPosition_);
            }

        private:
            AIAgentWorldState& facts_;
            const GameplayWorld& world_;
            EntityHandle keyEntity_{kNullEntity};
            mathUtils::Vec3 keyPosition_{};
            std::vector<GameplayWorldEvent>* events_{};
        };

        class AccessKeyDecision final : public GameplayAIDecisionInstance
        {
        public:
            AccessKeyDecision(const EntityHandle agent, GameplayGOAPDecisionDefinition definition,
                AccessKeyMoveToRequestProvider moveToRequests,
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                const EntityHandle coinAEntity, const EntityHandle coinBEntity,
                const EntityHandle coinCEntity, const EntityHandle keyEntity,
                const std::array<mathUtils::Vec3, kSpatialLocationCount>& spatialPositions)
                : moveToRequests_(std::move(moveToRequests)),
                    goap_(agent, std::move(definition)), agent_(agent), coinAEntity_(coinAEntity),
                    coinBEntity_(coinBEntity), coinCEntity_(coinCEntity), keyEntity_(keyEntity),
                    spatialPositions_(spatialPositions)
            {
                auto binding = std::make_unique<AIMoveToActionBinding>(world,
                    traversalLinkRegistry, traversalExecutorRegistry, moveToRequests_);
                bindingsInstalled_ = goap_.InstallActionBinding(
                    kAIMoveToActionId, std::move(binding));
                auto buyKeyBinding = std::make_unique<BuyKeyActionBinding>(
                goap_.GetObservedState(), world, keyEntity_,
                    spatialPositions_[static_cast<std::size_t>(SpatialLocation::AccessKeyShop)]);
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
                        facts.SetIntegerFact(kGOAPCoinCountFact,
                            facts.GetIntegerFact(kGOAPCoinCountFact) + 1);
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
                        collectCoin(event.subject, coinAEntity_, kGOAPCoinACollectedFact);
                        collectCoin(event.subject, coinBEntity_, kGOAPCoinBCollectedFact);
                        collectCoin(event.subject, coinCEntity_, kGOAPCoinCCollectedFact);
                    }
                    else if (event.type == GameplayWorldEventType::AccessKeyPurchased &&
                        event.subject == keyEntity_ &&
                        !facts.IsFactSet(kGOAPHasAccessKeyFact))
                    {
                        const std::int32_t coins = facts.GetIntegerFact(kGOAPCoinCountFact);
                        if (coins >= kAccessKeyPrice)
                        {
                            facts.SetIntegerFact(kGOAPCoinCountFact, coins - kAccessKeyPrice);
                            facts.SetFact(kGOAPHasAccessKeyFact, true);
                        }
                    }
                }
                
                const auto observeAvailability = [&](const EntityHandle coin,
                    const AIWorldFactId availableFact)
                {
                    const GameplayPickupComponent* pickup = world.TryGetPickup(coin);
                    facts.SetFact(availableFact,
                        world.IsEntityValid(coin) && pickup != nullptr && !pickup->collected);
                };
                observeAvailability(coinAEntity_, kGOAPCoinAAvailableFact);
                observeAvailability(coinBEntity_, kGOAPCoinBAvailableFact);
                observeAvailability(coinCEntity_, kGOAPCoinCAvailableFact);

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
                        facts.SetFact(SpatialFact(static_cast<SpatialLocation>(index)),
                            index == static_cast<std::size_t>(*confirmedLocation));
                    }
                    if (*confirmedLocation == SpatialLocation::Goal &&
                        facts.IsFactSet(kGOAPHasAccessKeyFact))
                    {
                        facts.SetFact(kGOAPAtDestinationFact, true);
                    }
                }
            }

            AccessKeyMoveToRequestProvider moveToRequests_;
            GameplayGOAPDecision goap_;
            EntityHandle agent_{};
            EntityHandle coinAEntity_{ kNullEntity };
            EntityHandle coinBEntity_{ kNullEntity };
            EntityHandle coinCEntity_{ kNullEntity };
            EntityHandle keyEntity_{ kNullEntity };
            std::array<mathUtils::Vec3, kSpatialLocationCount> spatialPositions_{};
            BuyKeyActionBinding* buyKeyBinding_{};
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
        const GameplayTraversalExecutorRegistry& traversalExecutorRegistry)
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
        std::optional<GameplayGOAPDecisionDefinition> definition =
            ai_access_key_detail::BuildDefinition(routeGraph);
        if (!definition.has_value())
        {
            return nullptr;
        }
        auto decision = std::make_unique<ai_access_key_detail::AccessKeyDecision>(agent,
        std::move(*definition),
         ai_access_key_detail::BuildMoveToProvider(world, std::move(routeGraph)),
                world, traversalLinkRegistry, traversalExecutorRegistry, coinAEntity,
                 coinBEntity, coinCEntity, keyEntity,
                 std::array{startPosition, coinAPosition, coinBPosition, coinCPosition,
                     keyPosition, goalPosition});
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}