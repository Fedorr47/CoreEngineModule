module;

#include <array>
#include <memory>
#include <optional>
#include <string_view>

export module core:gameplay_ai_decision;

import :ai_decision_runtime;
import :ai_move_to_action_binding;
import :ai_system;
import :gameplay;
import :gameplay_route;
import :level;

export namespace rendern
{
    inline constexpr std::string_view kAccessKeyAIDecisionId{"access_key"};
    inline constexpr AIWorldFactId kGOAPHasAccessKeyFact{40u};
    inline constexpr AIWorldFactId kGOAPAtDestinationFact{41u};
    inline constexpr AIActionContextId kGOAPAccessKeyMoveContext{1u};
    inline constexpr AIActionContextId kGOAPFinalGoalMoveContext{2u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{50u};

    class GameplayAIDecisionInstance : public IAIMoveToActionRequestProvider
    {
    public:
        virtual ~GameplayAIDecisionInstance() = default;
        [[nodiscard]] virtual bool InstallMoveToBinding(
            std::unique_ptr<AIMoveToActionBinding> binding) = 0;
        virtual void Update(AISystem& aiSystem, const GameplayWorld& world) = 0;
        virtual void Cancel(AISystem& aiSystem) noexcept = 0;
        [[nodiscard]] virtual AIPlanExecutionStatus GetStatus() const noexcept = 0;
        [[nodiscard]] virtual const AIAgentWorldState& GetObservedState() const noexcept = 0;
    };

    [[nodiscard]] bool IsGameplayAIDecisionDefinitionRegistered(
        const std::string_view definitionId) noexcept
    {
        return definitionId == kAccessKeyAIDecisionId;
    }

    namespace ai_decision_detail
    {
        class AccessKeyDecision final : public GameplayAIDecisionInstance
        {
        public:
            AccessKeyDecision(const EntityHandle agent, const mathUtils::Vec3 start,
                const mathUtils::Vec3 key, const mathUtils::Vec3 goal)
                : decision_(agent)
            {
                graph_.nodes = {{GameplayRouteNodeId{1u}, key}, {GameplayRouteNodeId{2u}, start},
                    {GameplayRouteNodeId{3u}, goal}};
                graph_.edges = {{GameplayRouteNodeId{2u}, GameplayRouteNodeId{1u}, 1.0f},
                    {GameplayRouteNodeId{1u}, GameplayRouteNodeId{2u}, 1.0f},
                    {GameplayRouteNodeId{2u}, GameplayRouteNodeId{3u}, 1.0f},
                    {GameplayRouteNodeId{1u}, GameplayRouteNodeId{3u}, 2.0f}};
            }

            bool InstallMoveToBinding(std::unique_ptr<AIMoveToActionBinding> binding) override
            {
                bindings_.Reset();
                if (!binding || !bindings_.Register(kAIMoveToActionId, *binding))
                {
                    return false;
                }
                binding_ = std::move(binding);
                return true;
            }

            void Update(AISystem& aiSystem, const GameplayWorld& world) override
            {
                Observe_(world);
                const std::array candidates{AIGoalSelectionCandidate{
                    .goal=AIGoalDefinition{kGOAPReachDestinationGoal, {{kGOAPAtDestinationFact, true}}},
                    .baseScore=1.0f}};
                const std::array<AIActionDefinition, 2> actions{
                    AIActionDefinition{.actionId=kAIMoveToActionId,
                        .preconditions={AIFactCondition{kGOAPHasAccessKeyFact, false}},
                        .effects={AIFactEffect{kGOAPHasAccessKeyFact, true}},
                        .contextId=kGOAPAccessKeyMoveContext, .baseCost=1.0f},
                    AIActionDefinition{.actionId=kAIMoveToActionId,
                        .preconditions={AIFactCondition{kGOAPHasAccessKeyFact, true},
                            AIFactCondition{kGOAPAtDestinationFact, false}},
                        .effects={AIFactEffect{kGOAPAtDestinationFact, true}},
                        .contextId=kGOAPFinalGoalMoveContext, .baseCost=1.0f}};
                (void)decision_.Update(facts_, candidates, actions, bindings_, aiSystem, world);
            }

            void Cancel(AISystem& aiSystem) noexcept override
            {
                decision_.Cancel(aiSystem);
                bindings_.Reset();
                binding_.reset();
            }

            [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept override
            {
                return decision_.GetStatus();
            }
            [[nodiscard]] const AIAgentWorldState& GetObservedState() const noexcept override
            {
                return facts_;
            }

        private:
            std::optional<AIMoveToActionRequest> ResolveRequest(
                const AIActionRuntimeContext& context) override
            {
                GameplayRouteNodeId start{};
                GameplayRouteNodeId goal{};
                if (context.contextId == kGOAPAccessKeyMoveContext)
                {
                    start = GameplayRouteNodeId{2u}; goal = GameplayRouteNodeId{1u};
                }
                else if (context.contextId == kGOAPFinalGoalMoveContext)
                {
                    start = GameplayRouteNodeId{1u}; goal = GameplayRouteNodeId{3u};
                }
                else
                {
                    return std::nullopt;
                }
                GameplayArrivalSteeringSettings steering{};
                steering.acceptanceRadius=0.35f; steering.slowingRadius=1.25f;
                return AIMoveToActionRequest{&graph_, start, goal, steering};
            }

            void Observe_(const GameplayWorld& world)
            {
                const auto* transform = world.TryGetTransform(decision_.GetAgentEntity());
                if (transform == nullptr)
                {
                    return;
                }
                const auto distanceSquared = [&](const mathUtils::Vec3 target)
                {
                    const mathUtils::Vec3 delta = transform->position - target;
                    return mathUtils::Dot(delta, delta);
                };
                if (!facts_.IsFactSet(kGOAPHasAccessKeyFact) &&
                    distanceSquared(graph_.nodes[0].worldPosition) <= 0.36f)
                {
                    facts_.SetFact(kGOAPHasAccessKeyFact, true);
                }
                if (facts_.IsFactSet(kGOAPHasAccessKeyFact) &&
                    distanceSquared(graph_.nodes[2].worldPosition) <= 0.36f)
                {
                    facts_.SetFact(kGOAPAtDestinationFact, true);
                }
            }

            AIDecisionRuntime decision_;
            GameplayRouteGraph graph_{};
            AIAgentWorldState facts_{};
            AIActionBindingRegistry bindings_{};
            std::unique_ptr<AIMoveToActionBinding> binding_{};
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

    [[nodiscard]] std::unique_ptr<GameplayAIDecisionInstance> CreateGameplayAIDecision(
        const std::string_view definitionId, const EntityHandle agent, LevelAsset& level)
    {
        if (definitionId != kAccessKeyAIDecisionId)
        {
            return nullptr;
        }
        const int start = ai_decision_detail::FindNode(level, "GOAP_Start");
        const int key = ai_decision_detail::FindNode(level, "GOAP_Access_Key");
        const int goal = ai_decision_detail::FindNode(level, "GOAP_Final_Goal");
        if (start < 0 || key < 0 || goal < 0)
        {
            return nullptr;
        }
        return std::make_unique<ai_decision_detail::AccessKeyDecision>(agent,
            level.nodes[static_cast<std::size_t>(start)].transform.position,
            level.nodes[static_cast<std::size_t>(key)].transform.position,
            level.nodes[static_cast<std::size_t>(goal)].transform.position);
    }
}