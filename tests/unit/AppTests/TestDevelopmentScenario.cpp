#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <limits>

import core;

#include "App/Development/DevelopmentScenario.h"
#include "App/Development/AppDevelopmentScenarioRuntime.h"
#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

using namespace appDevelopment;

namespace
{
    rendern::MeshCPU MakeCubeMesh()
    {
        rendern::MeshCPU mesh{};
        const auto vertex = [](const float x, const float y, const float z) {
            rendern::VertexDesc result{};
            result.px = x;
            result.py = y;
            result.pz = z;
            return result;
        };
        mesh.vertices = {
            vertex(-0.5f, -0.5f, 0.5f), vertex(0.5f, -0.5f, 0.5f),
            vertex(0.5f, 0.5f, 0.5f), vertex(-0.5f, 0.5f, 0.5f),
            vertex(-0.5f, -0.5f, -0.5f), vertex(0.5f, -0.5f, -0.5f),
            vertex(0.5f, 0.5f, -0.5f), vertex(-0.5f, 0.5f, -0.5f)};
        mesh.indices = {
            0, 1, 2, 0, 2, 3, 4, 7, 6, 4, 6, 5,
            4, 0, 3, 4, 3, 7, 1, 5, 6, 1, 6, 2,
            3, 2, 6, 3, 6, 7, 4, 5, 1, 4, 1, 0};
        return mesh;
    }
    
    void AddMoveToNode(rendern::LevelAsset& level, const char* name,
        const mathUtils::Vec3 position)
    {
        rendern::LevelNode node{};
        node.name = name; node.alive = true; node.transform.position = position;
        level.nodes.push_back(node);
    }

    rendern::EntityHandle FindRoleEntity(const DevelopmentScenarioRunner& runner,
        const rendern::GameplayRuntime& runtime, const char* role)
    {
        const int node = runner.GetResolvedNodeIndex(role);
        for (const auto entity : runtime.GetNodeBoundEntities())
            if (const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
                link && link->nodeIndex == node) return entity;
        return rendern::kNullEntity;
    }

    DevelopmentScenarioAsset FourNodeMoveToScenario()
    {
        return ParseDevelopmentScenarioAsset(R"({
          "id":"move.behavior", "title":"Move behavior",
          "roles":{"agent":"Agent","start":"Start","a":"A","b":"B","goal":"Goal"},
          "setup":[{"op":"ensureNodeBoundEntity","entity":"agent"},
                   {"op":"captureTransform","entity":"agent","slot":"baseline"},
                   {"op":"ensureAI","entity":"agent"}],
          "start":[{"op":"cancelAI","entity":"agent"},
                   {"op":"restoreTransform","entity":"agent","slot":"baseline"},
                   {"op":"resetEntitySimulationState","entity":"agent"},
                   {"op":"teleportPhysicsCharacter","entity":"agent"},
                   {"op":"startMoveTo","entity":"agent","nodes":["start","a","b","goal"],
                    "edges":[{"from":"start","to":"a","cost":1},{"from":"a","to":"b","cost":1},{"from":"b","to":"goal","cost":1}],
                    "start":"start","goal":"goal","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false}],
          "stop":[{"op":"cancelAI","entity":"agent"}],
          "reset":[{"op":"cancelAI","entity":"agent"},
                   {"op":"restoreTransform","entity":"agent","slot":"baseline"},
                   {"op":"resetEntitySimulationState","entity":"agent"},
                   {"op":"teleportPhysicsCharacter","entity":"agent"}]})");
    }

    rendern::LevelAsset FourNodeMoveToLevel()
    {
        rendern::LevelAsset level{};
        // Physical order intentionally disagrees with authored graph order.
        AddMoveToNode(level, "Goal", {4.0f, 0.0f, 1.5f});
        AddMoveToNode(level, "B", {2.0f, 0.0f, 1.5f});
        AddMoveToNode(level, "Agent", {-2.0f, 0.0f, 0.0f});
        AddMoveToNode(level, "Start", {-2.0f, 0.0f, 0.0f});
        AddMoveToNode(level, "A", {0.0f, 0.0f, 0.0f});
        return level;
    }
}

TEST(DevelopmentScenarioAsset, ParsesTypedOperationsAndRoles)
{
    const DevelopmentScenarioAsset asset = ParseDevelopmentScenarioAsset(R"({
        "id":"test.basic", "title":"Basic Scenario",
        "roles":{"agent":"Scenario_Agent"},
        "setup":[{"op":"captureTransform","entity":"agent","slot":"initial"},
                 {"op":"ensureAI","entity":"agent"}],
        "reset":[{"op":"cancelAI","entity":"agent"},
                 {"op":"restoreTransform","entity":"agent","slot":"initial"},
                 {"op":"teleportPhysicsCharacter","entity":"agent"}]
    })", "tests/basic.scenario.json");

    EXPECT_EQ(asset.id, "test.basic");
    ASSERT_EQ(asset.setup.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<CaptureTransformOperation>(asset.setup[0]));
    EXPECT_TRUE(std::holds_alternative<EnsureAIOperation>(asset.setup[1]));
}

TEST(DevelopmentScenarioAsset, ParsesPickupCollectedWorldMutation)
{
    const DevelopmentScenarioAsset asset=ParseDevelopmentScenarioAsset(R"({
        "id":"pickup","title":"Pickup mutation",
        "roles":{"coin":"Coin","trigger":"Trigger"},
        "update":[{"op":"setPickupCollected","entity":"coin","collected":true,
                   "whenPickupCollected":"trigger"}]
    })");
    ASSERT_EQ(asset.update.size(),1u);
    const auto* operation=std::get_if<SetPickupCollectedOperation>(&asset.update.front());
    ASSERT_NE(operation,nullptr);
    EXPECT_EQ(operation->entity,"coin"); EXPECT_TRUE(operation->collected);
    EXPECT_EQ(operation->whenPickupCollected,"trigger");

    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({
        "id":"pickup","title":"Pickup mutation","roles":{"coin":"Coin"},
        "update":[{"op":"setPickupCollected","entity":"coin","collected":1}]
    })"),std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({
        "id":"pickup","title":"Pickup mutation","roles":{"coin":"Coin"},
        "update":[{"op":"setPickupCollected","entity":"coin","collected":true,
                   "whenPickupCollected":true}]
    })"),std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({
        "id":"pickup","title":"Pickup mutation","roles":{"coin":"Coin"},
        "update":[{"op":"setRuntimeVisibility","entity":"coin","visible":false,
                   "whenPickupCollected":"missing"}]
    })"),std::runtime_error);
}

TEST(DevelopmentScenarioRunner, PickupConditionGatesPickupAndVisibilityMutations)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    level.meshes.emplace(
    "marker",
    rendern::LevelMeshDef{
        .path = "models/cube.obj",
        .debugName = "Conditional pickup marker"});
    AddMoveToNode(level, "Trigger", {0.0f, 0.0f, 0.0f});
    AddMoveToNode(level, "Target", {2.0f, 0.0f, 0.0f});
    level.nodes[1].mesh = "marker";
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelInstance instance=harness.Instantiate(level);
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level,instance,harness.GetScene());
    rendern::GameplayUpdateContext game{.mode=rendern::GameplayRuntimeMode::Game,
        .levelAsset=&level,.levelInstance=&instance,.scene=&harness.GetScene()};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    ScenarioContext context{runtime,level,instance,harness.GetScene(),
        rendern::GameplayRuntimeMode::Game};
    const DevelopmentScenarioAsset scenario=ParseDevelopmentScenarioAsset(R"({
        "id":"conditional-pickup","title":"Conditional pickup",
        "roles":{"trigger":"Trigger","target":"Target"},
        "setup":[
          {"op":"ensureNodeBoundEntity","entity":"trigger"},
          {"op":"ensurePickup","entity":"trigger","collectionRadius":0.6},
          {"op":"ensureNodeBoundEntity","entity":"target"},
          {"op":"ensurePickup","entity":"target","collectionRadius":0.6}],
        "update":[
          {"op":"setPickupCollected","entity":"target","collected":true,
           "whenPickupCollected":"trigger"},
          {"op":"setRuntimeVisibility","entity":"target","visible":false,
           "whenPickupCollected":"trigger"}]
    })");
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario,context)); ASSERT_TRUE(runner.Start(context));
    const rendern::EntityHandle trigger=FindRoleEntity(runner,runtime,"trigger");
    const rendern::EntityHandle target=FindRoleEntity(runner,runtime,"target");
    ASSERT_NE(trigger,rendern::kNullEntity); ASSERT_NE(target,rendern::kNullEntity);
    auto* triggerPickup=runtime.GetWorld().TryGetPickup(trigger);
    auto* targetPickup=runtime.GetWorld().TryGetPickup(target);
    ASSERT_NE(triggerPickup,nullptr); ASSERT_NE(targetPickup,nullptr);
    const int targetNode=runner.GetResolvedNodeIndex("target");

    runner.Update(context);
    EXPECT_TRUE(runner.IsRunning()); EXPECT_FALSE(targetPickup->collected);
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(targetNode));
    triggerPickup->collected=true;
    runner.Update(context);
    EXPECT_TRUE(targetPickup->collected);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(targetNode));
    runner.Update(context);
    EXPECT_TRUE(targetPickup->collected);
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(targetNode));
    runner.Unload(context);

    DevelopmentScenarioAsset missingTriggerPickup=scenario;
    missingTriggerPickup.setup.erase(missingTriggerPickup.setup.begin()+1);
    ASSERT_TRUE(runner.Load(missingTriggerPickup,context));
    ASSERT_TRUE(runner.Start(context));
    runner.Update(context);
    EXPECT_FALSE(runner.IsRunning());
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioAsset, ShippedScenarioInventoryAndLevelReferencesAreValid)
{
    namespace fs = std::filesystem;
    const fs::path repositoryRoot{CORE_REPOSITORY_ROOT};
    const fs::path developmentRoot = repositoryRoot / "assets/development";
    const fs::path levelsRoot = repositoryRoot / "assets/levels";

    std::vector<fs::path> scenarioPaths;
    for (const fs::directory_entry& entry : fs::directory_iterator(developmentRoot))
    {
        if (entry.is_regular_file() && entry.path().filename().string().ends_with(".scenario.json"))
        {
            scenarioPaths.push_back(entry.path());
        }
    }
    std::ranges::sort(scenarioPaths);

    const std::set<std::string> expectedScenarios{
        "ai_goap_access_key.scenario.json",
        "ai_goap_access_key_jump.scenario.json",
        "ai_goap_access_key_replanning.scenario.json",
        "ai_goap_access_key_reservation.scenario.json",
        "ai_jump_traversal.scenario.json",
        "ai_movement.scenario.json",
        "ai_physics_step.scenario.json",
        "navigation_agent_size.scenario.json"};
    std::set<std::string> actualScenarios;
    for (const fs::path& path : scenarioPaths)
    {
        actualScenarios.insert(path.filename().string());
        EXPECT_NO_THROW({
            const DevelopmentScenarioAsset asset = LoadDevelopmentScenarioAsset(path.string());
            ValidateDevelopmentScenarioAsset(asset);
        }) << path.string();
    }
    EXPECT_EQ(actualScenarios, expectedScenarios);

    std::vector<fs::path> levelPaths;
    for (const fs::directory_entry& entry : fs::directory_iterator(levelsRoot))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
        {
            levelPaths.push_back(entry.path());
        }
    }
    std::ranges::sort(levelPaths);

    std::size_t referencedLevelCount = 0;
    for (const fs::path& path : levelPaths)
    {
        if (!FILE_UTILS::ReadAllText(path).contains("\"developmentScenario\""))
        {
            continue;
        }
        ++referencedLevelCount;
        const rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(path.string());
        ASSERT_FALSE(level.developmentScenario.empty()) << path.string();
        const fs::path scenarioPath = repositoryRoot / "assets" / level.developmentScenario;
        ASSERT_TRUE(fs::is_regular_file(scenarioPath))
            << path.string() << " references " << level.developmentScenario;
        const DevelopmentScenarioAsset scenario = LoadDevelopmentScenarioAsset(scenarioPath.string());
        ValidateDevelopmentScenarioAsset(scenario);
        for (const auto& [role, nodeName] : scenario.roles)
        {
            EXPECT_TRUE(std::ranges::any_of(level.nodes, [&](const rendern::LevelNode& node) {
                return node.alive && node.name == nodeName;
            })) << path.string() << ": role '" << role << "' references missing node '"
               << nodeName << "'";
        }
    }
    EXPECT_EQ(referencedLevelCount, 8u);
}

TEST(DevelopmentScenarioAsset, RejectsDuplicateJsonKeysAndReportsOperationContext)
{
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"duplicate","id":"ambiguous","title":"Bad","roles":{}})",
        "development/duplicate.scenario.json"), std::runtime_error);

    try
    {
        (void)ParseDevelopmentScenarioAsset(R"({
            "id":"diagnostic","title":"Diagnostic","roles":{"agent":"Agent"},
            "start":[{"op":"startAIDecision","entity":"guard","decision":"test","result":"decision"}]
        })", "development/diagnostic.scenario.json");
        FAIL() << "Expected invalid scenario";
    }
    catch (const std::runtime_error& error)
    {
        const std::string_view message{error.what()};
        EXPECT_TRUE(message.contains("development/diagnostic.scenario.json"));
        EXPECT_TRUE(message.contains("start[0]"));
        EXPECT_TRUE(message.contains("startAIDecision"));
        EXPECT_TRUE(message.contains("unknown entity role 'guard'"));
    }
}
TEST(DevelopmentScenarioAsset, RejectsUnknownAndMalformedOperations)
{
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"setup":[{"op":"arbitraryCall","entity":"agent"}]})"),
        std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"setup":[{"op":"captureTransform","entity":"agent"}]})"),
        std::runtime_error);
}

TEST(DevelopmentScenarioAsset, RejectsUnknownRoleAndImpossibleRestore)
{
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"setup":[{"op":"ensureAI","entity":"missing"}]})"),
        std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"reset":[{"op":"restoreTransform","entity":"agent","slot":"never"}]})"),
        std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"update":[{"op":"captureTransform","entity":"agent","slot":"late"}],"reset":[{"op":"restoreTransform","entity":"agent","slot":"late"}]})"),
        std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(
        R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},"update":[{"op":"captureTransform","entity":"agent","slot":"late"}],"stop":[{"op":"restoreTransform","entity":"agent","slot":"late"}]})"),
        std::runtime_error);
}

TEST(DevelopmentScenarioAsset, ParsesAndValidatesTraversalLinkAndFollowRouteOperations)
{
    const auto asset = ParseDevelopmentScenarioAsset(R"({
      "id":"route", "title":"Route", "roles":{"agent":"Agent","entry":"Entry","target":"Target"},
      "start":[
        {"op":"registerJumpTraversalLink","entity":"target","handle":7,"takeoff":"entry","landing":"target",
         "verticalSpeed":5,"takeoffTolerance":0.2,"landingHorizontalTolerance":0.5,"landingVerticalTolerance":0.3},
        {"op":"startFollowRoute","entity":"agent","points":["entry","target"],
         "segmentTraversals":[{"segment":0,"link":7}],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false}
      ]})");
    ASSERT_EQ(asset.start.size(), 2u);
    EXPECT_TRUE(std::holds_alternative<RegisterJumpTraversalLinkOperation>(asset.start[0]));
    const auto* route = std::get_if<StartFollowRouteOperation>(&asset.start[1]);
    ASSERT_NE(route, nullptr);
    EXPECT_EQ(route->points, (std::vector<std::string>{"entry", "target"}));
    ASSERT_EQ(route->traversals.size(), 1u);
    EXPECT_EQ(route->traversals[0].segment, 0u);

    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({"id":"bad","title":"Bad","roles":{"agent":"Agent"},
      "start":[{"op":"startFollowRoute","entity":"agent","points":["agent"],"segmentTraversals":[],
      "acceptanceRadius":0.2,"slowingRadius":0.1,"wantsRun":false}]})"), std::runtime_error);
}

TEST(DevelopmentScenarioAsset, ParsesAndValidatesMoveToGraphOperation)
{
    const auto parse = [](std::string_view operation) {
        return ParseDevelopmentScenarioAsset(std::string(R"({"id":"move","title":"Move","roles":{"agent":"Agent","a":"A","b":"B"},"start":[)") +
            std::string(operation) + "]}");
    };
    const auto asset = parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","b"],"edges":[{"from":"a","to":"b","cost":1.0}],"start":"a","goal":"b","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})");
    const auto* move = std::get_if<StartMoveToOperation>(&asset.start[0]);
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->nodes, (std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(move->edges.size(), 1u);

    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","missing"],"edges":[],"start":"a","goal":"missing","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})"), std::runtime_error);
    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","a"],"edges":[],"start":"a","goal":"a","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})"), std::runtime_error);
    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","b"],"edges":[{"from":"a","to":"agent","cost":1}],"start":"a","goal":"b","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})"), std::runtime_error);
    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","b"],"edges":[{"from":"a","to":"b","cost":-1}],"start":"a","goal":"b","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})"), std::runtime_error);
    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","b"],"edges":[],"start":"a","acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false})"), std::runtime_error);
    EXPECT_THROW(parse(R"({"op":"startMoveTo","entity":"agent","nodes":["a","b"],"edges":[],"start":"a","goal":"b","acceptanceRadius":0.8,"slowingRadius":0.2,"wantsRun":false})"), std::runtime_error);
}

TEST(DevelopmentScenarioRunner, MoveToUsesAuthoredGraphOrderAndCompletesAtGoal)
{
    InlineThreadOwnerRolesGuard guard{};
    auto level = FourNodeMoveToLevel();
    auto scenario = FourNodeMoveToScenario();
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext game{.deltaSeconds=1.0f/60.0f,
        .mode=rendern::GameplayRuntimeMode::Game, .levelAsset=&level, .levelInstance=&instance, .scene=&scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    ASSERT_EQ(runner.GetResolvedNodeIndex("goal"), 0);
    ASSERT_EQ(runner.GetResolvedNodeIndex("start"), 3);
    ASSERT_TRUE(runner.Start(context));
    const auto agent = FindRoleEntity(runner, runtime, "agent");
    ASSERT_NE(agent, rendern::kNullEntity);
    auto status = runtime.GetAIActionStatus(agent);
    for (int frame = 0; frame < 1200 && status == rendern::AIActionExecutionStatus::Running; ++frame)
    {
        runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
        status = runtime.GetAIActionStatus(agent);
    }
    EXPECT_EQ(status, rendern::AIActionExecutionStatus::Succeeded);
    const auto* transform = runtime.GetWorld().TryGetTransform(agent);
    ASSERT_NE(transform, nullptr);
    EXPECT_NEAR(transform->position.x, 4.0f, 0.3f);
    EXPECT_NEAR(transform->position.z, 1.5f, 0.3f);
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, MoveToRestartAndResetRestoreStateWithoutRemovingAIMembership)
{
    InlineThreadOwnerRolesGuard guard{};
    auto level = FourNodeMoveToLevel(); auto scenario = FourNodeMoveToScenario();
    level.nodes[2].transform.rotationDegrees.y = 35.0f;
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext game{.deltaSeconds=1.0f/60.0f,
        .mode=rendern::GameplayRuntimeMode::Game, .levelAsset=&level, .levelInstance=&instance, .scene=&scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{}; ASSERT_TRUE(runner.Load(scenario, context)); ASSERT_TRUE(runner.Start(context));
    const auto agent = FindRoleEntity(runner, runtime, "agent"); ASSERT_NE(agent, rendern::kNullEntity);
    auto& world = runtime.GetWorld();
    auto mutate = [&] {
        world.TryGetTransform(agent)->position = {7, 8, 9};
        auto* command = world.TryGetCharacterCommand(agent); command->moveWorld = {1, 0, 0}; command->moveMagnitude = 1; command->wantsRun = true;
        auto* motor = world.TryGetCharacterMotor(agent); motor->velocity = {4, 3, 2}; motor->desiredVelocity = {3, 2, 1}; motor->desiredMoveWorld = {1, 0, 0};
        auto* movement = world.TryGetCharacterMovementState(agent); movement->facingYawDegrees = movement->desiredFacingYawDegrees = movement->previousFacingYawDegrees = movement->cameraFacingYawDegrees = 123;
    };
    mutate(); runner.Stop(context); ASSERT_TRUE(runner.Start(context));
    const auto expectCanonical = [&] {
        EXPECT_EQ(world.TryGetTransform(agent)->position, level.nodes[2].transform.position);
        const auto* command = world.TryGetCharacterCommand(agent); EXPECT_FLOAT_EQ(command->moveMagnitude, 0); EXPECT_FALSE(command->wantsRun); EXPECT_EQ(command->moveWorld, mathUtils::Vec3{});
        const auto* motor = world.TryGetCharacterMotor(agent); EXPECT_EQ(motor->velocity, mathUtils::Vec3{}); EXPECT_EQ(motor->desiredVelocity, mathUtils::Vec3{}); EXPECT_EQ(motor->desiredMoveWorld, mathUtils::Vec3{});
        const auto* movement = world.TryGetCharacterMovementState(agent); EXPECT_FLOAT_EQ(movement->facingYawDegrees, 35); EXPECT_FLOAT_EQ(movement->desiredFacingYawDegrees, 35); EXPECT_FLOAT_EQ(movement->previousFacingYawDegrees, 35); EXPECT_FLOAT_EQ(movement->cameraFacingYawDegrees, 35);
    };
    expectCanonical(); EXPECT_TRUE(world.HasAI(agent));
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Running);
    mutate(); runner.Reset(context); expectCanonical();
    EXPECT_TRUE(world.HasAI(agent));
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::NotStarted);
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, MoveToCancellationDoesNotDependOnLiveGraphMarkers)
{
    InlineThreadOwnerRolesGuard guard{};
    auto level = FourNodeMoveToLevel(); auto scenario = FourNodeMoveToScenario();
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext game{.mode=rendern::GameplayRuntimeMode::Game,
        .levelAsset=&level, .levelInstance=&instance, .scene=&scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{}; ASSERT_TRUE(runner.Load(scenario, context)); ASSERT_TRUE(runner.Start(context));
    const auto agent = FindRoleEntity(runner, runtime, "agent"); ASSERT_NE(agent, rendern::kNullEntity);
    level.nodes[static_cast<std::size_t>(runner.GetResolvedNodeIndex("a"))].alive = false;
    runner.Stop(context);
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Cancelled);
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioAsset, PreservesWideHandlesAndRejectsUnsafeIntegerFields)
{
    const auto parse = [](const std::string_view handle, const std::string_view segment = "0") {
        return ParseDevelopmentScenarioAsset(std::string(R"({"id":"wide","title":"Wide","roles":{"agent":"Agent","target":"Target"},"start":[{"op":"registerJumpTraversalLink","entity":"target","handle":)") +
            std::string(handle) + R"(,"takeoff":"target","landing":"target","verticalSpeed":5,"takeoffTolerance":0.2,"landingHorizontalTolerance":0.5,"landingVerticalTolerance":0.3},{"op":"startFollowRoute","entity":"agent","points":["agent","target"],"segmentTraversals":[{"segment":)" +
            std::string(segment) + R"(,"link":)" + std::string(handle) +
            R"(}],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false}]})");
    };
    const auto asset = parse("4294967297");
    ASSERT_EQ(std::get<RegisterJumpTraversalLinkOperation>(asset.start[0]).handle, 4294967297ull);
    ASSERT_EQ(std::get<StartFollowRouteOperation>(asset.start[1]).traversals[0].link, 4294967297ull);
    EXPECT_EQ(std::get<RegisterJumpTraversalLinkOperation>(parse("0").start[0]).handle, 0u);
    EXPECT_THROW(parse("-1"), std::runtime_error);
    EXPECT_THROW(parse("1.5"), std::runtime_error);
    EXPECT_THROW(parse("9007199254740992"), std::runtime_error);
    EXPECT_THROW(parse("18446744073709551616"), std::runtime_error);
    EXPECT_THROW(parse("7", "-1"), std::runtime_error);
    EXPECT_THROW(parse("7", "0.5"), std::runtime_error);
    EXPECT_THROW(parse("7", "18446744073709551616"), std::runtime_error);
}

TEST(DevelopmentScenarioAsset, LevelReferenceIsOptionalAndRoundTrips)
{
    rendern::LevelAsset legacy{};
    EXPECT_TRUE(legacy.developmentScenario.empty());

    const rendern::LevelAsset authored = rendern::LoadLevelAssetFromJson(
        "tests/development/basic.level.json");
    EXPECT_EQ(authored.developmentScenario, "tests/development/basic.scenario.json");
    const auto scenario = LoadDevelopmentScenarioAsset(authored.developmentScenario);
    EXPECT_EQ(scenario.id, "test.basic");

    const auto path = std::filesystem::temp_directory_path() / "core_scenario_roundtrip.level.json";
    rendern::SaveLevelAssetToJson(path.string(), authored);
    const rendern::LevelAsset reloaded = rendern::LoadLevelAssetFromJson(path.string());
    EXPECT_EQ(reloaded.developmentScenario, authored.developmentScenario);
    std::filesystem::remove(path);
}

TEST(DevelopmentScenarioRunner, ExercisesFrameworkFixtureAndKeepsMarkerRolesNodeOnly)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson("tests/development/basic.level.json");
    DevelopmentScenarioAsset scenario = LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelInstance instance = harness.Instantiate(level);
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};

    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    rendern::EntityHandle agent = rendern::kNullEntity;
    for (const auto entity : runtime.GetNodeBoundEntities())
        if (const auto* link = runtime.GetWorld().TryGetNodeLink(entity); link && link->nodeIndex == 0) agent = entity;
    ASSERT_NE(agent, rendern::kNullEntity);
    EXPECT_EQ(runner.GetResolvedNodeIndex("agent"), 0);
    EXPECT_EQ(runner.GetResolvedNodeIndex("marker"), 1);
    EXPECT_TRUE(runtime.GetWorld().HasAI(agent));
    EXPECT_FALSE(runtime.GetWorld().HasPlayerControlled(agent));
    EXPECT_FALSE(instance.IsNodeRuntimeVisible(1));
    EXPECT_TRUE(level.nodes[1].visible); // runtime visibility never mutates authored data

    auto* transform = runtime.GetWorld().TryGetTransform(agent);
    ASSERT_NE(transform, nullptr);
    rendern::GameplayRoute route{
        .points={{{0.0f, 0.0f, 0.0f}}, {{10.0f, 0.0f, 0.0f}}},
        .segmentAnnotations={{}}};
    ASSERT_EQ(runtime.StartAIFollowRoute(agent, std::move(route)),
        rendern::AIActionExecutionStatus::Running);
    transform->position = {20.0f, 30.0f, 40.0f};
    runner.Reset(context);
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Cancelled);
    EXPECT_EQ(transform->position, level.nodes[0].transform.position);
    runner.Reset(context);
    EXPECT_EQ(transform->position, level.nodes[0].transform.position);
    EXPECT_TRUE(runner.Start(context));
    runner.Reset(context);
    EXPECT_TRUE(runner.Start(context));
    runner.Unload(context);
    EXPECT_FALSE(runner.IsLoaded());
    EXPECT_FALSE(runtime.GetWorld().IsEntityValid(agent));
    EXPECT_FALSE(runtime.GetWorld().HasAI(agent));
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(1));
    EXPECT_EQ(std::ranges::count(runtime.GetNodeBoundEntities(), agent), 0);

    ASSERT_TRUE(runner.Load(scenario, context));
    rendern::EntityHandle reloadedAgent = rendern::kNullEntity;
    std::size_t agentMappingCount = 0;
    for (const auto entity : runtime.GetNodeBoundEntities())
    {
        const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
        if (runtime.GetWorld().IsEntityValid(entity) && link && link->nodeIndex == 0)
        {
            reloadedAgent = entity;
            ++agentMappingCount;
        }
    }
    EXPECT_NE(reloadedAgent, rendern::kNullEntity);
    EXPECT_NE(reloadedAgent, agent);
    EXPECT_EQ(agentMappingCount, 1u);
    runner.Unload(context);

    DevelopmentScenarioAsset invalidMarkerOperation = scenario;
    invalidMarkerOperation.setup.push_back(EnsureAIOperation{"marker"});
    EXPECT_FALSE(runner.Load(invalidMarkerOperation, context));
    EXPECT_FALSE(runner.IsLoaded());
    EXPECT_TRUE(instance.IsNodeRuntimeVisible(1));
    runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, InstancesDoNotShareCapturedTransforms)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    rendern::LevelNode node{}; node.name = "Agent"; node.alive = true; level.nodes.push_back(node);
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext gameplayContext{.mode = rendern::GameplayRuntimeMode::Editor,
        .levelAsset = &level, .levelInstance = &instance, .scene = &scene};
    const auto entity = runtime.SpawnNodeBoundEntity(gameplayContext, 0, false);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioAsset asset{.id="independent", .title="Independent", .roles={{"agent", "Agent"}},
        .setup={CaptureTransformOperation{"agent", "baseline"}},
        .reset={RestoreTransformOperation{"agent", "baseline"}}};
    DevelopmentScenarioRunner first{}, second{};
    ASSERT_TRUE(first.Load(asset, context));
    runtime.GetWorld().TryGetTransform(entity)->position.x = 10.0f;
    ASSERT_TRUE(second.Load(asset, context));
    runtime.GetWorld().TryGetTransform(entity)->position.x = 20.0f;
    first.Reset(context); EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetTransform(entity)->position.x, 0.0f);
    runtime.GetWorld().TryGetTransform(entity)->position.x = 20.0f;
    second.Reset(context); EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetTransform(entity)->position.x, 10.0f);
    first.Unload(context); second.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, RunsAuthoredJumpThroughProductionTraversalAndRouteSystems)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_jump_traversal_development.level.json");
    const DevelopmentScenarioAsset scenario = LoadDevelopmentScenarioAsset(level.developmentScenario);
    const auto* authoredLink = std::get_if<RegisterJumpTraversalLinkOperation>(&scenario.start[5]);
    const auto* authoredRoute = std::get_if<StartFollowRouteOperation>(&scenario.start[6]);
    ASSERT_NE(authoredLink, nullptr);
    ASSERT_NE(authoredRoute, nullptr);
    EXPECT_EQ(authoredLink->takeoff, "takeoff");
    EXPECT_NE(authoredLink->takeoff, "traversalEntry");
    EXPECT_EQ(authoredRoute->points, (std::vector<std::string>{
        "routeStart", "traversalEntry", "landing", "postLanding", "routeFinish"}));
    ASSERT_EQ(authoredRoute->traversals.size(), 1u);
    EXPECT_EQ(authoredRoute->traversals[0].segment, 1u);
    EXPECT_EQ(authoredRoute->traversals[0].link, 4470001u);
    ASSERT_EQ(authoredRoute->points.size() - 1, 4u);
    std::array<std::optional<std::uint64_t>, 4> annotations{};
    for (const auto& traversal : authoredRoute->traversals)
        annotations[traversal.segment] = traversal.link;
    EXPECT_FALSE(annotations[0].has_value());
    ASSERT_TRUE(annotations[1].has_value());
    EXPECT_EQ(*annotations[1], 4470001u);
    EXPECT_FALSE(annotations[2].has_value());
    EXPECT_FALSE(annotations[3].has_value());
    rendern::LevelInstance instance{};
    rendern::Scene scene{};
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext gameContext{.mode = rendern::GameplayRuntimeMode::Game,
        .levelAsset = &level, .levelInstance = &instance, .scene = &scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameContext); runtime.PostPhysicsUpdate(gameContext);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));

    auto findRoleEntity = [&](const char* role) {
        const int node = runner.GetResolvedNodeIndex(role);
        for (const auto entity : runtime.GetNodeBoundEntities())
            if (const auto* link = runtime.GetWorld().TryGetNodeLink(entity); link && link->nodeIndex == node) return entity;
        return rendern::kNullEntity;
    };
    const auto agent = findRoleEntity("agent");
    const auto landing = findRoleEntity("landing");
    ASSERT_NE(agent, rendern::kNullEntity);
    ASSERT_NE(landing, rendern::kNullEntity);
    EXPECT_TRUE(runtime.GetWorld().HasAI(agent));
    EXPECT_FALSE(runtime.GetWorld().HasPlayerControlled(agent));
    EXPECT_EQ(runtime.GetWorld().TryGetCharacterPhysicalSettings(landing), nullptr);

    ASSERT_TRUE(runner.Start(context));
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Running);
    const rendern::GameplayTraversalLinkHandle handle{4470001u};
    const auto link = runtime.FindGameplayTraversalLink(handle);
    ASSERT_TRUE(link.has_value());
    EXPECT_EQ(link->traversalTypeId, rendern::kJumpTraversalTypeId);
    EXPECT_EQ(link->targetEntity, landing);
    EXPECT_EQ(link->jump.takeoffPosition,
        level.nodes[static_cast<std::size_t>(runner.GetResolvedNodeIndex("takeoff"))].transform.position);
    EXPECT_EQ(link->jump.landingPosition,
        level.nodes[static_cast<std::size_t>(runner.GetResolvedNodeIndex("landing"))].transform.position);
    EXPECT_FLOAT_EQ(link->jump.verticalSpeed, 5.5f);
    EXPECT_FLOAT_EQ(link->jump.takeoffTolerance, 0.20f);
    EXPECT_FLOAT_EQ(link->jump.landingHorizontalTolerance, 0.55f);
    EXPECT_FLOAT_EQ(link->jump.landingVerticalTolerance, 0.30f);

    auto* action = runtime.GetWorld().TryGetAction(agent);
    ASSERT_NE(action, nullptr);
    action->current = rendern::kGameplayActionJump;
    runtime.GetWorld().TryGetTransform(agent)->position = {9.0f, 8.0f, 7.0f};
    runner.Stop(context);
    auto* movement = runtime.GetWorld().TryGetCharacterMovementState(agent);
    ASSERT_NE(movement, nullptr);
    movement->facingYawDegrees = movement->desiredFacingYawDegrees = 123.0f;
    ASSERT_TRUE(runner.Start(context));
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(agent)->position,
        level.nodes[static_cast<std::size_t>(runner.GetResolvedNodeIndex("agent"))].transform.position);
    EXPECT_FLOAT_EQ(movement->facingYawDegrees,
        runtime.GetWorld().TryGetTransform(agent)->rotationDegrees.y);
    EXPECT_FLOAT_EQ(movement->desiredFacingYawDegrees, movement->facingYawDegrees);
    EXPECT_FLOAT_EQ(movement->previousFacingYawDegrees, movement->facingYawDegrees);
    EXPECT_FLOAT_EQ(movement->cameraFacingYawDegrees, movement->facingYawDegrees);
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Running);
    runner.Reset(context);
    EXPECT_FALSE(runtime.FindGameplayTraversalLink(handle).has_value());
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::NotStarted);
    EXPECT_EQ(action->current, rendern::GameplayActionId{});
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(agent)->position,
        level.nodes[static_cast<std::size_t>(runner.GetResolvedNodeIndex("agent"))].transform.position);
    ASSERT_TRUE(runner.Start(context));
    runner.Stop(context);
    EXPECT_EQ(runtime.GetAIActionStatus(agent), rendern::AIActionExecutionStatus::Cancelled);
    EXPECT_FALSE(runtime.FindGameplayTraversalLink(handle).has_value());
    runner.Unload(context);
    EXPECT_FALSE(runtime.GetWorld().IsEntityValid(agent));
    EXPECT_FALSE(runtime.GetWorld().IsEntityValid(landing));
    runtime.Shutdown();
}


TEST(DevelopmentScenarioRunner, FailedRegistrationNeverRemovesForeignTraversalLink)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_jump_traversal_development.level.json");
    const auto scenario = LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext gameContext{.mode = rendern::GameplayRuntimeMode::Game,
        .levelAsset = &level, .levelInstance = &instance, .scene = &scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(gameContext); runtime.PostPhysicsUpdate(gameContext);
    ScenarioContext context{runtime, level, instance, scene, rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    const rendern::GameplayTraversalLinkHandle handle{4470001u};
    const auto foreignTarget = runtime.GetWorld().CreateEntity();
    const rendern::GameplayTraversalLink foreign{
        .handle = handle, .traversalTypeId = rendern::kJumpTraversalTypeId, .targetEntity = foreignTarget,
        .jump = {.takeoffPosition = {1, 2, 3}, .landingPosition = {4, 5, 6}, .verticalSpeed = 8,
            .takeoffTolerance = .4f, .landingHorizontalTolerance = .6f, .landingVerticalTolerance = .8f}};
    ASSERT_TRUE(runtime.RegisterGameplayTraversalLink(foreign));
    EXPECT_FALSE(runner.Start(context));
    auto preserved = runtime.FindGameplayTraversalLink(handle);
    ASSERT_TRUE(preserved.has_value());
    EXPECT_EQ(preserved->targetEntity, foreign.targetEntity);
    EXPECT_EQ(preserved->jump.takeoffPosition, foreign.jump.takeoffPosition);
    runner.Stop(context); runner.Reset(context); runner.Unload(context);
    preserved = runtime.FindGameplayTraversalLink(handle);
    ASSERT_TRUE(preserved.has_value());
    EXPECT_EQ(preserved->targetEntity, foreign.targetEntity);
    EXPECT_EQ(preserved->jump.verticalSpeed, foreign.jump.verticalSpeed);
    runtime.Shutdown();
}

TEST(DevelopmentScenarioAsset, ParsesAndValidatesPhysicalSettingsAndNavigationPath)
{
    const auto parse = [](const std::string_view physicalValues,
        const std::string_view pathValues = R"("searchExtents":[1,2,1],"acceptanceRadius":0.2,"slowingRadius":0.75)") {
        return ParseDevelopmentScenarioAsset(std::string(R"({"id":"navigation","title":"Navigation","roles":{"agent":"Agent","target":"Target"},"setup":[{"op":"setCharacterPhysicalSettings","entity":"agent",)") +
            std::string(physicalValues) + R"(}],"start":[{"op":"startNavigationPath","entity":"agent","target":"target",)" +
            std::string(pathValues) + R"(,"wantsRun":false,"result":"path"}]})");
    };
    constexpr std::string_view validPhysical =
        R"("radius":0.2,"cylinderHeight":1,"maximumSlopeAngleDegrees":45,"maximumStepHeight":0.25,"mass":70)";
    const auto asset = parse(validPhysical);
    ASSERT_TRUE(std::holds_alternative<SetCharacterPhysicalSettingsOperation>(asset.setup[0]));
    ASSERT_TRUE(std::holds_alternative<StartNavigationPathOperation>(asset.start[0]));
    EXPECT_EQ(std::get<StartNavigationPathOperation>(asset.start[0]).target, "target");
    EXPECT_EQ(std::get<StartNavigationPathOperation>(asset.start[0]).result, "path");

    for (const std::string_view invalid : {
        R"("radius":0,"cylinderHeight":1,"maximumSlopeAngleDegrees":45,"maximumStepHeight":0.25,"mass":70)",
        R"("radius":0.2,"cylinderHeight":0,"maximumSlopeAngleDegrees":45,"maximumStepHeight":0.25,"mass":70)",
        R"("radius":0.2,"cylinderHeight":1,"maximumSlopeAngleDegrees":90,"maximumStepHeight":0.25,"mass":70)",
        R"("radius":0.2,"cylinderHeight":1,"maximumSlopeAngleDegrees":45,"maximumStepHeight":1.4,"mass":70)",
        R"("radius":0.2,"cylinderHeight":1,"maximumSlopeAngleDegrees":45,"maximumStepHeight":0.25,"mass":0)"})
    {
        EXPECT_THROW(parse(invalid), std::runtime_error);
    }
    EXPECT_THROW(parse(validPhysical,
        R"("searchExtents":[0,2,1],"acceptanceRadius":0.2,"slowingRadius":0.75)"), std::runtime_error);
    EXPECT_THROW(parse(validPhysical,
        R"("searchExtents":[1,2,1],"acceptanceRadius":-1,"slowingRadius":0.75)"), std::runtime_error);
    EXPECT_THROW(parse(validPhysical,
        R"("searchExtents":[1,2,1],"acceptanceRadius":0.5,"slowingRadius":0.2)"), std::runtime_error);
    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({"id":"duplicate","title":"Duplicate","roles":{"agent":"Agent","target":"Target"},"start":[
      {"op":"startNavigationPath","entity":"agent","target":"target","searchExtents":[1,2,1],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false,"result":"path"},
      {"op":"startNavigationPath","entity":"agent","target":"target","searchExtents":[1,2,1],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false,"result":"path"}]})"), std::runtime_error);

    constexpr std::string_view physicalOperation =
        R"({"op":"setCharacterPhysicalSettings","entity":"agent","radius":0.2,"cylinderHeight":1,"maximumSlopeAngleDegrees":45,"maximumStepHeight":0.25,"mass":70})";
    constexpr std::string_view navigationOperation =
        R"({"op":"startNavigationPath","entity":"agent","target":"target","searchExtents":[1,2,1],"acceptanceRadius":0.2,"slowingRadius":0.75,"wantsRun":false,"result":"path"})";
    for (const std::string_view section : {"start", "update", "stop", "reset"})
    {
        EXPECT_THROW(ParseDevelopmentScenarioAsset(
            std::string(R"({"id":"section","title":"Section","roles":{"agent":"Agent"},")") +
            std::string(section) + R"(":[)" + std::string(physicalOperation) + "]}"), std::runtime_error)
            << section;
    }
    for (const std::string_view section : {"setup", "update", "stop", "reset"})
    {
        EXPECT_THROW(ParseDevelopmentScenarioAsset(
            std::string(R"({"id":"section","title":"Section","roles":{"agent":"Agent","target":"Target"},")") +
            std::string(section) + R"(":[)" + std::string(navigationOperation) + "]}"), std::runtime_error)
            << section;
    }
}

TEST(DevelopmentScenarioAsset, RestrictsDecisionOperationsToOwnedLifecycleSections)
{
    constexpr std::string_view cancel = R"({"op":"cancelAIDecision","entity":"agent"})";
    for (const std::string_view section : {"setup", "update"})
    {
        EXPECT_THROW(ParseDevelopmentScenarioAsset(
            std::string(R"({"id":"decision","title":"Decision","roles":{"agent":"Agent"},")") +
            std::string(section) + R"(":[)" + std::string(cancel) + "]}"), std::runtime_error);
    }
}

TEST(DevelopmentScenarioRunner, AuthoredAccessKeyRunsProductionDecisionAndRestarts)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_goap_access_key_development.level.json");
    const DevelopmentScenarioAsset scenario = LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::test::LevelInstantiateHarness harness{};
    rendern::LevelInstance instance = harness.Instantiate(level);
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{
        rendern::MakeDefaultGameplayAIDecisionFactories()};
    runtime.Initialize(level, instance, scene);
    rendern::GameplayUpdateContext game{.deltaSeconds=1.0f/60.0f,
        .mode=rendern::GameplayRuntimeMode::Game, .levelAsset=&level,
        .levelInstance=&instance, .scene=&scene};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    ScenarioContext context{runtime,level,instance,scene,rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario,context)); ASSERT_TRUE(runner.CanStart(context));
    ASSERT_TRUE(runner.Start(context));
    const rendern::EntityHandle agent=FindRoleEntity(runner,runtime,"agent");
    ASSERT_NE(agent,rendern::kNullEntity);
    const int agentNodeIndex=runner.GetResolvedNodeIndex("agent");
    ASSERT_GE(agentNodeIndex,0);
    const mathUtils::Vec3 baselinePosition=
        level.nodes[static_cast<std::size_t>(agentNodeIndex)].transform.position;
    
    // Production App ordering runs DevelopmentScenarioRuntime::Update before
    // GameplayRuntime::PrePhysicsUpdate. A successful StartAIDecision must
    // therefore already expose an active decision state here.
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),
        rendern::AIPlanExecutionStatus::ReadyToStartStep);
    
    ASSERT_EQ(runner.GetResults().size(),1u);
    EXPECT_EQ(runner.GetResults()[0].status,ScenarioOperationResultStatus::Running);
    runner.Update(context);
    EXPECT_EQ(runner.GetResults()[0].status,ScenarioOperationResultStatus::Running);
    
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),rendern::AIActionExecutionStatus::Running);
    
    const rendern::AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPHasAccessKeyFact));
   
    // Reaching the final destination before the key must not satisfy the goal.
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.08f,10};
    runner.Update(context);
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
    EXPECT_FALSE(facts->IsFactSet(rendern::kGOAPAtDestinationFact));
    for (const std::string_view role : {"coinA", "coinB", "coinC"})
    {
        const int nodeIndex = runner.GetResolvedNodeIndex(role);
        ASSERT_GE(nodeIndex, 0);
        runtime.GetWorld().TryGetTransform(agent)->position =
            level.nodes[static_cast<std::size_t>(nodeIndex)].transform.position;
        runner.Update(context);
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(game);
        runtime.PostPhysicsUpdate(game);
    }
    EXPECT_EQ(facts->GetIntegerFact(rendern::kGOAPCoinCountFact), 3);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    for (int tick = 0; tick < 5 &&
        !facts->IsFactSet(rendern::kGOAPHasAccessKeyFact); ++tick)
    {
        runner.Update(context);
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(game);
        runtime.PostPhysicsUpdate(game);
    }
    EXPECT_TRUE(facts->IsFactSet(rendern::kGOAPHasAccessKeyFact));
    EXPECT_EQ(facts->GetIntegerFact(rendern::kGOAPCoinCountFact), 1);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.08f,10};
    runner.Update(context);
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
    EXPECT_TRUE(facts->IsFactSet(rendern::kGOAPAtDestinationFact));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),rendern::AIPlanExecutionStatus::Succeeded);
    
    // Project the production terminal state on the next scenario update,
    // matching the real App frame order.
    runner.Update(context);
    EXPECT_EQ(runner.GetResults()[0].status,ScenarioOperationResultStatus::Succeeded);
    
    runner.Reset(context);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),rendern::AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runner.GetResults()[0].status,ScenarioOperationResultStatus::NotStarted);
    ASSERT_NE(runtime.GetWorld().TryGetTransform(agent),nullptr);
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(agent)->position,baselinePosition);
    
    ASSERT_TRUE(runner.Start(context));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),rendern::AIPlanExecutionStatus::ReadyToStartStep);
    
    runner.Update(context);
    EXPECT_EQ(runner.GetResults()[0].status,ScenarioOperationResultStatus::Running);
    
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, UnknownDecisionDefinitionDisablesStart)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{}; AddMoveToNode(level,"Agent",{});
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    
    ScenarioContext context{runtime,level,instance,scene,rendern::GameplayRuntimeMode::Game};
    const DevelopmentScenarioAsset scenario=ParseDevelopmentScenarioAsset(R"({
      "id":"unknown","title":"Unknown","roles":{"agent":"Agent"},
      "setup":[{"op":"ensureNodeBoundEntity","entity":"agent"},{"op":"ensureAI","entity":"agent"}],
      "start":[{"op":"startAIDecision","entity":"agent","decision":"missing","result":"decision"}]})");
    DevelopmentScenarioRunner runner{}; ASSERT_TRUE(runner.Load(scenario,context));
    EXPECT_FALSE(runner.CanStart(context)); EXPECT_FALSE(runner.Start(context));
    runner.Unload(context); runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, NavigationAgentSizeUsesProfilesAndTreatsNoPathAsDomainResult)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/navigation_small_large_passage.level.json");
    const DevelopmentScenarioAsset scenario = LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::test::LevelInstantiateHarness harness{};
    harness.SetMeshCPU(MakeCubeMesh());
    rendern::LevelInstance instance = harness.Instantiate(level);
    harness.DrainAssetPipeline();
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);

    const app::navigationRuntime::GeometryResult geometry =
        app::navigationRuntime::BuildLevelNavigationGeometry(instance);
    ASSERT_EQ(geometry.status, app::navigationRuntime::GeometryStatus::Ready);
    EXPECT_GT(geometry.sourceMeshCount, 0u);
    navigation::ProfileRegistry profiles{};
    ASSERT_EQ(profiles.Initialize(geometry.geometry, navigation::BuildSettings{}).status,
        navigation::BuildStatus::Succeeded);
    ScenarioContext context{runtime, level, instance, scene,
        rendern::GameplayRuntimeMode::Game, nullptr, &profiles};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    ASSERT_TRUE(runner.CanStart(context));
    ASSERT_TRUE(runner.Start(context));

    const auto small = FindRoleEntity(runner, runtime, "smallAgent");
    const auto large = FindRoleEntity(runner, runtime, "largeAgent");
    ASSERT_NE(small, rendern::kNullEntity);
    ASSERT_NE(large, rendern::kNullEntity);
    ASSERT_EQ(runner.GetResults().size(), 2u);
    EXPECT_EQ(runner.GetResults()[0].status, ScenarioOperationResultStatus::Running);
    EXPECT_EQ(runner.GetResults()[1].status, ScenarioOperationResultStatus::NoPath);
    EXPECT_EQ(runtime.GetAIActionStatus(small), rendern::AIActionExecutionStatus::Running);
    EXPECT_EQ(runtime.GetAIActionStatus(large), rendern::AIActionExecutionStatus::NotStarted);
    EXPECT_GE(profiles.GetProfileCount(), 2u);

    runner.Stop(context);
    EXPECT_EQ(runtime.GetAIActionStatus(small), rendern::AIActionExecutionStatus::Cancelled);
    runner.Reset(context);
    EXPECT_EQ(runner.GetResults()[0].status, ScenarioOperationResultStatus::NotStarted);
    EXPECT_EQ(runner.GetResults()[1].status, ScenarioOperationResultStatus::NotStarted);
    ASSERT_TRUE(runner.Start(context));
    EXPECT_EQ(runner.GetResults()[0].status, ScenarioOperationResultStatus::Running);
    EXPECT_EQ(runner.GetResults()[1].status, ScenarioOperationResultStatus::NoPath);
    runner.Unload(context);
    EXPECT_TRUE(runner.GetResults().empty());
    ASSERT_TRUE(runner.Load(scenario, context));
    EXPECT_EQ(runner.GetResults()[0].status, ScenarioOperationResultStatus::NotStarted);
    runner.Unload(context);

    DevelopmentScenarioAsset rollbackScenario = scenario;
    rollbackScenario.start.push_back(EnsureAIOperation{"smallTarget"});
    ASSERT_TRUE(runner.Load(rollbackScenario, context));
    EXPECT_FALSE(runner.Start(context));
    ASSERT_EQ(runner.GetResults().size(), 2u);
    EXPECT_EQ(runner.GetResults()[0].status, ScenarioOperationResultStatus::Cancelled);
    EXPECT_EQ(runner.GetResults()[1].status, ScenarioOperationResultStatus::NoPath);
    const auto rollbackSmall = FindRoleEntity(runner, runtime, "smallAgent");
    ASSERT_NE(rollbackSmall, rendern::kNullEntity);
    EXPECT_EQ(runtime.GetAIActionStatus(rollbackSmall),
        rendern::AIActionExecutionStatus::Cancelled);
    runner.Unload(context);
    runtime.Shutdown();
}

TEST(DevelopmentScenarioAsset, RejectsInvalidPickupCollectionRadius)
{
    EXPECT_THROW(ParseDevelopmentScenarioAsset(R"({
        "id":"pickup.invalid", "title":"Invalid pickup",
        "roles":{"coin":"Coin"},
        "setup":[{"op":"ensurePickup","entity":"coin","collectionRadius":-10}]
    })"), std::runtime_error);

    DevelopmentScenarioAsset nonFinite{
        .id="pickup.nonfinite", .title="Non-finite pickup", .roles={{"coin","Coin"}},
        .setup={EnsurePickupOperation{"coin",std::numeric_limits<float>::infinity()}}};
    EXPECT_THROW(ValidateDevelopmentScenarioAsset(nonFinite), std::runtime_error);
}

TEST(DevelopmentScenarioAsset, EnsureInteractionPointParsesOnlyInSetup)
{
    const DevelopmentScenarioAsset asset = ParseDevelopmentScenarioAsset(R"({
        "id":"interaction.setup", "title":"Interaction setup",
        "roles":{"coin":"Coin"},
        "setup":[{"op":"ensureInteractionPoint","entity":"coin"}]
    })");
    ASSERT_EQ(asset.setup.size(), 1u);
    EXPECT_TRUE(std::holds_alternative<EnsureInteractionPointOperation>(asset.setup.front()));

    for (const char* lifecycle : {"start", "update", "stop", "reset"})
    {
        const std::string json = std::string{"{\"id\":\"interaction.invalid\","
            "\"title\":\"Invalid interaction\",\"roles\":{\"coin\":\"Coin\"},\""} +
            lifecycle + "\":[{\"op\":\"ensureInteractionPoint\",\"entity\":\"coin\"}]}";
        EXPECT_THROW(ParseDevelopmentScenarioAsset(json), std::runtime_error) << lifecycle;
    }
}

TEST(DevelopmentScenarioRunner, EnsureInteractionPointAddsIdempotentlyAndRestoresOwnership)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    rendern::LevelNode node{}; node.name="Coin"; node.alive=true; level.nodes.push_back(node);
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    rendern::GameplayUpdateContext gameplayContext{.mode=rendern::GameplayRuntimeMode::Editor,
        .levelAsset=&level,.levelInstance=&instance,.scene=&scene};
    const rendern::EntityHandle coin=runtime.SpawnNodeBoundEntity(gameplayContext,0,false);
    ASSERT_NE(coin,rendern::kNullEntity);
    ScenarioContext context{runtime,level,instance,scene,rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioAsset asset{.id="interaction.ownership",.title="Interaction ownership",
        .roles={{"coin","Coin"}},.setup={EnsureInteractionPointOperation{"coin"},
            EnsureInteractionPointOperation{"coin"}}};

    runtime.GetWorld().AddInteractionPoint(coin,
        {.localPosition={1.0f,2.0f,3.0f},.localFacingYawDegrees=25.0f});
    DevelopmentScenarioRunner restoreExisting{};
    ASSERT_TRUE(restoreExisting.Load(asset,context));
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetInteractionPoint(coin)->localPosition.x,0.0f);
    restoreExisting.Reset(context);
    EXPECT_TRUE(runtime.GetWorld().HasInteractionPoint(coin));
    restoreExisting.Unload(context);
    ASSERT_NE(runtime.GetWorld().TryGetInteractionPoint(coin),nullptr);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetInteractionPoint(coin)->localPosition.z,3.0f);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetInteractionPoint(coin)->localFacingYawDegrees,25.0f);

    runtime.GetWorld().RemoveInteractionPoint(coin);
    DevelopmentScenarioRunner removeAdded{};
    ASSERT_TRUE(removeAdded.Load(asset,context));
    EXPECT_TRUE(runtime.GetWorld().HasInteractionPoint(coin));
    removeAdded.Unload(context);
    EXPECT_FALSE(runtime.GetWorld().HasInteractionPoint(coin));
    runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, EnsurePickupRestoresPreExistingComponentOwnership)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level{};
    rendern::LevelNode node{}; node.name="Coin"; node.alive=true; level.nodes.push_back(node);
    rendern::LevelInstance instance{}; rendern::Scene scene{}; rendern::GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    rendern::GameplayUpdateContext gameplayContext{.mode=rendern::GameplayRuntimeMode::Editor,
        .levelAsset=&level,.levelInstance=&instance,.scene=&scene};
    const rendern::EntityHandle coin=runtime.SpawnNodeBoundEntity(gameplayContext,0,false);
    ASSERT_NE(coin,rendern::kNullEntity);
    ScenarioContext context{runtime,level,instance,scene,rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioAsset asset{.id="pickup.ownership",.title="Pickup ownership",
        .roles={{"coin","Coin"}},.setup={EnsurePickupOperation{"coin",0.6f}}};

    runtime.GetWorld().AddPickup(coin,{.collectionRadius=2.5f,.collected=true});
    DevelopmentScenarioRunner restoreExisting{};
    ASSERT_TRUE(restoreExisting.Load(asset,context));
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetPickup(coin)->collectionRadius,0.6f);
    restoreExisting.Unload(context);
    ASSERT_NE(runtime.GetWorld().TryGetPickup(coin),nullptr);
    EXPECT_FLOAT_EQ(runtime.GetWorld().TryGetPickup(coin)->collectionRadius,2.5f);
    EXPECT_TRUE(runtime.GetWorld().TryGetPickup(coin)->collected);

    runtime.GetWorld().RemovePickup(coin);
    DevelopmentScenarioRunner removeAdded{};
    ASSERT_TRUE(removeAdded.Load(asset,context));
    EXPECT_TRUE(runtime.GetWorld().HasPickup(coin));
    removeAdded.Unload(context);
    EXPECT_FALSE(runtime.GetWorld().HasPickup(coin));
    runtime.Shutdown();
}

TEST(DevelopmentScenarioParser, SteeringPlaygroundDeclaresProductionTargetAndRouteOperations)
{
    const DevelopmentScenarioAsset asset = LoadDevelopmentScenarioAsset(
        "development/ai_steering_playground.scenario.json");
    EXPECT_EQ(asset.id, "core.ai_steering_playground");
    EXPECT_EQ(asset.roles.size(), 7u);
    ASSERT_EQ(asset.start.size(), 3u);
    EXPECT_TRUE(std::holds_alternative<StartFollowTargetOperation>(asset.start[0]));
    EXPECT_TRUE(std::holds_alternative<StartFleeTargetOperation>(asset.start[1]));
    EXPECT_TRUE(std::holds_alternative<StartMoveToOperation>(asset.start[2]));
}

TEST(DevelopmentScenarioRunner, SteeringPlaygroundLoadsAndStartsProductionActions)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_steering_playground.level.json");
    const DevelopmentScenarioAsset scenario =
        LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::test::LevelInstantiateHarness harness{};
    harness.SetMeshCPU(MakeCubeMesh());
    rendern::LevelInstance instance = harness.Instantiate(level);
    harness.DrainAssetPipeline();
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    runtime.SetSteeringDebugEnabled(true);
    ScenarioContext context{runtime, level, instance, scene,
        rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    ASSERT_TRUE(runner.Start(context));

    const auto follow = FindRoleEntity(runner, runtime, "followAgent");
    const auto flee = FindRoleEntity(runner, runtime, "fleeAgent");
    const auto route = FindRoleEntity(runner, runtime, "routeAgent");
    ASSERT_NE(follow, rendern::kNullEntity);
    ASSERT_NE(flee, rendern::kNullEntity);
    ASSERT_NE(route, rendern::kNullEntity);
    EXPECT_EQ(runtime.GetAIActionStatus(follow), rendern::AIActionExecutionStatus::Running);
    EXPECT_EQ(runtime.GetAIActionStatus(flee), rendern::AIActionExecutionStatus::Running);
    EXPECT_EQ(runtime.GetAIActionStatus(route), rendern::AIActionExecutionStatus::Running);
    rendern::GameplayUpdateContext gameContext{
        .deltaSeconds = 1.0f / 60.0f,
        .mode = rendern::GameplayRuntimeMode::Game,
        .levelAsset = &level,
        .levelInstance = &instance,
        .scene = &scene};
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(gameContext);
    runtime.PostPhysicsUpdate(gameContext);
    ASSERT_NE(runtime.GetSteeringDebugRegistry().Find(follow), nullptr);
    ASSERT_NE(runtime.GetSteeringDebugRegistry().Find(flee), nullptr);
    ASSERT_NE(runtime.GetSteeringDebugRegistry().Find(route), nullptr);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(follow)->mode,
        rendern::GameplaySteeringDebugMode::Follow);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(flee)->mode,
        rendern::GameplaySteeringDebugMode::Flee);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(route)->mode,
        rendern::GameplaySteeringDebugMode::Route);

    runner.Unload(context);
    runtime.Shutdown();
}

TEST(DevelopmentScenarioRunner, SteeringPlaygroundResetRestoresScenarioState)
{
    InlineThreadOwnerRolesGuard guard{};
    rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/ai_steering_playground.level.json");
    const DevelopmentScenarioAsset scenario =
        LoadDevelopmentScenarioAsset(level.developmentScenario);
    rendern::test::LevelInstantiateHarness harness{};
    harness.SetMeshCPU(MakeCubeMesh());
    rendern::LevelInstance instance = harness.Instantiate(level);
    harness.DrainAssetPipeline();
    rendern::Scene& scene = harness.GetScene();
    rendern::GameplayRuntime runtime{};
    runtime.Initialize(level, instance, scene);
    runtime.SetSteeringDebugEnabled(true);
    ScenarioContext context{runtime, level, instance, scene,
        rendern::GameplayRuntimeMode::Game};
    DevelopmentScenarioRunner runner{};
    ASSERT_TRUE(runner.Load(scenario, context));
    ASSERT_TRUE(runner.Start(context));
    const auto follow = FindRoleEntity(runner, runtime, "followAgent");
    const auto flee = FindRoleEntity(runner, runtime, "fleeAgent");
    const auto route = FindRoleEntity(runner, runtime, "routeAgent");
    ASSERT_NE(follow, rendern::kNullEntity);
    ASSERT_NE(flee, rendern::kNullEntity);
    ASSERT_NE(route, rendern::kNullEntity);
    const mathUtils::Vec3 baseline = runtime.GetWorld().TryGetTransform(follow)->position;
    rendern::GameplayTransformComponent moved = *runtime.GetWorld().TryGetTransform(follow);
    moved.position.x += 10.0f;
    runtime.GetWorld().SetTransform(follow, moved);

    runner.Reset(context);
    EXPECT_FALSE(runner.IsRunning());
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(follow)->position, baseline);
    EXPECT_EQ(runtime.GetAIActionStatus(follow), rendern::AIActionExecutionStatus::Cancelled);
    EXPECT_EQ(runtime.GetAIActionStatus(flee), rendern::AIActionExecutionStatus::Cancelled);
    EXPECT_EQ(runtime.GetAIActionStatus(route), rendern::AIActionExecutionStatus::Cancelled);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(follow), nullptr);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(flee), nullptr);
    EXPECT_EQ(runtime.GetSteeringDebugRegistry().Find(route), nullptr);

    runner.Unload(context);
    runtime.Shutdown();
}