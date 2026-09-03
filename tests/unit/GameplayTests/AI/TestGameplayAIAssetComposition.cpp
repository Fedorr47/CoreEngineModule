#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <any>

#include "TestSupport/TestTempPath.h"

import core;

using namespace rendern;

namespace
{
    class GameplayAIAssetComposition : public ::testing::Test
    {
    protected:
        GameplayAIAssetComposition()
        {
            LevelNode origin{};
            origin.name = "GOAP_Recovery_Start";
            LevelNode destination{};
            destination.name = "GOAP_Recovery_Goal";
            destination.transform.position = {10.0f, 0.0f, 0.0f};
            level.nodes = {origin, destination};
            agent = world.CreateEntity();
            world.AddTransform(agent, {});
            world.AddAI(agent);
            world.AddCharacterCommand(agent, {});
            world.AddCharacterMotor(agent, {});
            world.AddCharacterMovementState(agent, {});
            target = world.CreateEntity();
            world.AddNodeLink(target, {.nodeIndex = 1});
            world.AddInteractionPoint(target, {});
        }

        GameplayAIDecisionCreationContext Services()
        {
            return {agent, level, world, links, executors, reservations, &diagnostic};
        }

        std::unique_ptr<GameplayAIDecisionInstance> Create()
        {
            return CreateGameplayGOAPDecisionFromAssets(Services(), behavior, bindings, definition, components);
        }

        void ExpectFailure(std::string_view part)
        {
            try
            {
                auto decision = Create();
                FAIL() << "Invalid composition created a decision";
            }
            catch (const std::runtime_error& error)
            {
                EXPECT_NE(std::string_view(error.what()).find(part), std::string_view::npos) << error.what();
            }
            EXPECT_FALSE(ai.HasActiveAction(agent));
        }

        GameplayWorld world;
        LevelAsset level;
        GameplayTraversalLinkRegistry links;
        GameplayTraversalExecutorRegistry executors;
        GameplayObjectReservationSystem reservations;
        AISystem ai;
        EntityHandle agent{kNullEntity};
        EntityHandle target{kNullEntity};
        std::string diagnostic;
        GameplayGOAPCompositionRegistry components{MakeDefaultGameplayGOAPComponents()};
        GameplayAIBehaviorAsset behavior{LoadGameplayAIBehaviorAsset("ai/behaviors/target_recovery.behavior.json", components.AssetParsers())};
        GameplayAILevelBindingsAsset bindings{LoadGameplayAILevelBindingsAsset("ai/bindings/target_recovery.bindings.json")};
        GameplayGOAPDefinitionAsset definition{LoadGameplayGOAPDefinitionAsset(behavior.definition)};
    };
}

TEST_F(GameplayAIAssetComposition, RecoveryCancelsActiveMovementAndReplansWhenTargetReturns)
{
    auto decision = Create();
    ASSERT_NE(decision, nullptr);
    EXPECT_FALSE(ai.HasActiveAction(agent));
    decision->Update(ai, {world, {}}); // Plan.
    decision->Update(ai, {world, {}}); // Start the ready action.
    ASSERT_TRUE(ai.HasActiveAction(agent));
    world.RemoveInteractionPoint(target);
    decision->Update(ai, {world, {}});
    EXPECT_FALSE(ai.HasActiveAction(agent));
    world.AddInteractionPoint(target, {});
    decision->Update(ai, {world, {}});
    decision->Update(ai, {world, {}});
    EXPECT_TRUE(ai.HasActiveAction(agent));
    decision->Cancel(ai);
    EXPECT_FALSE(ai.HasActiveAction(agent));
}

TEST_F(GameplayAIAssetComposition, ProvidersOutliveTemporaryAssetsAndRegistry)
{
    auto decision = Create();
    ASSERT_NE(decision, nullptr);
    behavior = {};
    bindings = {};
    definition = {};
    components = {};
    decision->Update(ai, {world, {}});
    decision->Update(ai, {world, {}});
    EXPECT_TRUE(ai.HasActiveAction(agent));
    decision->Cancel(ai);
}

TEST_F(GameplayAIAssetComposition, RejectsUnknownComponentsAndModel)
{
    behavior.model = "unknown_model";
    ExpectFailure("unknown decision model");
    behavior.model = "goap";
    behavior.observations.front().type = "unknown_sensor";
    ExpectFailure("unknown observation type 'unknown_sensor'");
    behavior.observations.front().type = "target_available";
    behavior.capabilities.front().type = "unknown_capability";
    ExpectFailure("unknown capability 'unknown_capability'");
}

TEST_F(GameplayAIAssetComposition, RejectsMissingAmbiguousAndNonFiniteLevelReferences)
{
    bindings.roles.front().node = "missing";
    ExpectFailure("missing node 'missing'");
    bindings.roles.front().node = "GOAP_Recovery_Start";
    level.nodes.push_back(level.nodes.front());
    ExpectFailure("ambiguous node");
    level.nodes.pop_back();
    level.nodes.front().transform.position.x = std::numeric_limits<float>::infinity();
    ExpectFailure("position must be finite");
    level.nodes.front().transform.position.x = 0.0f;
    bindings.roles.front().role = "wrong_role";
    ExpectFailure("no level binding");
}

TEST_F(GameplayAIAssetComposition, RejectsAmbiguousEntitiesAndMissingInteractionPoint)
{
    const auto duplicate = world.CreateEntity();
    world.AddNodeLink(duplicate, {.nodeIndex = 1});
    ExpectFailure("multiple gameplay entities");
    world.RemoveNodeLink(duplicate);
    world.RemoveInteractionPoint(target);
    ExpectFailure("has no interaction point");
    world.RemoveNodeLink(target);
    ExpectFailure("has no gameplay entity");
}

TEST_F(GameplayAIAssetComposition, RequiresOneWriterPerFactAndOneBindingPerContext)
{
    behavior.observations.push_back(behavior.observations.front());
    ExpectFailure("multiple observation writers");
    behavior.observations.pop_back();
    const auto observation = behavior.observations.back();
    behavior.observations.pop_back();
    ExpectFailure("fact has no observation writer");
    behavior.observations.push_back(observation);
    std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.front().parameters).fact = "missing";
    ExpectFailure("unknown fact");
    std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.front().parameters).fact = "goalAvailable";
    behavior.capabilities.push_back(behavior.capabilities.front());
    ExpectFailure("multiple capability bindings");
    behavior.capabilities.pop_back();
    behavior.capabilities.front().context = "wrong_context";
    ExpectFailure("does not match");
    behavior.capabilities.clear();
    ExpectFailure("has no capability binding");
}

TEST_F(GameplayAIAssetComposition, RejectsInvalidSpatialParameters)
{
    std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.back().parameters).radius = 0.0f;
    ExpectFailure("radius must be positive");
    std::any_cast<GameplayAISpatialObservationAsset&>(behavior.observations.back().parameters).radius = 0.4f;
    std::any_cast<GameplayAIMoveToAsset&>(behavior.capabilities.front().parameters).slowingRadius = 0.1f;
    ExpectFailure("slowingRadius >= acceptanceRadius");
}

TEST_F(GameplayAIAssetComposition, MultipleContextsShareOneSemanticBinding)
{
    auto secondAction = definition.actions.front();
    secondAction.context = "alternative";
    secondAction.cost = 2.0f;
    definition.actions.push_back(secondAction);
    auto secondCapability = behavior.capabilities.front();
    secondCapability.context = "alternative";
    std::any_cast<GameplayAIMoveToAsset&>(secondCapability.parameters).acceptanceRadius = 0.8f;
    behavior.capabilities.push_back(secondCapability);
    auto decision = Create();
    ASSERT_NE(decision, nullptr); // Duplicate action bindings would reject configuration.
    decision->Update(ai, {world, {}});
    decision->Update(ai, {world, {}});
    EXPECT_TRUE(ai.HasActiveAction(agent));
    decision->Cancel(ai);
}

TEST_F(GameplayAIAssetComposition, CatalogRegistrationIsTransactionalAndOwnsCompilerCallbacks)
{
    GameplayAIDecisionFactoryRegistry registry;
    GameplayAIDecisionCatalogAsset catalog{
        .source = "duplicate-catalog.json",
        .decisions = {{"same", "a", "b"}, {"same", "c", "d"}}};
    EXPECT_THROW(RegisterGameplayAIDecisionAssets(registry, catalog, components), std::runtime_error);
    EXPECT_FALSE(registry.Contains("same"));
    RegisterGameplayAIDecisionAssets(registry,
        LoadGameplayAIDecisionCatalogAsset("ai/decisions/catalog.json"), components);
    components = {};
    auto decision = registry.Create("target_recovery", Services());
    ASSERT_NE(decision, nullptr) << diagnostic;
    EXPECT_TRUE(diagnostic.empty());
    decision->Update(ai, {world, {}});
    decision->Update(ai, {world, {}});
    EXPECT_TRUE(ai.HasActiveAction(agent));
    decision->Cancel(ai);
    EXPECT_EQ(registry.Create("unknown", Services()), nullptr);
    EXPECT_NE(diagnostic.find("Unknown AI decision 'unknown'"), std::string::npos);
}

TEST_F(GameplayAIAssetComposition, CatalogFailureReportsSourceAndStartsNoTask)
{
    auto registry = MakeDefaultGameplayAIDecisionFactories();
    level.nodes.front().name = "renamed";
    auto decision = registry.Create("target_recovery", Services());
    EXPECT_EQ(decision, nullptr);
    EXPECT_FALSE(ai.HasActiveAction(agent));
    EXPECT_NE(diagnostic.find("target_recovery.bindings.json"), std::string::npos);
    EXPECT_NE(diagnostic.find("GOAP_Recovery_Start"), std::string::npos);
}

TEST_F(GameplayAIAssetComposition, ThirdScenarioUsesItsOwnAssetsAndParameters)
{
    level = LoadLevelAssetFromJson("levels/ai_goap_marker_visit_development.level.json");
    const auto goal = std::ranges::find(level.nodes, "GOAP_Marker_Goal", &LevelNode::name);
    ASSERT_NE(goal, level.nodes.end());
    world.RemoveNodeLink(target);
    world.AddNodeLink(target, {.nodeIndex = static_cast<int>(goal - level.nodes.begin())});
    // .6 is inside marker_visit's .8 arrival radius and outside recovery's .4.
    world.TryGetTransform(agent)->position = goal->transform.position + mathUtils::Vec3{0.6f, 0.0f, 0.0f};
    auto registry = MakeDefaultGameplayAIDecisionFactories();
    auto decision = registry.Create("marker_visit", Services());
    ASSERT_NE(decision, nullptr) << diagnostic;
    EXPECT_NE(dynamic_cast<GameplayGOAPDecisionInstance*>(decision.get()), nullptr);
    decision->Update(ai, {world, {}});
    EXPECT_EQ(decision->GetStatus(), GameplayAIDecisionStatus::Succeeded);
    EXPECT_FALSE(ai.HasActiveAction(agent));
    decision->Cancel(ai);
}

TEST(GameplayAIDecisionAsset, RejectsUnknownFieldsWrongTypesAndDuplicateKeys)
{
    const auto components = MakeDefaultGameplayGOAPComponents();
    EXPECT_THROW(ParseGameplayAIDecisionCatalogAsset(
        R"({"version":2,"decisions":[]})", "catalog.json"), std::runtime_error);
    EXPECT_THROW(ParseGameplayAILevelBindingsAsset(
        R"({"version":1,"roles":[{"role":"goal","node":"a"},{"role":"goal","node":"b"}]})",
        "bindings.json"), std::runtime_error);
    EXPECT_THROW(ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a.json","observations":[],"capabilities":[],"typo":true})",
        "behavior.json", components.AssetParsers()), std::runtime_error);
    EXPECT_THROW(ParseGameplayAIBehaviorAsset(
        R"({"version":1,"id":"a","model":"goap","definition":"a.json","observations":[{"type":"within_distance","target":"goal","fact":"atGoal","radius":"far"}],"capabilities":[]})",
        "behavior.json", components.AssetParsers()), std::runtime_error);
}

TEST_F(GameplayAIAssetComposition, KeepsPreparedSnapshotUntilExplicitRegistryRebuild)
{
    ::test::ScopedTempPath temporary{::test::MakeUniqueTempPath("core-goap-assets")};
    ASSERT_TRUE(std::filesystem::create_directory(temporary.Path()));
    const auto path = temporary.Path() / "behavior.json";
    int parses = 0;
    const auto availableCompiler = *components.Observation("target_available");
    ASSERT_TRUE(components.RegisterObservation<GameplayAISpatialObservationAsset>("counted_available",
        [&](const GameplayAIComponentParseContext& input)
        {
            ++parses;
            return ParseGameplayAISpatialObservation(input);
        }, availableCompiler));
    auto original = FILE_UTILS::ReadAllText(
        corefs::ResolveAsset("ai/behaviors/target_recovery.behavior.json"));
    const auto sensor = original.find("target_available");
    ASSERT_NE(sensor, std::string::npos);
    original.replace(sensor, std::string_view("target_available").size(), "counted_available");
    const auto write = [&](std::string_view json)
    {
        std::ofstream output(path);
        output << json;
        ASSERT_TRUE(output.good());
    };
    write(original);
    GameplayAIDecisionFactoryRegistry registry;
    RegisterGameplayAIDecisionAssets(registry,
        {.source = "test-catalog", .decisions = {{"editable", path.string(),
            "ai/bindings/target_recovery.bindings.json"}}}, components);
    EXPECT_EQ(parses, 1);
    auto first = registry.Create("editable", Services());
    ASSERT_NE(first, nullptr) << diagnostic;
    auto changed = original;
    const auto type = changed.find("counted_available");
    ASSERT_NE(type, std::string::npos);
    changed.replace(type, std::string_view("counted_available").size(), "unknown_sensor");
    write(changed);
    // New instances of this registry keep using the same prepared snapshot.
    auto second = registry.Create("editable", Services());
    ASSERT_NE(second, nullptr) << diagnostic;
    EXPECT_EQ(parses, 1);
    GameplayAIDecisionFactoryRegistry replacement;
    EXPECT_THROW(RegisterGameplayAIDecisionAssets(replacement,
        {.source = "test-catalog", .decisions = {{"editable", path.string(),
            "ai/bindings/target_recovery.bindings.json"}}}, components), std::runtime_error);
    EXPECT_FALSE(replacement.Contains("editable"));
    // No hot reload: an existing instance keeps its validated providers.
    first->Update(ai, {world, {}});
    first->Update(ai, {world, {}});
    EXPECT_TRUE(ai.HasActiveAction(agent));
    first->Cancel(ai);
    // Explicit rebuild reads a valid edit; old registry copies retain the old radius.
    auto wider = original;
    for (std::size_t at = wider.find("0.4"); at != std::string::npos; at = wider.find("0.4", at + 3))
    {
        wider.replace(at, 3, "0.8");
    }
    write(wider);
    RegisterGameplayAIDecisionAssets(replacement,
        {.source = "test-catalog", .decisions = {{"editable", path.string(),
            "ai/bindings/target_recovery.bindings.json"}}}, components);
    ASSERT_TRUE(std::filesystem::remove(path));
    auto copiedRegistry = registry;
    registry = {};
    auto fresh = replacement.Create("editable", Services());
    auto old = copiedRegistry.Create("editable", Services());
    ASSERT_NE(fresh, nullptr) << diagnostic;
    ASSERT_NE(old, nullptr) << diagnostic;
    EXPECT_EQ(parses, 2);
    world.TryGetTransform(agent)->position = level.nodes.back().transform.position + mathUtils::Vec3{0.6f, 0, 0};
    fresh->Update(ai, {world, {}});
    old->Update(ai, {world, {}});
    EXPECT_EQ(fresh->GetStatus(), GameplayAIDecisionStatus::Succeeded);
    EXPECT_EQ(old->GetStatus(), GameplayAIDecisionStatus::Running);
    fresh->Cancel(ai);
    old->Cancel(ai);
    EXPECT_TRUE(diagnostic.empty());
}

TEST_F(GameplayAIAssetComposition, ComponentRegistrationRejectsDuplicatesAndInvalidActionIds)
{
    const auto compile = [](std::span<const GameplayAICapabilityAsset>,
        const GameplayGOAPCompositionContext&) -> std::unique_ptr<IGameplayGOAPCapability>
    {
        return nullptr;
    };
    EXPECT_FALSE(components.RegisterCapability<GameplayAIMoveToAsset>("other", kAIMoveToActionId, ParseGameplayAIMoveTo, compile));
    EXPECT_FALSE(components.RegisterCapability<GameplayAIMoveToAsset>("move_to", AIActionId{99u}, ParseGameplayAIMoveTo, compile));
    EXPECT_FALSE(components.RegisterCapability<GameplayAIMoveToAsset>("invalid", AIActionId{}, ParseGameplayAIMoveTo, compile));
    EXPECT_FALSE(components.RegisterObservation<GameplayAISpatialObservationAsset>("empty", ParseGameplayAISpatialObservation, {}));
    auto decision = Create();
    ASSERT_NE(decision, nullptr); // Failed registrations preserved the original compiler.
}

TEST_F(GameplayAIAssetComposition, FailedPreparationPublishesNoneOfTheNewFactories)
{
    GameplayAIDecisionFactoryRegistry registry;
    ASSERT_TRUE(registry.Register("existing", [](const GameplayAIDecisionCreationContext&)
        -> std::unique_ptr<GameplayAIDecisionInstance> { return nullptr; }));
    const GameplayAIDecisionCatalogAsset catalog{.source = "atomic-catalog", .decisions = {
        {"valid", "ai/behaviors/target_recovery.behavior.json", "ai/bindings/target_recovery.bindings.json"},
        {"invalid", "ai/behaviors/missing-for-test.json", "ai/bindings/target_recovery.bindings.json"}}};
    EXPECT_THROW(RegisterGameplayAIDecisionAssets(registry, catalog, components), std::runtime_error);
    EXPECT_TRUE(registry.Contains("existing"));
    EXPECT_FALSE(registry.Contains("valid"));
    EXPECT_FALSE(registry.Contains("invalid"));
    EXPECT_FALSE(ai.HasActiveAction(agent));
}
