#include <gtest/gtest.h>
#include <algorithm>
#include "TestSupport/TestThreadAffinity.h"
#include "unit/RenderTests/LevelInstantiateTestHelper.h"

import core;

using namespace rendern;

namespace
{
    GameplayUpdateContext Context(LevelAsset& level, LevelInstance& instance, Scene& scene,
        const GameplayRuntimeMode mode)
    {
        return {.deltaSeconds=1.0f/60.0f, .mode=mode, .levelAsset=&level,
            .levelInstance=&instance, .scene=&scene};
    }

    EntityHandle FindNodeEntity(GameplayRuntime& runtime, const LevelAsset& level,
        const std::string_view name)
    {
        for (const EntityHandle entity : runtime.GetNodeBoundEntities())
        {
            const auto* link = runtime.GetWorld().TryGetNodeLink(entity);
            if (link != nullptr && link->nodeIndex >= 0 &&
                level.nodes[static_cast<std::size_t>(link->nodeIndex)].name == name)
            {
                return entity;
            }
        }
        return kNullEntity;
    }
}

TEST(GameplayAIDecision, AuthoredAccessKeyUsesPhysicalObservationAndCancelsCleanly)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    EXPECT_EQ(level.developmentScenario, "development/ai_goap_access_key.scenario.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game); runtime.PostPhysicsUpdate(game);
    const EntityHandle player=runtime.GetControlledEntity();
    EntityHandle agent=FindNodeEntity(runtime,level,"GOAP_Agent");
    if (agent == kNullEntity)
    {
        const auto node = std::ranges::find_if(level.nodes,
            [](const LevelNode& value) { return value.name == "GOAP_Agent"; });
        ASSERT_NE(node,level.nodes.end());
        agent=runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(),node)),false);
    }
    ASSERT_NE(player,kNullEntity); ASSERT_NE(agent,kNullEntity); EXPECT_NE(player,agent);
    if (!runtime.GetWorld().HasAI(agent)) { runtime.GetWorld().AddAI(agent); }
    ASSERT_TRUE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    EXPECT_FALSE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));

    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    const AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,10};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtDestinationFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.08f,10};
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPAtDestinationFact));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::Succeeded);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::Succeeded);
    EXPECT_TRUE(runtime.DestroyNodeBoundEntity(agent));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(agent),nullptr);
}

TEST(GameplayAIDecision, MissingMovementContractRejectsStartWithoutPartialState)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    GameplayWorld& world=runtime.GetWorld();
    const EntityHandle incomplete=world.CreateEntity();
    world.AddTransform(incomplete,{}); world.AddAI(incomplete);
    EXPECT_FALSE(runtime.StartAIDecision(incomplete,kAccessKeyAIDecisionId));
    EXPECT_EQ(runtime.GetAIDecisionStatus(incomplete),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(incomplete),nullptr);
}

TEST(GameplayAIDecision, FailedStartLeavesNoActiveDecision)
{
    InlineThreadOwnerRolesGuard guard{};
    LevelAsset level{}; LevelNode node{}; node.alive=true; node.name="Agent";
    level.nodes.push_back(node); LevelInstance instance{}; Scene scene{}; GameplayRuntime runtime{};
    runtime.Initialize(level,instance,scene);
    
    auto editor=Context(level,instance,scene,GameplayRuntimeMode::Editor);
    const EntityHandle agent=runtime.SpawnNodeBoundEntity(editor,0,false);
    ASSERT_NE(agent,kNullEntity);
    runtime.GetWorld().AddAI(agent);
        // Enter Game mode first so this test exercises production decision
    // creation failure rather than merely the Editor-mode start guard.
    auto game=Context(level,instance,scene,GameplayRuntimeMode::Game);
    runtime.BeginFrame();
    runtime.PrePhysicsUpdate(game);
    runtime.PostPhysicsUpdate(game);
        ASSERT_TRUE(runtime.GetWorld().HasTransform(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterCommand(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterMotor(agent));
    ASSERT_TRUE(runtime.GetWorld().HasCharacterMovementState(agent));
    
    EXPECT_FALSE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::NotStarted);
    EXPECT_EQ(runtime.GetAIDecisionObservedState(agent),nullptr);
}