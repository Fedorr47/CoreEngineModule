#include <gtest/gtest.h>
#include <utility>

#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

import core;

using namespace rendern;

namespace
{
    void AddNode(LevelAsset& level, const char* name, const mathUtils::Vec3 position,
        const bool skinned=false)
    {
        LevelNode node{}; node.name=name; node.alive=true; node.visible=true;
        node.transform.position=position;
        if (skinned)
        {
            node.skinnedMesh="models_Character_fbx";
            node.animationController="fsm_test_locomotion_action";
            node.animationProfile="human";
        }
        level.nodes.push_back(std::move(node));
    }

    LevelAsset MakeLevel()
    {
        LevelAsset level{};
        AddNode(level,"GOAP_Observer_Player",{-5,0,-5},true);
        AddNode(level,"GOAP_Agent",{0,0,0},true);
        AddNode(level,"GOAP_Start",{0,0,0});
        AddNode(level,"GOAP_Access_Key",{0,0,-7});
        AddNode(level,"GOAP_Final_Goal",{0,0,10});
        return level;
    }

    GameplayUpdateContext Context(LevelAsset& level, LevelInstance& instance, Scene& scene,
        GameplayRuntimeMode mode)
    {
        return {.deltaSeconds=1.0f/60.0f,.mode=mode,.levelAsset=&level,
            .levelInstance=&instance,.scene=&scene};
    }

    void Frame(GameplayRuntime& runtime, const GameplayUpdateContext& context,
        GameplayAIGOAPAccessKeyDevelopmentScenario& scenario)
    {
        runtime.BeginFrame(); runtime.PrePhysicsUpdate(context); runtime.PostPhysicsUpdate(context);
        scenario.Update(runtime);
    }
}

TEST(GameplayAIGOAPAccessKeyDevelopmentScenario, UsesControlledSkinnedPlayerAndProductionMovementContract)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level=MakeLevel(); LevelInstance instance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    auto editor=Context(level,instance,scene,GameplayRuntimeMode::Editor);
    const EntityHandle player=runtime.SpawnNodeBoundEntity(editor,0,true);
    ASSERT_NE(player,kNullEntity);
    runtime.GetWorld().AddAnimationLink(player,{.skinnedDrawIndex=0,
        .controllerAssetId="fsm_test_locomotion_action"});
    auto game=Context(level,instance,scene,GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);

    GameplayAIGOAPAccessKeyDevelopmentScenario scenario{};
    ASSERT_TRUE(scenario.Prepare(runtime,game));
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPAtDestinationFact));
    ASSERT_TRUE(scenario.Start(runtime,game));
    const EntityHandle agent=scenario.GetAgentEntity();
    EXPECT_EQ(scenario.GetPlayerEntity(),runtime.GetControlledEntity());
    EXPECT_EQ(scenario.GetPlayerEntity(),player);
    EXPECT_NE(player,agent);
    const GameplayWorld& world=runtime.GetWorld();
    EXPECT_TRUE(world.HasPlayerControlled(player)); EXPECT_NE(world.TryGetAnimationLink(player),nullptr);
    EXPECT_TRUE(world.HasAI(agent)); EXPECT_TRUE(world.HasTransform(agent));
    EXPECT_TRUE(world.HasCharacterCommand(agent)); EXPECT_TRUE(world.HasCharacterMotor(agent));
    EXPECT_TRUE(world.HasCharacterMovementState(agent));
}

TEST(GameplayAIGOAPAccessKeyDevelopmentScenario, AuthoredLevelBootstrapsObserverAsControlledPlayer)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);

    const EntityHandle controlled=runtime.GetControlledEntity();
    ASSERT_NE(controlled,kNullEntity);
    const GameplayNodeLinkComponent* link=runtime.GetWorld().TryGetNodeLink(controlled);
    ASSERT_NE(link,nullptr);
    ASSERT_GE(link->nodeIndex,0);
    EXPECT_EQ(level.nodes[static_cast<std::size_t>(link->nodeIndex)].name,"GOAP_Observer_Player");
    EXPECT_TRUE(runtime.GetWorld().HasPlayerControlled(controlled));
    EXPECT_NE(runtime.GetWorld().TryGetAnimationLink(controlled),nullptr);
}

TEST(GameplayAIGOAPAccessKeyDevelopmentScenario, FailedStartLeavesNoPartialScenarioState)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level=MakeLevel(); LevelInstance instance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    auto editor=Context(level,instance,scene,GameplayRuntimeMode::Editor);
    ASSERT_NE(runtime.SpawnNodeBoundEntity(editor,0,true),kNullEntity);
    auto game=Context(level,instance,scene,GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    GameplayAIGOAPAccessKeyDevelopmentScenario scenario{};
    ASSERT_TRUE(scenario.Prepare(runtime,game));

    EXPECT_FALSE(scenario.Start(runtime,game));
    EXPECT_FALSE(scenario.IsActive());
    EXPECT_EQ(scenario.GetStatus(),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(scenario.GetPlayerEntity(),kNullEntity);
    EXPECT_EQ(scenario.GetAgentEntity(),kNullEntity);
}

TEST(GameplayAIGOAPAccessKeyDevelopmentScenario, ObservesPhysicalProgressCompletesStablyAndResets)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level=MakeLevel(); LevelInstance instance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    auto editor=Context(level,instance,scene,GameplayRuntimeMode::Editor);
    const EntityHandle player=runtime.SpawnNodeBoundEntity(editor,0,true);
    runtime.GetWorld().AddAnimationLink(player,{.skinnedDrawIndex=0});
    auto game=Context(level,instance,scene,GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    GameplayAIGOAPAccessKeyDevelopmentScenario scenario{};
    ASSERT_TRUE(scenario.Prepare(runtime,game)); ASSERT_TRUE(scenario.Start(runtime,game));
    const EntityHandle agent=scenario.GetAgentEntity();

    scenario.Update(runtime); // Planning predicts the key, but observation must remain real.
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,10};
    scenario.Observe(runtime.GetWorld());
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPAtDestinationFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,-7};
    scenario.Observe(runtime.GetWorld());
    EXPECT_TRUE(scenario.GetObservedFacts().IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,10};
    scenario.Observe(runtime.GetWorld());
    EXPECT_TRUE(scenario.GetObservedFacts().IsFactSet(kGOAPAtDestinationFact));
    scenario.Update(runtime);
    EXPECT_EQ(scenario.GetStatus(),AIPlanExecutionStatus::Succeeded);
    scenario.Update(runtime);
    EXPECT_EQ(scenario.GetStatus(),AIPlanExecutionStatus::Succeeded);

    EXPECT_EQ(scenario.Reset(runtime),agent);
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_FALSE(scenario.GetObservedFacts().IsFactSet(kGOAPAtDestinationFact));
    EXPECT_EQ(runtime.GetWorld().TryGetTransform(agent)->position,level.nodes[1].transform.position);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::NotStarted);
    EXPECT_FALSE(scenario.IsActive());
    EXPECT_TRUE(runtime.GetWorld().IsEntityValid(player));
}