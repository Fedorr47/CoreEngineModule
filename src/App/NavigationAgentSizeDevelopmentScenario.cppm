module;

export module app.navigation_agent_size_development_scenario;
import core;
import std;

namespace app::navigationRuntime
{
    export enum class AgentSizeScenarioStatus
    {
        NotStarted,
        Moving,
        Reached,
        NoPath,
        Failed
    };

    constexpr std::string_view kAgentSizeScenarioLevelName{"NavigationSmallLargePassage"};
    constexpr std::string_view kSmallAgentNodeName{"SMALL NPC"};
    constexpr std::string_view kLargeAgentNodeName{"LARGE NPC"};
    constexpr std::string_view kSmallTargetNodeName{"SMALL TARGET"};
    constexpr std::string_view kLargeTargetNodeName{"LARGE TARGET"};

    export [[nodiscard]] constexpr bool IsAgentSizeScenario(const rendern::LevelAsset& level) noexcept
    {
        return level.name == kAgentSizeScenarioLevelName;
    }

    [[nodiscard]] int FindAgentSizeScenarioNode(
        const rendern::LevelAsset& level, const std::string_view name) noexcept
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
    
    [[nodiscard]] bool TryGetScenarioNodeTransform(
        const rendern::LevelAsset& level,
        const std::string_view nodeName,
        rendern::Transform& outTransform) noexcept
    {
        const int nodeIndex = FindAgentSizeScenarioNode(level, nodeName);
        if (nodeIndex < 0)
        {
            return false;
        }

        outTransform = level.nodes[static_cast<std::size_t>(nodeIndex)].transform;

        return true;
    }

    [[nodiscard]] rendern::EntityHandle EnsureScenarioAgent(
        rendern::GameplayRuntime& runtime,
        const rendern::GameplayUpdateContext& context,
        const std::string_view nodeName,
        const float radius)
    {
        const int nodeIndex = FindAgentSizeScenarioNode(*context.levelAsset, nodeName);
        if (nodeIndex < 0)
        {
            return rendern::kNullEntity;
        }

        const rendern::EntityHandle entity = runtime.SpawnNodeBoundEntity(context, nodeIndex, false);
        if (entity == rendern::kNullEntity)
        {
            return entity;
        }

        rendern::GameplayWorld& world = runtime.GetWorld();
        // A scenario NPC must never reuse the bootstrap/player entity: player
        // physics intentionally uses separate collider constants.
        if (world.HasPlayerControlled(entity))
        {
            return rendern::kNullEntity;
        }
        if (!world.HasAI(entity))
        {
            world.AddAI(entity);
        }
        world.SetCharacterPhysicalSettings(entity, {
            .radius = radius,
            .cylinderHeight = 1.0f,
            .maximumSlopeAngleDegrees = 45.0f,
            .maximumStepHeight = 0.25f,
            .mass = 70.0f
        });
        return entity;
    }

    [[nodiscard]] AgentSizeScenarioStatus StartScenarioAgent(
        rendern::GameplayRuntime& runtime,
        navigation::ProfileRegistry& profiles,
        const rendern::LevelAsset& level,
        const rendern::EntityHandle entity,
        const std::string_view targetName)
    {
        const int targetIndex = FindAgentSizeScenarioNode(level, targetName);
        rendern::GameplayWorld& gameplayWorld = runtime.GetWorld();
        const auto* transform = gameplayWorld.TryGetTransform(entity);
        const auto* physical = gameplayWorld.TryGetCharacterPhysicalSettings(entity);
        if (targetIndex < 0 || transform == nullptr || physical == nullptr ||
            gameplayWorld.HasPlayerControlled(entity) || !gameplayWorld.HasAI(entity))
        {
            return AgentSizeScenarioStatus::Failed;
        }

        runtime.CancelAIAction(entity);
        const navigation::ProfileResolution profile = profiles.ResolveProfile(BuildAgentSettings(*physical));
        const navigation::World* world = profiles.TryGetWorld(profile.profile);
        if (profile.status != navigation::BuildStatus::Succeeded || world == nullptr)
        {
            return AgentSizeScenarioStatus::Failed;
        }
        const navigation::PathResult path = world->FindPath({
            .start = transform->position,
            .goal = level.nodes[static_cast<std::size_t>(targetIndex)].transform.position,
            .searchExtents = {1.0f, 2.0f, 1.0f}
        });
        if (path.status == navigation::QueryStatus::NoPath)
        {
            return AgentSizeScenarioStatus::NoPath;
        }
        if (path.status != navigation::QueryStatus::Succeeded || path.points.size() < 2)
        {
            return AgentSizeScenarioStatus::Failed;
        }

        rendern::GameplayRoute route{};
        for (const mathUtils::Vec3& point : path.points)
        {
            route.points.push_back({.worldPosition = point});
        }
        route.segmentAnnotations.resize(route.points.size() - 1);
        rendern::GameplayArrivalSteeringSettings steering{};
        steering.acceptanceRadius = 0.2f;
        steering.slowingRadius = 0.75f;
        return runtime.StartAIFollowRoute(entity, std::move(route), steering) ==
            rendern::AIActionExecutionStatus::Running
            ? AgentSizeScenarioStatus::Moving
            : AgentSizeScenarioStatus::Failed;
    }

    void UpdateMovingStatus(
        const rendern::GameplayRuntime& runtime,
        const rendern::EntityHandle entity,
        AgentSizeScenarioStatus& status) noexcept
    {
        if (status != AgentSizeScenarioStatus::Moving)
        {
            return;
        }
        switch (runtime.GetAIActionStatus(entity))
        {
        case rendern::AIActionExecutionStatus::Running:
            break;
        case rendern::AIActionExecutionStatus::Succeeded:
            status = AgentSizeScenarioStatus::Reached;
            break;
        case rendern::AIActionExecutionStatus::Failed:
        case rendern::AIActionExecutionStatus::Cancelled:
        case rendern::AIActionExecutionStatus::NotStarted:
            status = AgentSizeScenarioStatus::Failed;
            break;
        }
    }
    
    [[nodiscard]] rendern::EntityHandle ResetScenarioAgent(
    rendern::GameplayRuntime& runtime,
    const rendern::EntityHandle entity,
    const rendern::Transform& canonical) noexcept
    {
        rendern::GameplayWorld& world = runtime.GetWorld();

        if (!world.IsEntityValid(entity))
        {
            return rendern::kNullEntity;
        }

        auto* transform = world.TryGetTransform(entity);
        if (transform == nullptr)
        {
            return rendern::kNullEntity;
        }

        runtime.ClearAIAction(entity);

        transform->position = canonical.position;
        transform->rotationDegrees = canonical.rotationDegrees;
        transform->scale = canonical.scale;

        if (auto* intent = world.TryGetInputIntent(entity))
        {
            *intent = {};
        }

        if (auto* command = world.TryGetCharacterCommand(entity))
        {
            rendern::ApplyGameplayMovementIntent({}, *command);
        }

        if (auto* motor = world.TryGetCharacterMotor(entity))
        {
            motor->velocity = {};
            motor->desiredVelocity = {};
            motor->desiredMoveWorld = {};
        }

        if (auto* state = world.TryGetCharacterMovementState(entity))
        {
            state->grounded = true;
            state->jumping = false;
            state->falling = false;
            state->physicallyBlocked = false;
            state->physicalBlockedSeconds = 0.0f;
            state->jumpPhase = rendern::GameplayJumpPhase::None;
            state->jumpRequestConsumed = false;
            state->jumpRequestResult = rendern::GameplayJumpRequestResult::None;
            state->jumpAirbornePhysicallyObserved = false;
            state->turningInPlace = false;

            state->facingYawDegrees = canonical.rotationDegrees.y;
            state->desiredFacingYawDegrees = canonical.rotationDegrees.y;
            state->previousFacingYawDegrees = canonical.rotationDegrees.y;
            state->cameraFacingYawDegrees = canonical.rotationDegrees.y;

            state->jumpLockedVelocity = {};
        }

        if (auto* locomotion = world.TryGetLocomotion(entity))
        {
            *locomotion = {};
        }

        return entity;
    }

    export struct AgentSizeScenarioResetResult
    {
        rendern::EntityHandle smallEntity{rendern::kNullEntity};
        rendern::EntityHandle largeEntity{rendern::kNullEntity};
    };

    export class NavigationAgentSizeDevelopmentScenario
    {
    public:
        void Reset() noexcept
        {
            smallEntity_ = rendern::kNullEntity;
            largeEntity_ = rendern::kNullEntity;

            smallInitialTransform_ = {};
            largeInitialTransform_ = {};
            hasSmallInitialTransform_ = false;
            hasLargeInitialTransform_ = false;

            ResetExecutionState();
        }
        void ResetExecutionState() noexcept
        {
            smallStatus_ = AgentSizeScenarioStatus::NotStarted;
            largeStatus_ = AgentSizeScenarioStatus::NotStarted;
        }
        void Prepare(rendern::GameplayRuntime& runtime, const rendern::GameplayUpdateContext& context)
        {
            if (context.levelAsset == nullptr || !IsAgentSizeScenario(*context.levelAsset))
            {
                return;
            }
            hasSmallInitialTransform_ = TryGetScenarioNodeTransform(
                *context.levelAsset,kSmallAgentNodeName,smallInitialTransform_);

            hasLargeInitialTransform_ = TryGetScenarioNodeTransform(
                *context.levelAsset, kLargeAgentNodeName, largeInitialTransform_);

            smallEntity_ = EnsureScenarioAgent(runtime, context, kSmallAgentNodeName,0.2f);

            largeEntity_ = EnsureScenarioAgent(runtime, context,kLargeAgentNodeName, 0.7f);
        }
        void Start(rendern::GameplayRuntime& runtime, navigation::ProfileRegistry& profiles,
            const rendern::LevelAsset& level)
        {
            smallStatus_ = StartScenarioAgent(runtime, profiles, level, smallEntity_, kSmallTargetNodeName);
            largeStatus_ = StartScenarioAgent(runtime, profiles, level, largeEntity_, kLargeTargetNodeName);
        }
        [[nodiscard]] AgentSizeScenarioResetResult ResetToInitialState(
            rendern::GameplayRuntime& runtime, rendern::LevelAsset& level) noexcept
        {
            if (!IsAgentSizeScenario(level) || !runtime.IsCurrentLevelAsset(level))
            {
                return {};
            }
            
            if (hasSmallInitialTransform_)
            {
                const int smallNodeIndex = FindAgentSizeScenarioNode(level, kSmallAgentNodeName);

                if (smallNodeIndex >= 0)
                {
                    level.nodes[static_cast<std::size_t>(smallNodeIndex)].transform = smallInitialTransform_;
                }
            }

            if (hasLargeInitialTransform_)
            {
                const int largeNodeIndex = FindAgentSizeScenarioNode(level, kLargeAgentNodeName);

                if (largeNodeIndex >= 0)
                {
                    level.nodes[static_cast<std::size_t>(largeNodeIndex)].transform = largeInitialTransform_;
                }
            }

            const rendern::EntityHandle resetSmall =
                hasSmallInitialTransform_
                    ? ResetScenarioAgent(
                        runtime,
                        smallEntity_,
                        smallInitialTransform_)
                    : rendern::kNullEntity;

            const rendern::EntityHandle resetLarge =
                hasLargeInitialTransform_
                    ? ResetScenarioAgent(
                        runtime,
                        largeEntity_,
                        largeInitialTransform_)
                    : rendern::kNullEntity;

            smallStatus_ =
                resetSmall != rendern::kNullEntity
                    ? AgentSizeScenarioStatus::NotStarted
                    : AgentSizeScenarioStatus::Failed;

            largeStatus_ =
                resetLarge != rendern::kNullEntity
                    ? AgentSizeScenarioStatus::NotStarted
                    : AgentSizeScenarioStatus::Failed;

            return {.smallEntity = resetSmall, .largeEntity = resetLarge};
        }
        void Update(const rendern::GameplayRuntime& runtime) noexcept
        {
            UpdateMovingStatus(runtime, smallEntity_, smallStatus_);
            UpdateMovingStatus(runtime, largeEntity_, largeStatus_);
        }
        [[nodiscard]] AgentSizeScenarioStatus GetSmallStatus() const noexcept { return smallStatus_; }
        [[nodiscard]] AgentSizeScenarioStatus GetLargeStatus() const noexcept { return largeStatus_; }
        
        
    private:
        rendern::EntityHandle smallEntity_{rendern::kNullEntity};
        rendern::EntityHandle largeEntity_{rendern::kNullEntity};
        
        rendern::Transform smallInitialTransform_{};
        rendern::Transform largeInitialTransform_{};
        bool hasSmallInitialTransform_{false};
        bool hasLargeInitialTransform_{false};

        AgentSizeScenarioStatus smallStatus_{AgentSizeScenarioStatus::NotStarted};
        AgentSizeScenarioStatus largeStatus_{AgentSizeScenarioStatus::NotStarted};
    };
}