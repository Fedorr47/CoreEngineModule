#include <gtest/gtest.h>
#include <algorithm>
#include <array>
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
    
    void Tick(GameplayRuntime& runtime, const GameplayUpdateContext& game)
    {
        runtime.BeginFrame();
        runtime.PrePhysicsUpdate(game);
    }
}

TEST(GameplayAIDecision, AuthoredCoinsAndAccessKeyUsePhysicalObservationAndCancelCleanly)
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
    for (const std::string_view coinName : {"GOAP_Coin_A", "GOAP_Coin_B", "GOAP_Coin_C"})
    {
        EntityHandle coin = FindNodeEntity(runtime, level, coinName);
        if (coin == kNullEntity)
        {
            const auto node = std::ranges::find_if(level.nodes, [&](const LevelNode& value)
                { return value.name == coinName; });
            ASSERT_NE(node, level.nodes.end());
            coin = runtime.SpawnNodeBoundEntity(game,
                static_cast<int>(std::distance(level.nodes.begin(), node)), false);
        }
        ASSERT_NE(coin, kNullEntity);
        runtime.GetWorld().SetPickup(coin, {});
    }
    if (!runtime.GetWorld().HasAI(agent)) { runtime.GetWorld().AddAI(agent); }
    auto moveToRequests=ai_access_key_detail::BuildMoveToProvider(runtime.GetWorld(),{0,0,0},
        {-6,0.35f,-3},{6,0.35f,1},{-5,0.35f,6},{0,0.35f,-7},{0,0.08f,10});
    runtime.GetWorld().TryGetTransform(agent)->position={-5,0.35f,6};
    const auto resolveMove = [&](const AIActionContextId context)
    {
        return moveToRequests.ResolveRequest(
            AIActionRuntimeContext{.agentEntity=agent, .actionId=kAIMoveToActionId,
                .contextId=context});
    };
    const auto coinARequest=resolveMove(kGOAPCoinAMoveContext);
    ASSERT_TRUE(coinARequest.has_value());
    EXPECT_EQ(coinARequest->startNodeId,GameplayRouteNodeId{4u});
    EXPECT_EQ(coinARequest->goalNodeId,GameplayRouteNodeId{2u});
    const auto coinBRequest=resolveMove(kGOAPCoinBMoveContext);
    const auto coinCRequest=resolveMove(kGOAPCoinCMoveContext);
    const auto keyRequest=resolveMove(kGOAPAccessKeyMoveContext);
    const auto goalRequest=resolveMove(kGOAPFinalGoalMoveContext);
    ASSERT_TRUE(coinBRequest.has_value());
    ASSERT_TRUE(coinCRequest.has_value());
    ASSERT_TRUE(keyRequest.has_value());
    ASSERT_TRUE(goalRequest.has_value());
    EXPECT_EQ(coinBRequest->goalNodeId,GameplayRouteNodeId{3u});
    EXPECT_EQ(coinCRequest->goalNodeId,GameplayRouteNodeId{4u});
    EXPECT_EQ(keyRequest->goalNodeId,GameplayRouteNodeId{5u});
    EXPECT_EQ(goalRequest->goalNodeId,GameplayRouteNodeId{6u});
    EXPECT_FALSE(moveToRequests.ResolveRequest(
        AIActionRuntimeContext{.agentEntity=agent, .actionId=kAIMoveToActionId,
            .contextId=AIActionContextId{99u}}).has_value());
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,0};
    ASSERT_TRUE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    EXPECT_FALSE(runtime.StartAIDecision(agent,kAccessKeyAIDecisionId));
    
    Tick(runtime,game);
    const AIAgentWorldState* facts=runtime.GetAIDecisionObservedState(agent);
    ASSERT_NE(facts,nullptr);
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0,10};
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPAtDestinationFact));
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    Tick(runtime,game);
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    runtime.GetWorld().TryGetTransform(agent)->position={-6,0.35f,-3};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinACollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={6,0.35f,1};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={-5,0.35f,6};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPCoinCCollectedFact));
    EXPECT_FALSE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.35f,-7};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPHasAccessKeyFact));
    Tick(runtime,game);
    Tick(runtime,game);
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::RunningStep);
    EXPECT_EQ(runtime.GetAIActionStatus(agent),AIActionExecutionStatus::Running);
    runtime.GetWorld().TryGetTransform(agent)->position={0,0.08f,10};
    Tick(runtime,game);
    EXPECT_TRUE(facts->IsFactSet(kGOAPAtDestinationFact));
    EXPECT_EQ(runtime.GetAIDecisionStatus(agent),AIPlanExecutionStatus::Succeeded);
    Tick(runtime,game);
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

TEST(GameplayAIDecision, AccessKeyMapsOnlyMatchingAgentAndCoinEvents)
{
    InlineThreadOwnerRolesGuard guard{};
    test::LevelInstantiateHarness harness{};
    LevelAsset level=LoadLevelAssetFromJson("levels/ai_goap_access_key_development.level.json");
    LevelInstance instance=harness.Instantiate(level);
    GameplayRuntime runtime{}; runtime.Initialize(level,instance,harness.GetScene());
    auto game=Context(level,instance,harness.GetScene(),GameplayRuntimeMode::Game);
    runtime.BeginFrame(); runtime.PrePhysicsUpdate(game);
    GameplayWorld& world=runtime.GetWorld();
    const auto spawnNamed = [&](const std::string_view name)
    {
        if (const EntityHandle existing=FindNodeEntity(runtime,level,name); existing != kNullEntity)
            return existing;
        const auto node=std::ranges::find_if(level.nodes,
            [&](const LevelNode& value) { return value.name == name; });
        EXPECT_NE(node,level.nodes.end());
        return runtime.SpawnNodeBoundEntity(game,
            static_cast<int>(std::distance(level.nodes.begin(),node)),false);
    };
    const EntityHandle agent=spawnNamed("GOAP_Agent");
    const EntityHandle coinA=spawnNamed("GOAP_Coin_A");
    const EntityHandle coinB=spawnNamed("GOAP_Coin_B");
    ASSERT_NE(agent,kNullEntity); ASSERT_NE(coinA,kNullEntity); ASSERT_NE(coinB,kNullEntity);
    const EntityHandle coinC=spawnNamed("GOAP_Coin_C");
    ASSERT_NE(coinC,kNullEntity);
    world.AddPickup(coinA); world.AddPickup(coinB); world.AddPickup(coinC);
    world.AddAI(agent);
    const EntityHandle otherAgent=world.CreateEntity();
    world.AddTransform(otherAgent,{}); world.AddAI(otherAgent);
    GameplayTraversalLinkRegistry links;
    GameplayTraversalExecutorRegistry executors;
    auto decision=CreateAccessKeyAIDecision(agent,level,world,links,executors);
    ASSERT_NE(decision,nullptr);
    AISystem ai;

    const std::array coinBEvent{GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
        agent,coinB}};
    decision->Update(ai,GameplayAIObservationContext{world,coinBEvent});
    EXPECT_TRUE(decision->GetObservedState().IsFactSet(kGOAPCoinBCollectedFact));
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));

    const std::array otherAgentCoinAEvent{GameplayWorldEvent{
        GameplayWorldEventType::PickupCollected,otherAgent,coinA}};
    decision->Update(ai,GameplayAIObservationContext{world,otherAgentCoinAEvent});
    EXPECT_FALSE(decision->GetObservedState().IsFactSet(kGOAPCoinACollectedFact));
}