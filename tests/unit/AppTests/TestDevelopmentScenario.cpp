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