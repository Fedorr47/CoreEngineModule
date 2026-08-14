#include <gtest/gtest.h>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    constexpr float kPositionTolerance = 0.3f;

    void AddScenarioNode(
        LevelAsset& levelAsset,
        const char* name,
        const mathUtils::Vec3& position,
        const bool visible = true)
    {
        LevelNode node{};
        node.name = name;
        node.alive = true;
        node.visible = visible;
        node.transform.position = position;
        levelAsset.nodes.push_back(node);
    }

    [[nodiscard]] LevelAsset MakeDevelopmentScenarioLevel()
    {
        LevelAsset levelAsset{};
        levelAsset.name = "AIMovementDevelopmentScenarioFixture";

        AddScenarioNode(levelAsset, "Player", {10.0f, 0.0f, 0.0f});

        AddScenarioNode(levelAsset, "AI_Move_Agent", {-2.0f, 0.0f, 0.0f});

        // Route nodes are deliberately stored out of order so the fixture
        // protects numeric suffix sorting instead of LevelAsset array order.
        AddScenarioNode(levelAsset, "AI_Move_Point_003", {4.0f, 0.0f, 1.5f}, false);
        AddScenarioNode(levelAsset, "AI_Move_Point_001", {0.0f, 0.0f, 0.0f}, false);
        AddScenarioNode(levelAsset, "AI_Move_Point_000", {-2.0f, 0.0f, 0.0f}, false);
        AddScenarioNode(levelAsset, "AI_Move_Point_002", {2.0f, 0.0f, 1.5f}, false);

        return levelAsset;
    }

    [[nodiscard]] LevelAsset MakeMinimalDevelopmentScenarioLevel()
    {
        LevelAsset levelAsset{};
        levelAsset.name = "MinimalAIMovementDevelopmentScenarioFixture";

        AddScenarioNode(levelAsset, "Player", {10.0f, 0.0f, 0.0f});
        AddScenarioNode(levelAsset, "AI_Move_Agent", {-1.0f, 0.0f, 0.0f});
        AddScenarioNode(levelAsset, "AI_Move_Point_001", {1.0f, 0.0f, 0.0f}, false);
        AddScenarioNode(levelAsset, "AI_Move_Point_000", {-1.0f, 0.0f, 0.0f}, false);

        return levelAsset;
    }

    [[nodiscard]] GameplayUpdateContext MakeScenarioContext(
        LevelAsset& levelAsset,
        LevelInstance& levelInstance,
        Scene& scene,
        const GameplayRuntimeMode mode = GameplayRuntimeMode::Game)
    {
        GameplayUpdateContext context{};
        context.mode = mode;
        context.deltaSeconds = 1.0f / 60.0f;
        context.levelAsset = &levelAsset;
        context.levelInstance = &levelInstance;
        context.scene = &scene;
        return context;
    }

    void EnterGameMode(GameplayRuntime& runtime, const GameplayUpdateContext& gameContext)
    {
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(gameContext);
        runtime.PostPhysicsUpdate(gameContext);
    }

    [[nodiscard]] int FindNodeIndexByName(const LevelAsset& levelAsset, const std::string_view nodeName) noexcept
    {
        for (std::size_t index = 0; index < levelAsset.nodes.size(); ++index)
        {
            const LevelNode& node = levelAsset.nodes[index];

            if (node.alive && node.name == nodeName)
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    [[nodiscard]] EntityHandle FindNodeBoundEntityByNodeName(
        const GameplayRuntime& runtime,
        const LevelAsset& levelAsset,
        const std::string_view nodeName) noexcept
    {
        const int nodeIndex = FindNodeIndexByName(levelAsset, nodeName);

        if (nodeIndex < 0)
        {
            return kNullEntity;
        }

        const GameplayWorld& world = runtime.GetWorld();

        for (const EntityHandle entity : runtime.GetNodeBoundEntities())
        {
            if (!world.IsEntityValid(entity))
            {
                continue;
            }

            const GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);

            const bool bMatchesNode = nodeLink != nullptr && nodeLink->nodeIndex == nodeIndex;

            if (bMatchesNode)
            {
                return entity;
            }
        }

        return kNullEntity;
    }

    [[nodiscard]] AIActionExecutionStatus RunScenarioUntilTerminal(
        GameplayRuntime& runtime,
        const GameplayUpdateContext& gameContext,
        const LevelAsset& levelAsset,
        const int maxFrameCount = 1200)
    {
        AIActionExecutionStatus status = GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset);

        for (int frameIndex = 0; frameIndex < maxFrameCount && status == AIActionExecutionStatus::Running; ++frameIndex)
        {
            runtime.BeginFrame();
            runtime.PrePhysicsUpdate(gameContext);
            runtime.PostPhysicsUpdate(gameContext);

            status = GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset);
        }

        return status;
    }
}

// Protects the development naming contract and verifies numeric route order
// is independent from the physical LevelAsset node array order.
TEST(GameplayAIMovementDevelopmentScenario, StartResolvesNumericallyOrderedRoutePoints)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    const EntityHandle agentEntity = FindNodeBoundEntityByNodeName(runtime, levelAsset, "AI_Move_Agent");

    ASSERT_NE(agentEntity, kNullEntity);

    const GameplayWorld& world = runtime.GetWorld();
    const GameplayTransformComponent* transform = world.TryGetTransform(agentEntity);

    ASSERT_NE(transform, nullptr);

    EXPECT_NEAR(transform->position.x, -2.0f, 0.001f);
    EXPECT_NEAR(transform->position.z, 0.0f, 0.001f);
    EXPECT_TRUE(world.HasAI(agentEntity));
    EXPECT_FALSE(world.HasPlayerControlled(agentEntity));
    EXPECT_FALSE(world.HasFollowCamera(agentEntity));
}

// Protects the minimum authored-route contract so two ordered points remain
// sufficient without retaining the old three-point development assumption.
TEST(GameplayAIMovementDevelopmentScenario, TwoPointRouteStartsSuccessfully)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeMinimalDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);
}

// Protects arbitrary route length by executing a four-point authored route
// through the public runtime frame until the final ordered point is reached.
TEST(GameplayAIMovementDevelopmentScenario, ArbitraryLengthRouteCompletesAtLastOrderedPoint)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    EXPECT_EQ(RunScenarioUntilTerminal(runtime, gameContext, levelAsset), AIActionExecutionStatus::Succeeded);

    const EntityHandle agentEntity = FindNodeBoundEntityByNodeName(runtime, levelAsset, "AI_Move_Agent");

    ASSERT_NE(agentEntity, kNullEntity);

    const GameplayTransformComponent* transform = runtime.GetWorld().TryGetTransform(agentEntity);

    ASSERT_NE(transform, nullptr);

    EXPECT_NEAR(transform->position.x, 4.0f, kPositionTolerance);

    EXPECT_NEAR(transform->position.z, 1.5f, kPositionTolerance);
}

// Protects deterministic restart semantics so restarting returns the agent to
// Point_000, clears stale movement, and preserves unrelated AI world facts.
TEST(GameplayAIMovementDevelopmentScenario, RestartResetsAgentToFirstPointAndClearsMovement)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    const EntityHandle agentEntity = FindNodeBoundEntityByNodeName(runtime, levelAsset, "AI_Move_Agent");

    ASSERT_NE(agentEntity, kNullEntity);

    GameplayWorld& world = runtime.GetWorld();

    GameplayTransformComponent* transform = world.TryGetTransform(agentEntity);

    GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(agentEntity);

    GameplayCharacterMotorComponent* motor = world.TryGetCharacterMotor(agentEntity);

    AIComponent* ai = world.TryGetAI(agentEntity);

    ASSERT_NE(transform, nullptr);
    ASSERT_NE(command, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(ai, nullptr);

    constexpr AIWorldFactId preservedFactId{7u};
    ai->worldState.SetFact(preservedFactId, true);

    transform->position = {7.0f, 0.0f, 7.0f};

    command->moveWorld = {1.0f, 0.0f, 0.0f};
    command->moveMagnitude = 1.0f;
    command->wantsRun = true;

    motor->velocity = {4.0f, 0.0f, 2.0f};
    motor->desiredMoveWorld = {1.0f, 0.0f, 0.0f};

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    EXPECT_NEAR(transform->position.x, -2.0f, 0.001f);
    EXPECT_NEAR(transform->position.y, 0.0f, 0.001f);
    EXPECT_NEAR(transform->position.z, 0.0f, 0.001f);

    EXPECT_FLOAT_EQ(command->moveMagnitude, 0.0f);
    EXPECT_FALSE(command->wantsRun);

    EXPECT_FLOAT_EQ(motor->velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(motor->velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(motor->velocity.z, 0.0f);

    EXPECT_FLOAT_EQ(motor->desiredMoveWorld.x, 0.0f);
    EXPECT_FLOAT_EQ(motor->desiredMoveWorld.y, 0.0f);
    EXPECT_FLOAT_EQ(motor->desiredMoveWorld.z, 0.0f);

    EXPECT_TRUE(ai->worldState.IsFactSet(preservedFactId));

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::Running);
}

TEST(GameplayAIMovementDevelopmentScenario, ResetRestoresCanonicalStateAndClearsAction)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();
    const int startIndex = FindNodeIndexByName(levelAsset, "AI_Move_Point_000");
    const int agentIndex = FindNodeIndexByName(levelAsset, "AI_Move_Agent");
    ASSERT_GE(startIndex, 0);
    ASSERT_GE(agentIndex, 0);
    levelAsset.nodes[static_cast<std::size_t>(startIndex)].transform.rotationDegrees.y = -70.0f;
    levelAsset.nodes[static_cast<std::size_t>(startIndex)].transform.scale = {0.5f, 0.5f, 0.5f};
    levelAsset.nodes[static_cast<std::size_t>(agentIndex)].transform.rotationDegrees = {5.0f, 35.0f, 10.0f};
    levelAsset.nodes[static_cast<std::size_t>(agentIndex)].transform.scale = {1.5f, 1.5f, 1.5f};
    LevelInstance levelInstance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(levelAsset, levelInstance, scene);
    const auto context = MakeScenarioContext(levelAsset, levelInstance, scene);
    EnterGameMode(runtime, context);
    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, context), AIActionExecutionStatus::Running);
    const EntityHandle entity = FindNodeBoundEntityByNodeName(runtime, levelAsset, "AI_Move_Agent");
    ASSERT_NE(entity, kNullEntity);
    auto& world = runtime.GetWorld();
    world.TryGetTransform(entity)->position = {9.0f, 8.0f, 7.0f};
    world.TryGetCharacterMotor(entity)->velocity = {3.0f, 2.0f, 1.0f};
    world.TryGetCharacterMotor(entity)->desiredVelocity = {1.0f, 1.0f, 1.0f};

    EXPECT_EQ(ResetGameplayAIMovementDevelopmentScenario(runtime, levelAsset), entity);
    const auto* transform = world.TryGetTransform(entity);
    EXPECT_EQ(transform->position, levelAsset.nodes[static_cast<std::size_t>(startIndex)].transform.position);
    EXPECT_EQ(transform->rotationDegrees, levelAsset.nodes[static_cast<std::size_t>(agentIndex)].transform.rotationDegrees);
    EXPECT_EQ(transform->scale, levelAsset.nodes[static_cast<std::size_t>(agentIndex)].transform.scale);
    EXPECT_EQ(world.TryGetCharacterMotor(entity)->velocity, mathUtils::Vec3{});
    EXPECT_EQ(world.TryGetCharacterMotor(entity)->desiredVelocity, mathUtils::Vec3{});
    const auto* movementState = world.TryGetCharacterMovementState(entity);
    ASSERT_NE(movementState, nullptr);
    EXPECT_FLOAT_EQ(movementState->facingYawDegrees, 35.0f);
    EXPECT_FLOAT_EQ(movementState->desiredFacingYawDegrees, 35.0f);
    EXPECT_FLOAT_EQ(movementState->previousFacingYawDegrees, 35.0f);
    EXPECT_FLOAT_EQ(movementState->cameraFacingYawDegrees, 35.0f);
    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::NotStarted);
}

// Protects action lookup from route-authoring changes so losing a route point
// after Start cannot hide or orphan the already-running agent action.
TEST(GameplayAIMovementDevelopmentScenario, MissingRoutePointAfterStartDoesNotBlockStatusOrCancellation)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    const int routePointIndex = FindNodeIndexByName(levelAsset, "AI_Move_Point_001");

    ASSERT_GE(routePointIndex, 0);

    levelAsset.nodes[static_cast<std::size_t>(routePointIndex)].alive = false;

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::Running);

    CancelGameplayAIMovementDevelopmentScenario(runtime, levelAsset);

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::Cancelled);
}

// Protects route-order uniqueness so differently formatted names cannot map
// to the same numeric position and produce ambiguous authored movement.
TEST(GameplayAIMovementDevelopmentScenario, DuplicateNumericRouteOrderFails)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeMinimalDevelopmentScenarioLevel();

    AddScenarioNode(levelAsset, "AI_Move_Point_00", {5.0f, 0.0f, 0.0f}, false);

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Failed);
}

// Protects malformed route-point authoring so a node using the reserved prefix
// must provide a complete numeric suffix instead of being silently ignored.
TEST(GameplayAIMovementDevelopmentScenario, MalformedRoutePointSuffixFails)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeMinimalDevelopmentScenarioLevel();

    AddScenarioNode(levelAsset, "AI_Move_Point_Invalid", {5.0f, 0.0f, 0.0f}, false);

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Failed);
}

// Protects the minimum route length so a single authored point cannot create
// an action that has no traversable segment.
TEST(GameplayAIMovementDevelopmentScenario, SingleRoutePointFails)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset{};
    levelAsset.name = "SinglePointScenario";

    AddScenarioNode(levelAsset, "Player", {10.0f, 0.0f, 0.0f});

    AddScenarioNode(levelAsset, "AI_Move_Agent", {-1.0f, 0.0f, 0.0f});

    AddScenarioNode(levelAsset, "AI_Move_Point_000", {-1.0f, 0.0f, 0.0f}, false);

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Failed);
}

// Protects the missing-node failure path under a valid Game-mode context so
// the test cannot pass early because the runtime is still in Editor mode.
TEST(GameplayAIMovementDevelopmentScenario, MissingScenarioNodesFailInGameMode)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset{};
    levelAsset.name = "MissingDevelopmentScenario";

    AddScenarioNode(levelAsset, "Player", {0.0f, 0.0f, 0.0f});

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Game);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Failed);

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::NotStarted);

    CancelGameplayAIMovementDevelopmentScenario(runtime, levelAsset);
}

// Protects runtime-level identity so another valid map with matching node
// indices cannot query, cancel, teleport, or restart the current map's agent.
TEST(GameplayAIMovementDevelopmentScenario, ForeignValidLevelCannotControlCurrentScenario)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset currentLevel = MakeDevelopmentScenarioLevel();

    LevelInstance currentLevelInstance{};
    Scene currentScene{};
    GameplayRuntime runtime{};

    runtime.Initialize(currentLevel, currentLevelInstance, currentScene);

    const GameplayUpdateContext currentContext = MakeScenarioContext(currentLevel, currentLevelInstance, currentScene);

    EnterGameMode(runtime, currentContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, currentContext), AIActionExecutionStatus::Running);

    const EntityHandle agentEntity = FindNodeBoundEntityByNodeName(runtime, currentLevel, "AI_Move_Agent");

    ASSERT_NE(agentEntity, kNullEntity);

    GameplayTransformComponent* transform = runtime.GetWorld().TryGetTransform(agentEntity);

    ASSERT_NE(transform, nullptr);

    const mathUtils::Vec3 positionBeforeForeignRequest = transform->position;

    LevelAsset foreignLevel = MakeDevelopmentScenarioLevel();

    LevelInstance foreignLevelInstance{};
    Scene foreignScene{};

    const GameplayUpdateContext foreignContext = MakeScenarioContext(foreignLevel, foreignLevelInstance, foreignScene);

    EXPECT_EQ(
        GetGameplayAIMovementDevelopmentScenarioStatus(runtime, foreignLevel),
        AIActionExecutionStatus::NotStarted);

    CancelGameplayAIMovementDevelopmentScenario(runtime, foreignLevel);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, foreignContext), AIActionExecutionStatus::Failed);

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, currentLevel), AIActionExecutionStatus::Running);

    EXPECT_FLOAT_EQ(transform->position.x, positionBeforeForeignRequest.x);

    EXPECT_FLOAT_EQ(transform->position.y, positionBeforeForeignRequest.y);

    EXPECT_FLOAT_EQ(transform->position.z, positionBeforeForeignRequest.z);
}

// Protects mode validation as a non-mutating boundary so an Editor-context
// restart request cannot cancel an already-running Game-mode action.
TEST(GameplayAIMovementDevelopmentScenario, EditorContextDoesNotCancelRunningScenario)
{
    InlineThreadOwnerRolesGuard guard{};

    LevelAsset levelAsset = MakeDevelopmentScenarioLevel();

    LevelInstance levelInstance{};
    Scene scene{};
    GameplayRuntime runtime{};

    runtime.Initialize(levelAsset, levelInstance, scene);

    const GameplayUpdateContext gameContext = MakeScenarioContext(levelAsset, levelInstance, scene);

    EnterGameMode(runtime, gameContext);

    ASSERT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, gameContext), AIActionExecutionStatus::Running);

    const GameplayUpdateContext editorContext =
        MakeScenarioContext(
            levelAsset,
            levelInstance,
            scene,
            GameplayRuntimeMode::Editor);

    EXPECT_EQ(StartGameplayAIMovementDevelopmentScenario(runtime, editorContext), AIActionExecutionStatus::Failed);

    EXPECT_EQ(GetGameplayAIMovementDevelopmentScenarioStatus(runtime, levelAsset), AIActionExecutionStatus::Running);
}