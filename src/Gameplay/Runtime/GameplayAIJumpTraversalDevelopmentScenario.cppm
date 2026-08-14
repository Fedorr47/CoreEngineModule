module;

#include <array>
#include <optional>
#include <string_view>
#include <utility>

export module core:gameplay_ai_jump_traversal_development_scenario;

import :ai_action_task;
import :gameplay;
import :gameplay_runtime;
import :gameplay_route;
import :gameplay_steering;
import :gameplay_traversal_link;
import :level;

export namespace rendern
{
    inline constexpr GameplayTraversalLinkHandle kAIJumpTraversalDevelopmentLinkHandle{4470001u};

        constexpr std::string_view kAgentName{"JumpTraversalAgent"};
        constexpr std::string_view kStartName{"JumpRouteStart"};
        constexpr std::string_view kTraversalEntryName{"JumpTraversalEntry"};
        constexpr std::string_view kTakeoffName{"JumpTakeoff"};
        constexpr std::string_view kLandingName{"JumpLanding"};
        constexpr std::string_view kPostLandingName{"JumpPostLanding"};
        constexpr std::string_view kFinishName{"JumpRouteFinish"};

        struct ScenarioNodes
        {
            int agent{-1};
            int start{-1};
            int traversalEntry{-1};
            int takeoff{-1};
            int landing{-1};
            int postLanding{-1};
            int finish{-1};
        };

        [[nodiscard]] std::optional<int> FindNode(const LevelAsset& level, const std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < level.nodes.size(); ++index)
            {
                if (level.nodes[index].alive && level.nodes[index].name == name)
                {
                    return static_cast<int>(index);
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<ScenarioNodes> ResolveNodes(const LevelAsset& level) noexcept
        {
            const auto agent = FindNode(level, kAgentName);
            const auto start = FindNode(level, kStartName);
            const auto traversalEntry = FindNode(level, kTraversalEntryName);
            const auto takeoff = FindNode(level, kTakeoffName);
            const auto landing = FindNode(level, kLandingName);
            const auto postLanding = FindNode(level, kPostLandingName);
            const auto finish = FindNode(level, kFinishName);
            if (!agent || !start || !traversalEntry || !takeoff || !landing || !postLanding || !finish)
            {
                return std::nullopt;
            }
            return ScenarioNodes{*agent, *start, *traversalEntry, *takeoff, *landing, *postLanding, *finish};
        }

        [[nodiscard]] EntityHandle FindEntity(const GameplayRuntime& runtime, const int nodeIndex) noexcept
        {
            const GameplayWorld& world = runtime.GetWorld();
            for (const EntityHandle entity : runtime.GetNodeBoundEntities())
            {
                const GameplayNodeLinkComponent* link = world.TryGetNodeLink(entity);
                if (world.IsEntityValid(entity) && link != nullptr && link->nodeIndex == nodeIndex)
                {
                    return entity;
                }
            }
            return kNullEntity;
        }

        void RestoreCanonicalAgent(GameplayWorld& world, const EntityHandle entity, const Transform& canonical) noexcept
        {
            if (GameplayActionComponent* action = world.TryGetAction(entity))
            {
                ResetGameplayActionState(*action);
            }
            if (GameplayTransformComponent* transform = world.TryGetTransform(entity))
            {
                transform->position = canonical.position;
                transform->rotationDegrees = canonical.rotationDegrees;
                transform->scale = canonical.scale;
            }
            if (GameplayInputIntentComponent* intent = world.TryGetInputIntent(entity)) *intent = {};
            if (GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity)) *command = {};
            if (GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(entity))
            {
                motor->velocity = {};
                motor->desiredVelocity = {};
                motor->desiredMoveWorld = {};
            }
            if (GameplayCharacterMovementStateComponent* state = world.TryGetCharacterMovementState(entity))
            {
                const float yaw = canonical.rotationDegrees.y;
                *state = {};
                state->grounded = true;
                state->facingYawDegrees = yaw;
                state->desiredFacingYawDegrees = yaw;
                state->previousFacingYawDegrees = yaw;
                state->cameraFacingYawDegrees = yaw;
            }
            if (GameplayLocomotionComponent* locomotion = world.TryGetLocomotion(entity)) *locomotion = {};
        }

    class GameplayAIJumpTraversalDevelopmentScenarioState
    {
    public:
        void Reset() noexcept
        {
            initialAgentTransform_ = {};
            hasInitialAgentTransform_ = false;
        }

        void CaptureInitialAgentTransform(const Transform& transform) noexcept
        {
            if (!hasInitialAgentTransform_)
            {
                initialAgentTransform_ = transform;
                hasInitialAgentTransform_ = true;
            }
        }

        [[nodiscard]] const Transform* TryGetInitialAgentTransform() const noexcept
        {
            return hasInitialAgentTransform_ ? &initialAgentTransform_ : nullptr;
        }

    private:
        Transform initialAgentTransform_{};
        bool hasInitialAgentTransform_{false};
    };

    void PrepareGameplayAIJumpTraversalDevelopmentScenario(
        GameplayAIJumpTraversalDevelopmentScenarioState& state,
        const LevelAsset& level) noexcept
    {
        state.Reset();
        const auto nodes = ResolveNodes(level);
        if (nodes)
        {
            state.CaptureInitialAgentTransform(
                level.nodes[static_cast<std::size_t>(nodes->agent)].transform);
        }
    }

    [[nodiscard]] bool IsGameplayAIJumpTraversalDevelopmentScenario(const LevelAsset& level) noexcept
    {
        return ResolveNodes(level).has_value();
    }

    [[nodiscard]] std::optional<GameplayRoute> BuildGameplayAIJumpTraversalDevelopmentRoute(
        const LevelAsset& level) noexcept
    {
        const auto nodes = ResolveNodes(level);
        if (!nodes) return std::nullopt;

        const std::array<int, 5> routeNodes{
            nodes->start, nodes->traversalEntry, nodes->landing, nodes->postLanding, nodes->finish};
        GameplayRoute route{};
        route.points.reserve(routeNodes.size());
        for (const int node : routeNodes)
        {
            route.points.push_back({level.nodes[static_cast<std::size_t>(node)].transform.position});
        }
        route.segmentAnnotations = {
            GameplayRouteSegmentAnnotation{},
            GameplayRouteSegmentAnnotation{.traversalLink = kAIJumpTraversalDevelopmentLinkHandle},
            GameplayRouteSegmentAnnotation{},
            GameplayRouteSegmentAnnotation{}
        };
        return route;
    }

    [[nodiscard]] AIActionExecutionStatus StartGameplayAIJumpTraversalDevelopmentScenario(
        GameplayRuntime& runtime, const GameplayUpdateContext& context)
    {
        if (context.levelAsset == nullptr || context.levelInstance == nullptr || context.scene == nullptr ||
            context.mode != GameplayRuntimeMode::Game || runtime.GetCurrentMode() != GameplayRuntimeMode::Game ||
            !runtime.IsCurrentLevelContext(context))
        {
            return AIActionExecutionStatus::Failed;
        }
        const auto nodes = ResolveNodes(*context.levelAsset);
        if (!nodes) return AIActionExecutionStatus::Failed;

        EntityHandle agent = FindEntity(runtime, nodes->agent);
        if (agent == kNullEntity) agent = runtime.SpawnNodeBoundEntity(context, nodes->agent, false);
        EntityHandle landingMarker = FindEntity(runtime, nodes->landing);
        if (landingMarker == kNullEntity) landingMarker = runtime.SpawnNodeBoundEntity(context, nodes->landing, false);

        GameplayWorld& world = runtime.GetWorld();
        if (agent == kNullEntity || landingMarker == kNullEntity || !world.HasTransform(agent) ||
            !world.HasCharacterCommand(agent) || !world.HasCharacterMotor(agent) ||
            !world.HasCharacterMovementState(agent))
        {
            return AIActionExecutionStatus::Failed;
        }
        // The authored landing node is an identity target for the traversal
        // link, not a second character or a physical obstacle on the pad.
        world.RemoveCharacterPhysicalSettings(landingMarker);
        if (!world.HasAI(agent)) world.AddAI(agent);

        const auto& takeoff = context.levelAsset->nodes[static_cast<std::size_t>(nodes->takeoff)].transform.position;
        const auto& landing = context.levelAsset->nodes[static_cast<std::size_t>(nodes->landing)].transform.position;
        const GameplayTraversalLink link{
            .handle = kAIJumpTraversalDevelopmentLinkHandle,
            .traversalTypeId = kJumpTraversalTypeId,
            .targetEntity = landingMarker,
            .jump = {
                .takeoffPosition = takeoff,
                .landingPosition = landing,
                .verticalSpeed = 5.5f,
                .takeoffTolerance = 0.20f,
                .landingHorizontalTolerance = 0.55f,
                .landingVerticalTolerance = 0.30f
            }
        };
        if (!runtime.RegisterGameplayTraversalLink(link)) return AIActionExecutionStatus::Failed;

        std::optional<GameplayRoute> route = BuildGameplayAIJumpTraversalDevelopmentRoute(*context.levelAsset);
        if (!route) return AIActionExecutionStatus::Failed;
        GameplayArrivalSteeringSettings steering{};
        steering.acceptanceRadius = 0.20f;
        steering.slowingRadius = 0.75f;
        steering.wantsRun = false;
        return runtime.StartAIFollowRoute(agent, std::move(*route), steering);
    }

    void CancelGameplayAIJumpTraversalDevelopmentScenario(GameplayRuntime& runtime, const LevelAsset& level) noexcept
    {
        const auto nodes = ResolveNodes(level);
        if (!nodes || !runtime.IsCurrentLevelAsset(level)) return;
        const EntityHandle agent = FindEntity(runtime, nodes->agent);
        if (agent != kNullEntity) runtime.CancelAIAction(agent);
    }

    [[nodiscard]] EntityHandle ResetGameplayAIJumpTraversalDevelopmentScenario(
        GameplayRuntime& runtime,
        const LevelAsset& level,
        const GameplayAIJumpTraversalDevelopmentScenarioState& state) noexcept
    {
        const auto nodes = ResolveNodes(level);
        const Transform* initialTransform = state.TryGetInitialAgentTransform();
        if (!nodes || !runtime.IsCurrentLevelAsset(level) || initialTransform == nullptr) return kNullEntity;
        const EntityHandle agent = FindEntity(runtime, nodes->agent);
        if (agent == kNullEntity) return kNullEntity;
        runtime.ClearAIAction(agent);
        (void)runtime.RemoveGameplayTraversalLink(kAIJumpTraversalDevelopmentLinkHandle);
        RestoreCanonicalAgent(runtime.GetWorld(), agent, *initialTransform);
        return agent;
    }

    [[nodiscard]] AIActionExecutionStatus GetGameplayAIJumpTraversalDevelopmentScenarioStatus(
        const GameplayRuntime& runtime, const LevelAsset& level) noexcept
    {
        const auto nodes = ResolveNodes(level);
        if (!nodes || !runtime.IsCurrentLevelAsset(level)) return AIActionExecutionStatus::NotStarted;
        const EntityHandle agent = FindEntity(runtime, nodes->agent);
        return agent == kNullEntity ? AIActionExecutionStatus::NotStarted : runtime.GetAIActionStatus(agent);
    }
}