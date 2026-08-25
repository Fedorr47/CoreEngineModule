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
    inline constexpr AIWorldIntegerFactId kGOAPCoinCountFact{0u};
    inline constexpr std::int32_t kAccessKeyPrice = 3;
    inline constexpr AIActionId kAIBuyKeyActionId{3u};
    inline constexpr AIActionContextId kGOAPAccessKeyMoveContext{1u};
    inline constexpr AIActionContextId kGOAPFinalGoalMoveContext{2u};
    inline constexpr AIActionContextId kGOAPCoinAMoveContext{3u};
    inline constexpr AIActionContextId kGOAPCoinBMoveContext{4u};
    inline constexpr AIActionContextId kGOAPCoinCMoveContext{5u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{50u};

    namespace ai_access_key_detail
    {
        struct MoveTarget
        {
            AIActionContextId contextId{};
            GameplayRouteNodeId goalNodeId{};
        };

        class AccessKeyMoveToRequestProvider final :
            public IAIMoveToActionRequestProvider
        {
        public:
            AccessKeyMoveToRequestProvider(GameplayWorld& world, GameplayRouteGraph routeGraph,
                std::vector<MoveTarget> targets)
                : world_(world), routeGraph_(std::move(routeGraph)),
                  targets_(std::move(targets))
            {
            }

            std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                for (const MoveTarget& target : targets_)
                {
                    if (target.contextId == context.contextId)
                    {
                        const auto* transform = world_.TryGetTransform(context.agentEntity);
                        if (transform == nullptr)
                        {
                            return std::nullopt;
                        }
                        GameplayRouteNodeId startNode{};
                        float nearestDistanceSquared = std::numeric_limits<float>::max();
                        for (const GameplayRouteGraphNode& node : routeGraph_.nodes)
                        {
                            const mathUtils::Vec3 delta =
                                transform->position - node.worldPosition;
                            const float distanceSquared = mathUtils::Dot(delta, delta);
                            if (distanceSquared < nearestDistanceSquared)
                            {
                                nearestDistanceSquared = distanceSquared;
                                startNode = node.nodeId;
                            }
                        }
                        if (!startNode.IsValid())
                        {
                            return std::nullopt;
                        }
                        GameplayArrivalSteeringSettings steering{};
                        steering.acceptanceRadius=0.35f;
                        steering.slowingRadius=1.25f;
                        return AIMoveToActionRequest{&routeGraph_, startNode,
                            target.goalNodeId, steering};
                    }
                }
                return std::nullopt;
            }

        private:
            GameplayWorld& world_;
            GameplayRouteGraph routeGraph_{};
            std::vector<MoveTarget> targets_{};
        };

        inline constexpr GameplayRouteNodeId kStartNode{1u};
        inline constexpr GameplayRouteNodeId kCoinANode{2u};
        inline constexpr GameplayRouteNodeId kCoinBNode{3u};
        inline constexpr GameplayRouteNodeId kCoinCNode{4u};
        inline constexpr GameplayRouteNodeId kAccessKeyNode{5u};
        inline constexpr GameplayRouteNodeId kFinalGoalNode{6u};

        [[nodiscard]] GameplayGOAPDecisionDefinition BuildDefinition()
        {
            GameplayGOAPDecisionDefinition definition{};
            definition.goals = {AIGoalSelectionCandidate{
                .goal=AIGoalDefinition{kGOAPReachDestinationGoal,
                    {{kGOAPAtDestinationFact, true}}}, .baseScore=1.0f}};
            definition.actions = {
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinACollectedFact, false}},
                    .effects={AIFactEffect{kGOAPCoinACollectedFact, true}},
                    .contextId=kGOAPCoinAMoveContext, .baseCost=1.0f,
                    .numericEffects={{kGOAPCoinCountFact, AINumericEffectOperation::Add, 1}}},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinBCollectedFact, false}},
                    .effects={AIFactEffect{kGOAPCoinBCollectedFact, true}},
                    .contextId=kGOAPCoinBMoveContext, .baseCost=1.0f,
                    .numericEffects={{kGOAPCoinCountFact, AINumericEffectOperation::Add, 1}}},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinCCollectedFact, false}},
                    .effects={AIFactEffect{kGOAPCoinCCollectedFact, true}},
                    .contextId=kGOAPCoinCMoveContext, .baseCost=1.0f,
                    .numericEffects={{kGOAPCoinCountFact, AINumericEffectOperation::Add, 1}}},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinACollectedFact, true},
                        AIFactCondition{kGOAPCoinBCollectedFact, true},
                        AIFactCondition{kGOAPCoinCCollectedFact, true},
                        AIFactCondition{kGOAPHasAccessKeyFact, false}},
                    .effects={AIFactEffect{kGOAPAtAccessKeyShopFact, true}},
                    .contextId=kGOAPAccessKeyMoveContext, .baseCost=1.0f,
                    .numericPreconditions={{kGOAPCoinCountFact,
                        AINumericConditionOperator::GreaterOrEqual, kAccessKeyPrice}}},
                AIActionDefinition{.actionId=kAIBuyKeyActionId,
                    .preconditions={AIFactCondition{kGOAPHasAccessKeyFact, false},
                        AIFactCondition{kGOAPAtAccessKeyShopFact, true}},
                    .effects={AIFactEffect{kGOAPHasAccessKeyFact, true}},
                    .baseCost=1.0f,
                    .numericPreconditions={{kGOAPCoinCountFact,
                        AINumericConditionOperator::GreaterOrEqual, kAccessKeyPrice}},
                    .numericEffects={{kGOAPCoinCountFact,
                        AINumericEffectOperation::Add, -kAccessKeyPrice}}},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPHasAccessKeyFact, true},
                        AIFactCondition{kGOAPAtDestinationFact, false}},
                    .effects={AIFactEffect{kGOAPAtDestinationFact, true}},
                    .contextId=kGOAPFinalGoalMoveContext, .baseCost=1.0f}};
            return definition;
        }

        [[nodiscard]] AccessKeyMoveToRequestProvider BuildMoveToProvider(
            GameplayWorld& world,
            const mathUtils::Vec3 start, const mathUtils::Vec3 coinA,
            const mathUtils::Vec3 coinB, const mathUtils::Vec3 coinC,
            const mathUtils::Vec3 key, const mathUtils::Vec3 goal)
        {
            GameplayRouteGraph graph{};
            graph.nodes = {{kStartNode, start}, {kCoinANode, coinA}, {kCoinBNode, coinB},
                {kCoinCNode, coinC}, {kAccessKeyNode, key}, {kFinalGoalNode, goal}};
            for (const GameplayRouteGraphNode& from : graph.nodes)
            {
                for (const GameplayRouteGraphNode& to : graph.nodes)
                {
                    if (from.nodeId != to.nodeId)
                    {
                        graph.edges.push_back({from.nodeId, to.nodeId, 1.0f});
                    }
                }
            }
            std::vector<MoveTarget> targets{
                {kGOAPCoinAMoveContext, kCoinANode},
                {kGOAPCoinBMoveContext, kCoinBNode},
                {kGOAPCoinCMoveContext, kCoinCNode},
                {kGOAPAccessKeyMoveContext, kAccessKeyNode},
                {kGOAPFinalGoalMoveContext, kFinalGoalNode}};
            return AccessKeyMoveToRequestProvider{
                world, std::move(graph), std::move(targets)};
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
                const mathUtils::Vec3 key, const mathUtils::Vec3 goal)
                : moveToRequests_(std::move(moveToRequests)),
                    goap_(agent, std::move(definition)), agent_(agent), coinAEntity_(coinAEntity),
                    coinBEntity_(coinBEntity), coinCEntity_(coinCEntity), keyEntity_(keyEntity),
                    accessKeyPosition_(key), finalGoalPosition_(goal)
            {
                auto binding = std::make_unique<AIMoveToActionBinding>(world,
                    traversalLinkRegistry, traversalExecutorRegistry, moveToRequests_);
                bindingsInstalled_ = goap_.InstallActionBinding(
                    kAIMoveToActionId, std::move(binding));
                auto buyKeyBinding = std::make_unique<BuyKeyActionBinding>(
                    goap_.GetObservedState(), world, keyEntity_, accessKeyPosition_);
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

                const auto* transform = world.TryGetTransform(agent_);
                if (transform == nullptr)
                {
                    return;
                }
                const auto distanceSquared = [&](const mathUtils::Vec3 target)
                {
                    const mathUtils::Vec3 delta = transform->position - target;
                    return mathUtils::Dot(delta, delta);
                };
                if (facts.IsFactSet(kGOAPHasAccessKeyFact) &&
                    distanceSquared(finalGoalPosition_) <= 0.36f)
                {
                    facts.SetFact(kGOAPAtDestinationFact, true);
                }
            }

            AccessKeyMoveToRequestProvider moveToRequests_;
            GameplayGOAPDecision goap_;
            EntityHandle agent_{};
            EntityHandle coinAEntity_{ kNullEntity };
            EntityHandle coinBEntity_{ kNullEntity };
            EntityHandle coinCEntity_{ kNullEntity };
            EntityHandle keyEntity_{ kNullEntity };
            mathUtils::Vec3 accessKeyPosition_{};
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
        auto decision = std::make_unique<ai_access_key_detail::AccessKeyDecision>(agent,
            ai_access_key_detail::BuildDefinition(),
            ai_access_key_detail::BuildMoveToProvider(world, startPosition, coinAPosition,
                coinBPosition, coinCPosition, keyPosition, goalPosition),
           	world, traversalLinkRegistry, traversalExecutorRegistry, coinAEntity,
           	coinBEntity, coinCEntity, keyEntity, keyPosition, goalPosition);
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}