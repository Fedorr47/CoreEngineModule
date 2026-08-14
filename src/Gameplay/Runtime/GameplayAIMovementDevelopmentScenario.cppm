module;

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

export module core:gameplay_ai_movement_development_scenario;

import :ai_action_task;
import :gameplay;
import :gameplay_runtime;
import :gameplay_route;
import :gameplay_route_search;
import :gameplay_steering;
import :level;
import :math_utils;

namespace rendern
{
    namespace
    {
        inline constexpr std::string_view kDevelopmentAgentNodeName{
            "AI_Move_Agent"
        };

        inline constexpr std::string_view kDevelopmentRoutePointPrefix{
            "AI_Move_Point_"
        };
        
        inline constexpr std::string_view kStepDebugAgentNodeName{
            "NPC_Step_Start"
        };

        inline constexpr std::string_view kStepDebugTargetNodeName{
            "RouteTarget"
        };

        struct GameplayAIMovementDevelopmentRouteNode
        {
            std::size_t order{0};
            int nodeIndex{-1};
        };

        struct GameplayAIMovementDevelopmentScenarioNodes
        {
            int agentNodeIndex{-1};
            std::vector<int> routeNodeIndices{};
        };

        [[nodiscard]] std::optional<int> FindGameplayAIMovementDevelopmentNodeIndex_(
            const LevelAsset& levelAsset,
            const std::string_view nodeName) noexcept
        {
            for (std::size_t index = 0; index < levelAsset.nodes.size(); ++index)
            {
                const LevelNode& node = levelAsset.nodes[index];

                const bool bIsMatchingNode = node.alive && node.name == nodeName;

                if (bIsMatchingNode)
                {
                    return static_cast<int>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<int> FindGameplayAIMovementDevelopmentAgentNodeIndex_(
            const LevelAsset& levelAsset) noexcept
        {
            return FindGameplayAIMovementDevelopmentNodeIndex_(levelAsset, kDevelopmentAgentNodeName);
        }

        [[nodiscard]] std::optional<std::size_t> ParseGameplayAIMovementDevelopmentRoutePointOrder_(
            const std::string_view suffix) noexcept
        {
            if (suffix.empty())
            {
                return std::nullopt;
            }

            std::size_t order{0};

            const char* const begin = suffix.data();
            const char* const end = begin + suffix.size();

            const auto [parsedEnd, error] =
                std::from_chars(begin, end, order);

            const bool bParsedCompleteSuffix =
                error == std::errc{} &&
                parsedEnd == end;

            if (!bParsedCompleteSuffix)
            {
                return std::nullopt;
            }

            return order;
        }

        [[nodiscard]] std::optional<std::vector<int>> ResolveGameplayAIMovementDevelopmentRouteNodeIndices_(
            const LevelAsset& levelAsset)
        {
            std::vector<GameplayAIMovementDevelopmentRouteNode> indexedRouteNodes{};

            for (std::size_t index = 0; index < levelAsset.nodes.size(); ++index)
            {
                const LevelNode& node = levelAsset.nodes[index];

                if (!node.alive)
                {
                    continue;
                }

                const std::string_view nodeName = node.name;

                if (!nodeName.starts_with(kDevelopmentRoutePointPrefix))
                {
                    continue;
                }

                const std::string_view orderSuffix = nodeName.substr(kDevelopmentRoutePointPrefix.size());

                const std::optional<std::size_t> routeOrder = 
                    ParseGameplayAIMovementDevelopmentRoutePointOrder_(orderSuffix);

                // A node using the route-point prefix must contain a valid
                // numeric suffix so authored route order is unambiguous.
                if (!routeOrder.has_value())
                {
                    return std::nullopt;
                }

                indexedRouteNodes.push_back(
                    GameplayAIMovementDevelopmentRouteNode{
                        .order = *routeOrder,
                        .nodeIndex = static_cast<int>(index)
                    });
            }

            const bool bHasMinimumRoutePointCount = indexedRouteNodes.size() >= 2;
            if (!bHasMinimumRoutePointCount)
            {
                return std::nullopt;
            }

            std::sort(
                indexedRouteNodes.begin(),
                indexedRouteNodes.end(),
                [](
                    const GameplayAIMovementDevelopmentRouteNode& lhs,
                    const GameplayAIMovementDevelopmentRouteNode& rhs)
                {
                    return lhs.order < rhs.order;
                });

            for (std::size_t index = 1; index < indexedRouteNodes.size(); ++index)
            {
                const bool bHasDuplicateRouteOrder =
                    indexedRouteNodes[index - 1].order ==
                    indexedRouteNodes[index].order;

                // Point_1 and Point_001 describe the same authored order and
                // therefore make the route definition ambiguous.
                if (bHasDuplicateRouteOrder)
                {
                    return std::nullopt;
                }
            }

            std::vector<int> routeNodeIndices{};
            routeNodeIndices.reserve(indexedRouteNodes.size());

            for (const GameplayAIMovementDevelopmentRouteNode& routeNode : indexedRouteNodes)
            {
                routeNodeIndices.push_back(routeNode.nodeIndex);
            }

            return routeNodeIndices;
        }

        [[nodiscard]] std::optional<GameplayAIMovementDevelopmentScenarioNodes>
        ResolveGameplayAIMovementDevelopmentScenarioNodes_(
            const LevelAsset& levelAsset)
        {
            const std::optional<int> agentNodeIndex =
                FindGameplayAIMovementDevelopmentAgentNodeIndex_(
                    levelAsset);

            std::optional<std::vector<int>> routeNodeIndices =
                ResolveGameplayAIMovementDevelopmentRouteNodeIndices_(levelAsset);

            const bool bHasDevelopmentAgent = agentNodeIndex.has_value();
            const bool bHasDevelopmentRoute = routeNodeIndices.has_value();

            if (!bHasDevelopmentAgent || !bHasDevelopmentRoute)
            {
                return std::nullopt;
            }

            return GameplayAIMovementDevelopmentScenarioNodes{
                .agentNodeIndex = *agentNodeIndex,
                .routeNodeIndices = std::move(*routeNodeIndices)
            };
        }

        [[nodiscard]] EntityHandle
        FindGameplayAIMovementDevelopmentAgentEntity_(
            const GameplayRuntime& runtime,
            const int agentNodeIndex) noexcept
        {
            const GameplayWorld& world = runtime.GetWorld();

            for (const EntityHandle entity : runtime.GetNodeBoundEntities())
            {
                if (!world.IsEntityValid(entity))
                {
                    continue;
                }

                const GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);

                const bool bIsDevelopmentAgent =
                    nodeLink != nullptr &&
                    nodeLink->nodeIndex == agentNodeIndex;

                if (bIsDevelopmentAgent)
                {
                    return entity;
                }
            }

            return kNullEntity;
        }

        [[nodiscard]] GameplayRouteNodeId MakeGameplayAIMovementDevelopmentRouteNodeId_(
            const int nodeIndex) noexcept
        {
            return GameplayRouteNodeId{ static_cast<GameplayRouteNodeId::ValueType>(nodeIndex) + 1u };
        }

        [[nodiscard]] GameplayRouteGraph BuildGameplayAIMovementDevelopmentRouteGraph_(
            const LevelAsset& levelAsset,
            const std::vector<int>& routeNodeIndices)
        {
            GameplayRouteGraph routeGraph{};
            routeGraph.nodes.reserve(routeNodeIndices.size());
            routeGraph.edges.reserve(routeNodeIndices.size() - 1u);

            for (const int nodeIndex : routeNodeIndices)
            {
                const LevelNode& routeNode = levelAsset.nodes[static_cast<std::size_t>(nodeIndex)];

                routeGraph.nodes.push_back(
                    GameplayRouteGraphNode{
                        .nodeId = MakeGameplayAIMovementDevelopmentRouteNodeId_(nodeIndex),
                        .worldPosition = routeNode.transform.position
                    });
            }

            for (std::size_t pointIndex = 1; pointIndex < routeNodeIndices.size(); ++pointIndex)
            {
                routeGraph.edges.push_back(
                    GameplayRouteGraphEdge{
                        .fromNodeId = MakeGameplayAIMovementDevelopmentRouteNodeId_(
                            routeNodeIndices[pointIndex - 1u]),
                        .toNodeId = MakeGameplayAIMovementDevelopmentRouteNodeId_(
                            routeNodeIndices[pointIndex]),
                        .cost = 1.0f,
                        .annotation = GameplayRouteSegmentAnnotation{}
                    });
            }

            return routeGraph;
        }

        void ResetGameplayAIMovementDevelopmentAgentForRestart_(
            GameplayWorld& world,
            const EntityHandle agentEntity,
            const mathUtils::Vec3& routeStartPosition,
            const auto& agentCanonicalTransform) noexcept
        {
            if (!world.IsEntityValid(agentEntity))
            {
                return;
            }

            if (GameplayTransformComponent* transform = world.TryGetTransform(agentEntity))
            {
                transform->position = routeStartPosition;
                transform->rotationDegrees = agentCanonicalTransform.rotationDegrees;
                transform->scale = agentCanonicalTransform.scale;
            }

            if (GameplayInputIntentComponent* intent = world.TryGetInputIntent(agentEntity))
            {
                *intent = {};
            }

            if (GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agentEntity))
            {
                ApplyGameplayMovementIntent(GameplayMovementIntent{}, *command);
            }

            if (GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(agentEntity))
            {
                motor->velocity = {};
                motor->desiredVelocity = {};
                motor->desiredMoveWorld = {};
            }

            if (GameplayCharacterMovementStateComponent* movementState = world.TryGetCharacterMovementState(agentEntity))
            {
                movementState->grounded = true;
                movementState->jumping = false;
                movementState->falling = false;
                movementState->physicallyBlocked = false;
                movementState->physicalBlockedSeconds = 0.0f;
                movementState->jumpPhase = GameplayJumpPhase::None;
                movementState->jumpRequestConsumed = false;
                movementState->jumpRequestResult = GameplayJumpRequestResult::None;
                movementState->jumpAirbornePhysicallyObserved = false;
                movementState->turningInPlace = false;

                movementState->facingYawDegrees = agentCanonicalTransform.rotationDegrees.y;
                movementState->desiredFacingYawDegrees = agentCanonicalTransform.rotationDegrees.y;
                movementState->previousFacingYawDegrees = agentCanonicalTransform.rotationDegrees.y;
                movementState->cameraFacingYawDegrees = agentCanonicalTransform.rotationDegrees.y;
                movementState->jumpLockedVelocity = {};
            }
            
            if (GameplayLocomotionComponent* locomotion = world.TryGetLocomotion(agentEntity))
            {
                *locomotion = {};
            }
        }
    }
    
     export [[nodiscard]] bool IsGameplayAIStepDebugScenario(
        const LevelAsset& levelAsset) noexcept
    {
        return FindGameplayAIMovementDevelopmentNodeIndex_(levelAsset, kStepDebugAgentNodeName).has_value() &&
            FindGameplayAIMovementDevelopmentNodeIndex_(levelAsset, kStepDebugTargetNodeName).has_value();
    }
    
    export [[nodiscard]] bool IsGameplayAIMovementDevelopmentScenario(
        const LevelAsset& levelAsset)
    {
        return ResolveGameplayAIMovementDevelopmentScenarioNodes_(levelAsset).has_value();
    }


    export [[nodiscard]] AIActionExecutionStatus StartGameplayAIStepDebugRoute(
        GameplayRuntime& runtime,
        const GameplayUpdateContext& context)
    {
        if (context.levelAsset == nullptr || context.levelInstance == nullptr || context.scene == nullptr ||
            context.mode != GameplayRuntimeMode::Game ||
            runtime.GetCurrentMode() != GameplayRuntimeMode::Game ||
            !runtime.IsCurrentLevelContext(context))
        {
            return AIActionExecutionStatus::Failed;
        }

        const auto agentNodeIndex = FindGameplayAIMovementDevelopmentNodeIndex_(
            *context.levelAsset, kStepDebugAgentNodeName);
        const auto targetNodeIndex = FindGameplayAIMovementDevelopmentNodeIndex_(
            *context.levelAsset, kStepDebugTargetNodeName);
        if (!agentNodeIndex || !targetNodeIndex)
        {
            return AIActionExecutionStatus::Failed;
        }

        EntityHandle agentEntity = FindGameplayAIMovementDevelopmentAgentEntity_(runtime, *agentNodeIndex);
        if (agentEntity == kNullEntity)
        {
            agentEntity = runtime.SpawnNodeBoundEntity(context, *agentNodeIndex, false);
        }

        GameplayWorld& world = runtime.GetWorld();
        const GameplayTransformComponent* transform = world.TryGetTransform(agentEntity);
        if (transform == nullptr)
        {
            return AIActionExecutionStatus::Failed;
        }
        if (!world.HasAI(agentEntity))
        {
            world.AddAI(agentEntity);
        }

        runtime.CancelAIAction(agentEntity);
        GameplayRoute route{
            .points = {
                GameplayRoutePoint{ .worldPosition = transform->position },
                GameplayRoutePoint{ .worldPosition = context.levelAsset->nodes[static_cast<std::size_t>(*targetNodeIndex)].transform.position }
            },
            .segmentAnnotations = { GameplayRouteSegmentAnnotation{} }
        };
        GameplayArrivalSteeringSettings steering{};
        steering.acceptanceRadius = 0.2f;
        steering.slowingRadius = 0.75f;
        steering.wantsRun = false;
        return runtime.StartAIFollowRoute(agentEntity, std::move(route), steering);
    }

    export [[nodiscard]] EntityHandle ResetGameplayAIStepDebugNPC(
        GameplayRuntime& runtime,
        const LevelAsset& levelAsset) noexcept
    {
        if (!runtime.IsCurrentLevelAsset(levelAsset))
        {
            return kNullEntity;
        }
        const auto agentNodeIndex = FindGameplayAIMovementDevelopmentNodeIndex_(levelAsset, kStepDebugAgentNodeName);
        if (!agentNodeIndex)
        {
            return kNullEntity;
        }
        const EntityHandle agentEntity = FindGameplayAIMovementDevelopmentAgentEntity_(runtime, *agentNodeIndex);
        if (agentEntity == kNullEntity)
        {
            return kNullEntity;
        }
        runtime.ClearAIAction(agentEntity);
        ResetGameplayAIMovementDevelopmentAgentForRestart_(
            runtime.GetWorld(), agentEntity,
            levelAsset.nodes[static_cast<std::size_t>(*agentNodeIndex)].transform.position,
            levelAsset.nodes[static_cast<std::size_t>(*agentNodeIndex)].transform);
        return agentEntity;
    }

    export void CancelGameplayAIStepDebugRoute(
        GameplayRuntime& runtime,
        const LevelAsset& levelAsset) noexcept
    {
        if (!runtime.IsCurrentLevelAsset(levelAsset))
        {
            return;
        }
        const auto agentNodeIndex = FindGameplayAIMovementDevelopmentNodeIndex_(levelAsset, kStepDebugAgentNodeName);
        if (!agentNodeIndex)
        {
            return;
        }
        const EntityHandle entity = FindGameplayAIMovementDevelopmentAgentEntity_(runtime, *agentNodeIndex);
        if (entity != kNullEntity)
        {
            runtime.CancelAIAction(entity);
        }
    }
}

export namespace rendern
{
    [[nodiscard]] AIActionExecutionStatus
    StartGameplayAIMovementDevelopmentScenario(
        GameplayRuntime& runtime,
        const GameplayUpdateContext& context)
    {
        const bool bHasValidContext =
            context.levelAsset != nullptr &&
            context.levelInstance != nullptr &&
            context.scene != nullptr;

        const bool bIsGameMode =
            context.mode == GameplayRuntimeMode::Game &&
            runtime.GetCurrentMode() == GameplayRuntimeMode::Game;

        const bool bIsCurrentContext =
            bHasValidContext &&
            runtime.IsCurrentLevelContext(context);

        if (!bHasValidContext ||
            !bIsGameMode ||
            !bIsCurrentContext)
        {
            return AIActionExecutionStatus::Failed;
        }

        const std::optional<GameplayAIMovementDevelopmentScenarioNodes>
            scenarioNodes = ResolveGameplayAIMovementDevelopmentScenarioNodes_(*context.levelAsset);

        if (!scenarioNodes.has_value())
        {
            return AIActionExecutionStatus::Failed;
        }

        GameplayRouteGraph routeGraph = BuildGameplayAIMovementDevelopmentRouteGraph_(
                *context.levelAsset,
                scenarioNodes->routeNodeIndices);

        if (!routeGraph.IsValid())
        {
            return AIActionExecutionStatus::Failed;
        }

        GameplayWorld& world = runtime.GetWorld();

        EntityHandle agentEntity =
            FindGameplayAIMovementDevelopmentAgentEntity_(
                runtime,
                scenarioNodes->agentNodeIndex);

        if (agentEntity == kNullEntity)
        {
            agentEntity = runtime.SpawnNodeBoundEntity(
                context,
                scenarioNodes->agentNodeIndex,
                false);
        }

        const bool bHasValidEntity =
            agentEntity != kNullEntity &&
            world.IsEntityValid(agentEntity);

        const bool bHasTransform =
            bHasValidEntity &&
            world.HasTransform(agentEntity);

        const bool bHasCharacterCommand =
            bHasValidEntity &&
            world.HasCharacterCommand(agentEntity);

        const bool bHasCharacterMotor =
            bHasValidEntity &&
            world.HasCharacterMotor(agentEntity);

        const bool bHasMovementState =
            bHasValidEntity &&
            world.HasCharacterMovementState(agentEntity);

        const bool bHasRequiredMovementComponents =
            bHasTransform &&
            bHasCharacterCommand &&
            bHasCharacterMotor &&
            bHasMovementState;

        if (!bHasValidEntity || !bHasRequiredMovementComponents)
        {
            return AIActionExecutionStatus::Failed;
        }

        if (!world.HasAI(agentEntity))
        {
            world.AddAI(agentEntity);
        }

        const int routeStartNodeIndex = scenarioNodes->routeNodeIndices.front();
        const auto& routeStartPosition =
            context.levelAsset->nodes[static_cast<std::size_t>(routeStartNodeIndex)].transform.position;
        const auto& agentCanonicalTransform =
            context.levelAsset->nodes[static_cast<std::size_t>(scenarioNodes->agentNodeIndex)].transform;

        runtime.CancelAIAction(agentEntity);

        ResetGameplayAIMovementDevelopmentAgentForRestart_(
            world,
            agentEntity,
            routeStartPosition,
            agentCanonicalTransform);

        GameplayArrivalSteeringSettings steering{};
        steering.acceptanceRadius = 0.2f;
        steering.slowingRadius = 0.75f;
        steering.wantsRun = false;

        return runtime.StartAIMoveTo(
            agentEntity,
            routeGraph,
            MakeGameplayAIMovementDevelopmentRouteNodeId_(scenarioNodes->routeNodeIndices.front()),
            MakeGameplayAIMovementDevelopmentRouteNodeId_(scenarioNodes->routeNodeIndices.back()),
            steering);
    }

    void CancelGameplayAIMovementDevelopmentScenario(
        GameplayRuntime& runtime,
        const LevelAsset& levelAsset)
    {
        const bool bIsCurrentLevel = runtime.IsCurrentLevelAsset(levelAsset);

        if (!bIsCurrentLevel)
        {
            return;
        }

        const std::optional<int> agentNodeIndex = FindGameplayAIMovementDevelopmentAgentNodeIndex_(levelAsset);

        if (!agentNodeIndex.has_value())
        {
            return;
        }

        const EntityHandle agentEntity =
            FindGameplayAIMovementDevelopmentAgentEntity_(
                runtime,
                *agentNodeIndex);

        if (agentEntity == kNullEntity)
        {
            return;
        }

        runtime.CancelAIAction(agentEntity);
    }
    
    [[nodiscard]] EntityHandle ResetGameplayAIMovementDevelopmentScenario(
        GameplayRuntime& runtime,
        const LevelAsset& levelAsset) noexcept
    {
        if (!runtime.IsCurrentLevelAsset(levelAsset))
        {
            return kNullEntity;
        }
        const auto scenarioNodes = ResolveGameplayAIMovementDevelopmentScenarioNodes_(levelAsset);
        if (!scenarioNodes.has_value())
        {
            return kNullEntity;
        }
        const EntityHandle agentEntity = FindGameplayAIMovementDevelopmentAgentEntity_(
            runtime, scenarioNodes->agentNodeIndex);
        if (agentEntity == kNullEntity)
        {
            return kNullEntity;
        }
        runtime.ClearAIAction(agentEntity);
        ResetGameplayAIMovementDevelopmentAgentForRestart_(
            runtime.GetWorld(), agentEntity,
            levelAsset.nodes[static_cast<std::size_t>(scenarioNodes->routeNodeIndices.front())].transform.position,
            levelAsset.nodes[static_cast<std::size_t>(scenarioNodes->agentNodeIndex)].transform);
        return agentEntity;
    }

    [[nodiscard]] AIActionExecutionStatus
    GetGameplayAIMovementDevelopmentScenarioStatus(
        const GameplayRuntime& runtime,
        const LevelAsset& levelAsset) noexcept
    {
        const bool bIsCurrentLevel = runtime.IsCurrentLevelAsset(levelAsset);

        if (!bIsCurrentLevel)
        {
            return AIActionExecutionStatus::NotStarted;
        }

        const std::optional<int> agentNodeIndex = FindGameplayAIMovementDevelopmentAgentNodeIndex_(levelAsset);

        if (!agentNodeIndex.has_value())
        {
            return AIActionExecutionStatus::NotStarted;
        }

        const EntityHandle agentEntity =
            FindGameplayAIMovementDevelopmentAgentEntity_(
                runtime,
                *agentNodeIndex);

        if (agentEntity == kNullEntity)
        {
            return AIActionExecutionStatus::NotStarted;
        }

        return runtime.GetAIActionStatus(agentEntity);
    }
}