module;

#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

export module core:gameplay_ai_target_recovery_decision;

import :gameplay;
import :gameplay_ai_decision;
import :gameplay_goap_decision;
import :gameplay_goap_inspection;
import :ai_move_to_action_binding;

export namespace rendern
{
    inline constexpr std::string_view kTargetRecoveryAIDecisionId{"target_recovery"};
    inline constexpr AIWorldFactId kTargetRecoveryAtDestinationFact{0u};
    inline constexpr AIWorldFactId kTargetRecoveryGoalAvailableFact{1u};

    namespace ai_target_recovery_detail
    {
        inline constexpr AIGoalId kReachDestinationGoalId{1u};
        inline constexpr AIActionContextId kMoveToDestinationContextId{1u};
        inline constexpr GameplayRouteNodeId kStartRouteNodeId{1u};
        inline constexpr GameplayRouteNodeId kGoalRouteNodeId{2u};
        inline constexpr float kArrivalRadius = 0.4f;

        class TargetRecoveryMoveToRequestProvider final : public IAIMoveToActionRequestProvider
        {
        public:
            TargetRecoveryMoveToRequestProvider(
                const mathUtils::Vec3 startPosition,
                const mathUtils::Vec3 goalPosition)
            {
                routeGraph_.nodes = {
                    {kStartRouteNodeId, startPosition},
                    {kGoalRouteNodeId, goalPosition}};
                routeGraph_.edges = {
                    {kStartRouteNodeId, kGoalRouteNodeId, 1.0f, {}}};
            }

            [[nodiscard]] std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                if (context.contextId != kMoveToDestinationContextId)
                {
                    return std::nullopt;
                }

                GameplayArrivalSteeringSettings steering{};
                steering.acceptanceRadius = kArrivalRadius;
                steering.slowingRadius = 1.5f;
                steering.wantsRun = false;
                return AIMoveToActionRequest{
                    &routeGraph_, kStartRouteNodeId, kGoalRouteNodeId, steering};
            }

        private:
            GameplayRouteGraph routeGraph_{};
        };

        class TargetRecoveryDecision final :
            public GameplayAIDecisionInstance,
            public IGameplayGOAPInspection
        {
        public:
            TargetRecoveryDecision(
                const EntityHandle agentEntity,
                const EntityHandle targetEntity,
                const mathUtils::Vec3 startPosition,
                const mathUtils::Vec3 goalPosition,
                GameplayWorld& world,
                const GameplayTraversalLinkRegistry& traversalLinks,
                const GameplayTraversalExecutorRegistry& traversalExecutors)
                : agentEntity_(agentEntity),
                  targetEntity_(targetEntity),
                  goalPosition_(goalPosition),
                  moveToRequests_(startPosition, goalPosition),
                  goap_(agentEntity, BuildDefinition_())
            {
                configured_ = goap_.InstallActionBinding(
                    kAIMoveToActionId,
                    std::make_unique<AIMoveToActionBinding>(
                        world, traversalLinks, traversalExecutors, moveToRequests_));
            }

            void Update(
                AISystem& aiSystem,
                const GameplayAIObservationContext& observation) override
            {
                AIAgentWorldState& facts = goap_.GetObservedState();
                const bool goalAvailable = observation.world.IsEntityValid(targetEntity_)
                    && observation.world.HasInteractionPoint(targetEntity_);
                facts.SetFact(kTargetRecoveryGoalAvailableFact, goalAvailable);

                const GameplayTransformComponent* agentTransform =
                    observation.world.TryGetTransform(agentEntity_);
                bool atDestination = false;
                if (goalAvailable && agentTransform != nullptr)
                {
                    const mathUtils::Vec3 offset = agentTransform->position - goalPosition_;
                    atDestination = mathUtils::Dot(offset, offset)
                        <= kArrivalRadius * kArrivalRadius;
                }
                facts.SetFact(kTargetRecoveryAtDestinationFact, atDestination);
                goap_.Update(aiSystem, observation.world);
            }

            void Cancel(AISystem& aiSystem) noexcept override
            {
                goap_.Cancel(aiSystem);
            }

            [[nodiscard]] GameplayAIDecisionStatus GetStatus() const noexcept override
            {
                switch (goap_.GetStatus())
                {
                case AIPlanExecutionStatus::NotStarted:
                    return GameplayAIDecisionStatus::NotStarted;
                case AIPlanExecutionStatus::ReadyToStartStep: [[fallthrough]];
                case AIPlanExecutionStatus::RunningStep:
                    return GameplayAIDecisionStatus::Running;
                case AIPlanExecutionStatus::Succeeded:
                    return GameplayAIDecisionStatus::Succeeded;
                case AIPlanExecutionStatus::Cancelled:
                    return GameplayAIDecisionStatus::Cancelled;
                case AIPlanExecutionStatus::Failed:
                    return GameplayAIDecisionStatus::Failed;
                }
                return GameplayAIDecisionStatus::Failed;
            }

            [[nodiscard]] AIPlanExecutionStatus GetGOAPStatus() const noexcept override
            {
                return goap_.GetStatus();
            }

            [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept override
            {
                return goap_.GetObservedState();
            }

            [[nodiscard]] AIDebugViewModel BuildDebugViewModel() const override
            {
                return goap_.BuildDebugViewModel();
            }

            [[nodiscard]] bool IsConfigured() const noexcept
            {
                return configured_;
            }

        private:
            [[nodiscard]] static GameplayGOAPDecisionDefinition BuildDefinition_()
            {
                GameplayGOAPDecisionDefinition definition{};
                definition.goals.push_back({
                    {kReachDestinationGoalId,
                        {{kTargetRecoveryAtDestinationFact, true}}},
                    1.0f,
                    {}});
                definition.actions.push_back(AIActionDefinition{
                    .actionId=kAIMoveToActionId,
                    .preconditions={{kTargetRecoveryGoalAvailableFact, true}},
                    .effects={{kTargetRecoveryAtDestinationFact, true}},
                    .contextId=kMoveToDestinationContextId,
                    .baseCost=1.0f,
                    .continuationConditions={
                        {kTargetRecoveryGoalAvailableFact, true}}});
                definition.metadata.booleanFacts = {
                    {kTargetRecoveryAtDestinationFact, "AtDestination"},
                    {kTargetRecoveryGoalAvailableFact, "GoalAvailable"}};
                definition.metadata.goals = {
                    {kReachDestinationGoalId, "ReachDestination"}};
                definition.metadata.actions = {
                    {kAIMoveToActionId, kMoveToDestinationContextId,
                        "MoveTo", "Destination"}};
                return definition;
            }

            EntityHandle agentEntity_{kNullEntity};
            EntityHandle targetEntity_{kNullEntity};
            mathUtils::Vec3 goalPosition_{};
            TargetRecoveryMoveToRequestProvider moveToRequests_;
            GameplayGOAPDecision goap_;
            bool configured_{};
        };

        [[nodiscard]] EntityHandle FindNodeEntity(
            const GameplayWorld& world,
            const int nodeIndex)
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

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance>
        CreateTargetRecoveryAIDecision(
            const EntityHandle agentEntity,
            const LevelAsset& level,
            GameplayWorld& world,
            const GameplayTraversalLinkRegistry& traversalLinks,
            const GameplayTraversalExecutorRegistry& traversalExecutors)
    {
        const auto startNode = std::ranges::find_if(level.nodes,
            [](const LevelNode& node)
            {
                return node.alive && node.name == "GOAP_Recovery_Start";
            });
        const auto goalNode = std::ranges::find_if(level.nodes,
            [](const LevelNode& node)
            {
                return node.alive && node.name == "GOAP_Recovery_Goal";
            });
        if (startNode == level.nodes.end() || goalNode == level.nodes.end())
        {
            return nullptr;
        }

        const int goalNodeIndex = static_cast<int>(
            std::distance(level.nodes.begin(), goalNode));
        const EntityHandle targetEntity = ai_target_recovery_detail::FindNodeEntity(
            world, goalNodeIndex);
        if (targetEntity == kNullEntity || !world.HasInteractionPoint(targetEntity))
        {
            return nullptr;
        }

        auto decision = std::make_unique<ai_target_recovery_detail::TargetRecoveryDecision>(
            agentEntity,
            targetEntity,
            startNode->transform.position,
            goalNode->transform.position,
            world,
            traversalLinks,
            traversalExecutors);
        if (!decision->IsConfigured())
        {
            return nullptr;
        }
        return decision;
    }
}