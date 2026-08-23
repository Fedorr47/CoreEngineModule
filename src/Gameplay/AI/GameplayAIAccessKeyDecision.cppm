#include <memory>
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
                    .contextId=kGOAPCoinAMoveContext, .baseCost=1.0f},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinBCollectedFact, false}},
                    .effects={AIFactEffect{kGOAPCoinBCollectedFact, true}},
                    .contextId=kGOAPCoinBMoveContext, .baseCost=1.0f},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinCCollectedFact, false}},
                    .effects={AIFactEffect{kGOAPCoinCCollectedFact, true}},
                    .contextId=kGOAPCoinCMoveContext, .baseCost=1.0f},
                AIActionDefinition{.actionId=kAIMoveToActionId,
                    .preconditions={AIFactCondition{kGOAPCoinACollectedFact, true},
                        AIFactCondition{kGOAPCoinBCollectedFact, true},
                        AIFactCondition{kGOAPCoinCCollectedFact, true},
                        AIFactCondition{kGOAPHasAccessKeyFact, false}},
                    .effects={AIFactEffect{kGOAPHasAccessKeyFact, true}},
                    .contextId=kGOAPAccessKeyMoveContext, .baseCost=1.0f},
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

        class AccessKeyDecision final : public GameplayAIDecisionInstance
        {
        public:
            AccessKeyDecision(const EntityHandle agent, GameplayGOAPDecisionDefinition definition,
                AccessKeyMoveToRequestProvider moveToRequests,
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinkRegistry,
                const GameplayTraversalExecutorRegistry& traversalExecutorRegistry,
                const mathUtils::Vec3 coinA, const mathUtils::Vec3 coinB,
                const mathUtils::Vec3 coinC, const mathUtils::Vec3 key,
                const mathUtils::Vec3 goal)
                : moveToRequests_(std::move(moveToRequests)),
                  goap_(agent, std::move(definition)), agent_(agent), coinAPosition_(coinA),
                  coinBPosition_(coinB), coinCPosition_(coinC), accessKeyPosition_(key),
                  finalGoalPosition_(goal)
            {
                auto binding = std::make_unique<AIMoveToActionBinding>(world,
                    traversalLinkRegistry, traversalExecutorRegistry, moveToRequests_);
                bindingsInstalled_ = goap_.InstallActionBinding(
                    kAIMoveToActionId, std::move(binding));
            }

            [[nodiscard]] bool IsConfigured() const noexcept
            {
                return bindingsInstalled_;
            }

            void Update(AISystem& aiSystem, const GameplayWorld& world) override
            {
                Observe_(world, goap_.GetObservedState());
                goap_.Update(aiSystem, world);
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
            void Observe_(const GameplayWorld& world, AIAgentWorldState& facts)
            {
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
                const auto observeCoin = [&](const AIWorldFactId fact,
                    const mathUtils::Vec3 position)
                {
                    if (!facts.IsFactSet(fact) && distanceSquared(position) <= 0.36f)
                    {
                        facts.SetFact(fact, true);
                    }
                };
                observeCoin(kGOAPCoinACollectedFact, coinAPosition_);
                observeCoin(kGOAPCoinBCollectedFact, coinBPosition_);
                observeCoin(kGOAPCoinCCollectedFact, coinCPosition_);
                const bool hasAllCoins = facts.IsFactSet(kGOAPCoinACollectedFact) &&
                    facts.IsFactSet(kGOAPCoinBCollectedFact) &&
                    facts.IsFactSet(kGOAPCoinCCollectedFact);
                if (hasAllCoins && !facts.IsFactSet(kGOAPHasAccessKeyFact) &&
                    distanceSquared(accessKeyPosition_) <= 0.36f)
                {
                    facts.SetFact(kGOAPHasAccessKeyFact, true);
                }
                if (facts.IsFactSet(kGOAPHasAccessKeyFact) &&
                    distanceSquared(finalGoalPosition_) <= 0.36f)
                {
                    facts.SetFact(kGOAPAtDestinationFact, true);
                }
            }

            AccessKeyMoveToRequestProvider moveToRequests_;
            GameplayGOAPDecision goap_;
            EntityHandle agent_{};
            mathUtils::Vec3 coinAPosition_{};
            mathUtils::Vec3 coinBPosition_{};
            mathUtils::Vec3 coinCPosition_{};
            mathUtils::Vec3 accessKeyPosition_{};
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
        auto decision = std::make_unique<ai_access_key_detail::AccessKeyDecision>(agent,
            ai_access_key_detail::BuildDefinition(),
            ai_access_key_detail::BuildMoveToProvider(world, startPosition, coinAPosition,
                coinBPosition, coinCPosition, keyPosition, goalPosition),
            world, traversalLinkRegistry, traversalExecutorRegistry, coinAPosition,
            coinBPosition, coinCPosition, keyPosition, goalPosition);
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}