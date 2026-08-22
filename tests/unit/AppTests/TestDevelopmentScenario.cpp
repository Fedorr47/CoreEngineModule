#include <gtest/gtest.h>

import core;

#include "App/Development/DevelopmentScenario.h"
#include "App/Development/AppDevelopmentScenarioRuntime.h"
#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

using namespace appDevelopment;

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