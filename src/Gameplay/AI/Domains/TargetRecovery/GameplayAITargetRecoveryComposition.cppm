module;

#include <algorithm>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <string_view>
#include <vector>

export module core:gameplay_ai_target_recovery_composition;

import :gameplay;
import :gameplay_ai_decision_contracts;
import :level;
import :gameplay_traversal_link_registry;
import :gameplay_traversal_executor_registry;
import :gameplay_goap_decision_instance;
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

        class TargetRecoveryGOAPContext final : public IGameplayGOAPContext
        {
        public:
            TargetRecoveryGOAPContext(const EntityHandle agent, const EntityHandle target,
                const mathUtils::Vec3 start, const mathUtils::Vec3 goal)
                : agentEntity_(agent), targetEntity_(target), goalPosition_(goal),
                  moveToRequests_(start, goal)
            {
            }

            void Observe(const GameplayWorld& world, std::span<const GameplayWorldEvent>,
                AIAgentWorldState& facts) override
            {
                const bool goalAvailable = world.IsEntityValid(targetEntity_)
                    && world.HasInteractionPoint(targetEntity_);
                facts.SetFact(kTargetRecoveryGoalAvailableFact, goalAvailable);
                const GameplayTransformComponent* agentTransform = world.TryGetTransform(agentEntity_);
                bool atDestination = false;
                if (goalAvailable && agentTransform != nullptr)
                {
                    const mathUtils::Vec3 offset = agentTransform->position - goalPosition_;
                    atDestination = mathUtils::Dot(offset, offset) <= kArrivalRadius * kArrivalRadius;
                }
                facts.SetFact(kTargetRecoveryAtDestinationFact, atDestination);
            }

            [[nodiscard]] TargetRecoveryMoveToRequestProvider& MoveToRequests() noexcept
            {
                return moveToRequests_;
            }

        private:
            EntityHandle agentEntity_{kNullEntity};
            EntityHandle targetEntity_{kNullEntity};
            mathUtils::Vec3 goalPosition_{};
            TargetRecoveryMoveToRequestProvider moveToRequests_;
        };

        [[nodiscard]] GameplayGOAPDecisionDefinition BuildDefinition()
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

        auto domain = std::make_unique<ai_target_recovery_detail::TargetRecoveryGOAPContext>(
            agentEntity, targetEntity, startNode->transform.position, goalNode->transform.position);
        auto* requests = &domain->MoveToRequests();
        GameplayGOAPDecisionSetup setup{
            .definition = ai_target_recovery_detail::BuildDefinition(),
            .context = std::move(domain)};
        setup.actionBindings.push_back({kAIMoveToActionId,
            [&world, &traversalLinks, &traversalExecutors, requests]
            (AIAgentWorldState&, std::vector<GameplayWorldEvent>&)
            {
                return std::make_unique<AIMoveToActionBinding>(
                    world, traversalLinks, traversalExecutors, *requests);
            }});
        return CreateGameplayGOAPDecision(agentEntity, std::move(setup));
    }
}
