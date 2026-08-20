module;

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string_view>

export module core:gameplay_ai_goap_access_key_development_scenario;

import :ai_decision_runtime;
import :ai_move_to_action_binding;
import :gameplay;
import :gameplay_runtime;
import :gameplay_route;
import :level;

export namespace rendern
{
    inline constexpr AIWorldFactId kGOAPHasAccessKeyFact{40u};
    inline constexpr AIWorldFactId kGOAPAtDestinationFact{41u};
    inline constexpr AIActionContextId kGOAPAccessKeyMoveContext{1u};
    inline constexpr AIActionContextId kGOAPFinalGoalMoveContext{2u};
    inline constexpr AIGoalId kGOAPReachDestinationGoal{50u};

    namespace goap_access_key_detail
    {
        constexpr std::string_view kPlayer{"GOAP_Observer_Player"};
        constexpr std::string_view kAgent{"GOAP_Agent"};
        constexpr std::string_view kStart{"GOAP_Start"};
        constexpr std::string_view kKey{"GOAP_Access_Key"};
        constexpr std::string_view kGoal{"GOAP_Final_Goal"};

        struct Nodes { int player{-1}; int agent{-1}; int start{-1}; int key{-1}; int goal{-1}; };

        [[nodiscard]] std::optional<int> Find(const LevelAsset& level, std::string_view name) noexcept
        {
            for (std::size_t i = 0; i < level.nodes.size(); ++i)
                if (level.nodes[i].alive && level.nodes[i].name == name) return static_cast<int>(i);
            return std::nullopt;
        }

        [[nodiscard]] std::optional<Nodes> Resolve(const LevelAsset& level) noexcept
        {
            const auto player=Find(level,kPlayer), agent=Find(level,kAgent), start=Find(level,kStart);
            const auto key=Find(level,kKey), goal=Find(level,kGoal);
            if (!player || !agent || !start || !key || !goal) return std::nullopt;
            return Nodes{*player,*agent,*start,*key,*goal};
        }

        [[nodiscard]] EntityHandle FindEntity(const GameplayRuntime& runtime, int node) noexcept
        {
            for (const EntityHandle entity : runtime.GetNodeBoundEntities())
                if (const auto* link=runtime.GetWorld().TryGetNodeLink(entity);
                    runtime.GetWorld().IsEntityValid(entity) && link && link->nodeIndex == node) return entity;
            return kNullEntity;
        }

        [[nodiscard]] float DistanceSquared(const mathUtils::Vec3& a, const mathUtils::Vec3& b) noexcept
        {
            const auto d=a-b; return mathUtils::Dot(d,d);
        }
    }

    class GameplayAIGOAPAccessKeyDevelopmentScenario final : private IAIMoveToActionRequestProvider
    {
    public:
        GameplayAIGOAPAccessKeyDevelopmentScenario() = default;

        [[nodiscard]] bool Prepare(GameplayRuntime& runtime, const GameplayUpdateContext& context)
        {
            (void)Reset(runtime);
            if (!context.levelAsset || !runtime.IsCurrentLevelContext(context)) return false;
            const auto nodes=goap_access_key_detail::Resolve(*context.levelAsset);
            if (!nodes) return false;
            nodes_=*nodes;
            const auto& level=*context.levelAsset;
            initialAgentTransform_=level.nodes[static_cast<std::size_t>(nodes_.agent)].transform;
            graph_.nodes = {
                {GameplayRouteNodeId{1u}, level.nodes[static_cast<std::size_t>(nodes_.key)].transform.position},
                {GameplayRouteNodeId{2u}, level.nodes[static_cast<std::size_t>(nodes_.start)].transform.position},
                {GameplayRouteNodeId{3u}, level.nodes[static_cast<std::size_t>(nodes_.goal)].transform.position}};
            graph_.edges = {{GameplayRouteNodeId{2u},GameplayRouteNodeId{1u},1.0f},
                {GameplayRouteNodeId{1u},GameplayRouteNodeId{2u},1.0f},
                {GameplayRouteNodeId{2u},GameplayRouteNodeId{3u},1.0f},
                {GameplayRouteNodeId{1u},GameplayRouteNodeId{3u},2.0f}};
            
            levelAsset_ = context.levelAsset;
            levelInstance_ = context.levelInstance;
            scene_ = context.scene;
            
            if (levelAsset_ != nullptr &&
                levelInstance_ != nullptr &&
                scene_ != nullptr)
            {
                (void)levelInstance_->SetNodeRuntimeVisible(
                    *levelAsset_,
                    *scene_,
                    nodes_.key,
                    true);
            }
            
            prepared_=true;
            return true;
        }

        [[nodiscard]] bool Start(GameplayRuntime& runtime, const GameplayUpdateContext& context)
        {
            if (!prepared_ && !Prepare(runtime, context)) return false;
            if (!context.levelAsset || context.mode != GameplayRuntimeMode::Game
                || active_ || decision_ || binding_) return false;
            const EntityHandle player=goap_access_key_detail::FindEntity(runtime,nodes_.player);
            EntityHandle agent=goap_access_key_detail::FindEntity(runtime,nodes_.agent);
            if (agent == kNullEntity) agent=runtime.SpawnNodeBoundEntity(context,nodes_.agent,false);
            const GameplayWorld& world=runtime.GetWorld();
            if (player == kNullEntity || player != runtime.GetControlledEntity() || player == agent
                || !world.HasPlayerControlled(player) || world.TryGetAnimationLink(player) == nullptr
                || agent == kNullEntity) return false;
            const bool bAgentHasMovementContract = world.HasTransform(agent)
                && world.HasCharacterCommand(agent) && world.HasCharacterMotor(agent)
                && world.HasCharacterMovementState(agent);
            if (!bAgentHasMovementContract) return false;
            if (!runtime.GetWorld().HasAI(agent)) runtime.GetWorld().AddAI(agent);
            std::unique_ptr<AIMoveToActionBinding> binding=runtime.CreateAIMoveToActionBinding(*this);
            bindings_.Reset();
            if (!binding || !bindings_.Register(kAIMoveToActionId,*binding)) return false;
            std::optional<AIDecisionRuntime> decision{std::in_place,agent};
            player_=player; agent_=agent;
            binding_=std::move(binding); decision_=std::move(decision);
            active_=true;
            return true;
        }

        void Update(GameplayRuntime& runtime)
        {
            if (!active_ || !decision_ || agent_ == kNullEntity) return;
            Observe(runtime.GetWorld());
            const std::array candidates{AIGoalSelectionCandidate{
                .goal=AIGoalDefinition{kGOAPReachDestinationGoal,{{kGOAPAtDestinationFact,true}}}, .baseScore=1.0f}};
            const std::array<AIActionDefinition, 2> actions{
                AIActionDefinition{
                    .actionId = kAIMoveToActionId,
                    .preconditions = {
                        AIFactCondition{kGOAPHasAccessKeyFact, false}
                    },
                    .effects = {
                        AIFactEffect{kGOAPHasAccessKeyFact, true}
                    },
                    .contextId = kGOAPAccessKeyMoveContext,
                    .baseCost = 1.0f
                },
                AIActionDefinition{
                    .actionId = kAIMoveToActionId,
                    .preconditions = {
                        AIFactCondition{kGOAPHasAccessKeyFact, true},
                        AIFactCondition{kGOAPAtDestinationFact, false}
                    },
                    .effects = {
                        AIFactEffect{kGOAPAtDestinationFact, true}
                    },
                    .contextId = kGOAPFinalGoalMoveContext,
                    .baseCost = 1.0f
                }
            };
            (void)runtime.UpdateAIDecision(*decision_,facts_,candidates,actions,bindings_);
        }

        [[nodiscard]] EntityHandle Reset(GameplayRuntime& runtime) noexcept
        {
            if (decision_) runtime.CancelAIDecision(*decision_);
            bindings_.Reset(); binding_.reset(); decision_.reset();
            facts_={}; 
            prepared_ = false;
            active_=false;
            if (levelInstance_ != nullptr && scene_ != nullptr)
            {
                (void)levelInstance_->SetNodeRuntimeVisible(
                    *levelAsset_,
                    *scene_,
                    nodes_.key,
                    true);
            }
            const EntityHandle resetAgent = agent_;
            if (agent_ != kNullEntity && runtime.GetWorld().IsEntityValid(agent_))
            {
                runtime.ClearAIAction(agent_);
                if (auto* transform=runtime.GetWorld().TryGetTransform(agent_))
                {
                    transform->position=initialAgentTransform_.position;
                    transform->rotationDegrees=initialAgentTransform_.rotationDegrees;
                }
                if (auto* motor=runtime.GetWorld().TryGetCharacterMotor(agent_)) *motor={};
                if (auto* command=runtime.GetWorld().TryGetCharacterCommand(agent_)) *command={};
                if (auto* intent=runtime.GetWorld().TryGetInputIntent(agent_)) *intent={};
                if (auto* state=runtime.GetWorld().TryGetCharacterMovementState(agent_))
                {
                    const float yaw=initialAgentTransform_.rotationDegrees.y;
                    *state={}; state->grounded=true; state->facingYawDegrees=yaw;
                    state->desiredFacingYawDegrees=yaw; state->previousFacingYawDegrees=yaw;
                    state->cameraFacingYawDegrees=yaw;
                }
                if (auto* locomotion=runtime.GetWorld().TryGetLocomotion(agent_)) *locomotion={};
            }
            agent_=kNullEntity; player_=kNullEntity;
            levelAsset_ = nullptr;
            levelInstance_ = nullptr;
            scene_ = nullptr;
            return resetAgent;
        }

        void Observe(const GameplayWorld& world) noexcept
        {
            if (agent_ == kNullEntity) return;
            const auto* transform=world.TryGetTransform(agent_); if (!transform) return;
            if (!facts_.IsFactSet(kGOAPHasAccessKeyFact) &&
                goap_access_key_detail::DistanceSquared(
                transform->position,
                graph_.nodes[0].worldPosition) <= 0.36f)
            {
                facts_.SetFact(kGOAPHasAccessKeyFact, true);

                if (levelAsset_ != nullptr &&
                    levelInstance_ != nullptr &&
                    scene_ != nullptr)
                {
                    (void)levelInstance_->SetNodeRuntimeVisible(
                        *levelAsset_,
                        *scene_,
                        nodes_.key,
                        false);
                }
            }
            if (facts_.IsFactSet(kGOAPHasAccessKeyFact) &&
                goap_access_key_detail::DistanceSquared(transform->position,graph_.nodes[2].worldPosition)<=0.36f)
                facts_.SetFact(kGOAPAtDestinationFact,true);
        }

        [[nodiscard]] const AIAgentWorldState& GetObservedFacts() const noexcept { return facts_; }
        [[nodiscard]] AIPlanExecutionStatus GetStatus() const noexcept
        { return decision_ ? decision_->GetStatus() : AIPlanExecutionStatus::NotStarted; }
        [[nodiscard]] EntityHandle GetPlayerEntity() const noexcept { return player_; }
        [[nodiscard]] EntityHandle GetAgentEntity() const noexcept { return agent_; }
        [[nodiscard]] bool IsActive() const noexcept { return active_; }

    private:
        std::optional<AIMoveToActionRequest> ResolveRequest(const AIActionRuntimeContext& context) override
        {
            GameplayRouteNodeId start{}, goal{};
            if (context.contextId==kGOAPAccessKeyMoveContext) { start=GameplayRouteNodeId{2u}; goal=GameplayRouteNodeId{1u}; }
            else if (context.contextId==kGOAPFinalGoalMoveContext) { start=GameplayRouteNodeId{1u}; goal=GameplayRouteNodeId{3u}; }
            else return std::nullopt;
            GameplayArrivalSteeringSettings steering{}; steering.acceptanceRadius=0.35f; steering.slowingRadius=1.25f;
            return AIMoveToActionRequest{&graph_,start,goal,steering};
        }

        goap_access_key_detail::Nodes nodes_{}; Transform initialAgentTransform_{};
        GameplayRouteGraph graph_{}; AIAgentWorldState facts_{};
        EntityHandle player_{kNullEntity}, agent_{kNullEntity}; bool prepared_{}, active_{};
        AIActionBindingRegistry bindings_{};
        std::unique_ptr<AIMoveToActionBinding> binding_{};
        std::optional<AIDecisionRuntime> decision_{};
        const LevelAsset* levelAsset_{};
        LevelInstance* levelInstance_{};
        Scene* scene_{};
    };

    [[nodiscard]] bool IsGameplayAIGOAPAccessKeyDevelopmentScenario(const LevelAsset& level) noexcept
    {
        return goap_access_key_detail::Resolve(level).has_value();
    }
}