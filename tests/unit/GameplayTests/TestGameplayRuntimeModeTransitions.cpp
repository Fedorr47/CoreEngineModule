#include <gtest/gtest.h>

#include "TestSupport/TestGlobalVariables.h"

import core;

using namespace rendern;

namespace
{
    LevelAsset MakeMinimalLevelAsset()
    {
        LevelAsset asset;
        asset.name = "GameplayRuntimeTransitionFixture";
        LevelNode node{};
        node.name = "Player";
        node.alive = true;
        node.visible = true;
        node.transform.position = {0.0f, 0.0f, 0.0f};
        asset.nodes.push_back(node);
        return asset;
    }
    
    GameplayUpdateContext MakeGameplayUpdateContext(
        GameplayRuntimeMode mode,
        LevelAsset& levelAsset,
        LevelInstance& levelInstance,
        Scene& scene)
    {
        GameplayUpdateContext context;
        context.deltaSeconds = kStepSeconds;
        context.mode = mode;
        context.input = nullptr;
        context.levelAsset = &levelAsset;
        context.levelInstance = &levelInstance;
        context.scene = &scene;
        return context;
    }
    
    void StepFrame(
        GameplayRuntime& runtime,
        GameplayRuntimeMode mode,
        LevelAsset& levelAsset,
        LevelInstance& levelInstance,
        Scene& scene)
    {
        runtime.BeginFrame();
        const GameplayUpdateContext context = MakeGameplayUpdateContext(mode, levelAsset, levelInstance, scene);
        runtime.PreAnimationUpdate(context);
        runtime.PostAnimationUpdate(context);
    }
}

TEST(GameplayRuntimeModeTransitions, EditorToGame_SetsRuntimeModeAndControlledEntity)
{
    GameplayRuntime runtime{};
    LevelAsset levelAsset = MakeMinimalLevelAsset();
    LevelInstance levelInstance{};
    Scene scene{};
    
    runtime.Initialize(levelAsset, levelInstance, scene);
    
    ASSERT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Editor);
    
    const EntityHandle controlledBeforeGame = runtime.GetControlledEntity();
    ASSERT_NE(controlledBeforeGame, kNullEntity);
    ASSERT_TRUE(runtime.GetWorld().IsEntityValid(controlledBeforeGame));
    
    GameplayWorld& world = runtime.GetWorld();
    GameplayInputIntentComponent* intent = world.TryGetInputIntent(controlledBeforeGame);
    GameplayCharacterCommandComponent* characterCommand = world.TryGetCharacterCommand(controlledBeforeGame);
    ASSERT_NE(intent, nullptr);
    ASSERT_NE(characterCommand, nullptr);
    
    intent->moveX = 1.0f;
    intent->runHeld = true;
    characterCommand->moveInputX = 1.0f;
    characterCommand->wantsRun = true;
    
    StepFrame(runtime, GameplayRuntimeMode::Game, levelAsset, levelInstance, scene);
    
    EXPECT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Game);
    EXPECT_EQ(runtime.GetControlledEntity(), controlledBeforeGame);
    ASSERT_TRUE(world.IsEntityValid(runtime.GetControlledEntity()));
    
    intent = world.TryGetInputIntent(controlledBeforeGame);
    characterCommand = world.TryGetCharacterCommand(controlledBeforeGame);
    ASSERT_NE(intent, nullptr);
    ASSERT_NE(characterCommand, nullptr);
    
    EXPECT_FLOAT_EQ(intent->moveX, 0.0f);
    EXPECT_FLOAT_EQ(intent->moveY, 0.0f);
    EXPECT_FALSE(intent->runHeld);
    EXPECT_FLOAT_EQ(characterCommand->moveInputX, 0.0f);
    EXPECT_FLOAT_EQ(characterCommand->moveInputY, 0.0f);
    EXPECT_FALSE(characterCommand->wantsRun);
}

TEST(GameplayRuntimeModeTransitions, GameToEditor_RestoresEditorModeAndClearsOrRestoresControlledEntity)
{
    GameplayRuntime runtime{};
    LevelAsset levelAsset = MakeMinimalLevelAsset();
    LevelInstance levelInstance{};
    Scene scene{};
    
    runtime.Initialize(levelAsset, levelInstance, scene);
    
    const EntityHandle controlled  = runtime.GetControlledEntity();
    ASSERT_NE(controlled , kNullEntity);
    
    StepFrame(runtime, GameplayRuntimeMode::Game, levelAsset, levelInstance, scene);
    
    GameplayWorld& world = runtime.GetWorld();
    auto* movement = world.TryGetCharacterMovementState(controlled);
    auto* motor = world.TryGetCharacterMotor(controlled);
    auto* action = world.TryGetAction(controlled);
    ASSERT_NE(movement, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(action, nullptr);
    
    movement->falling = true;
    movement->jumping = true;
    movement->grounded = false;
    motor->velocity = {2.0f, 1.0f, 0.5f};
    action->busy = true;
    action->pending.kind = GameplayActionKind::Jump;
    
    StepFrame(runtime, GameplayRuntimeMode::Editor, levelAsset, levelInstance, scene);

    EXPECT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Editor);
    EXPECT_EQ(runtime.GetControlledEntity(), controlled);

    movement = world.TryGetCharacterMovementState(controlled);
    motor = world.TryGetCharacterMotor(controlled);
    action = world.TryGetAction(controlled);
    ASSERT_NE(movement, nullptr);
    ASSERT_NE(motor, nullptr);
    ASSERT_NE(action, nullptr);

    EXPECT_TRUE(movement->grounded);
    EXPECT_FALSE(movement->jumping);
    EXPECT_FALSE(movement->falling);
    EXPECT_FLOAT_EQ(motor->velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(motor->velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(motor->velocity.z, 0.0f);
    EXPECT_FALSE(action->busy);
    EXPECT_EQ(action->pending.kind, GameplayActionKind::None);
}

TEST(GameplayRuntimeModeTransitions, EditorGameEditor_RepeatedTransitionsDeterministic)
{
    GameplayRuntime runtime{};
    LevelAsset levelAsset = MakeMinimalLevelAsset();
    LevelInstance levelInstance{};
    Scene scene{};

    runtime.Initialize(levelAsset, levelInstance, scene);
    const EntityHandle controlled = runtime.GetControlledEntity();
    ASSERT_NE(controlled, kNullEntity);

    auto runTransitionCycle = [&]()
    {
        GameplayWorld& world = runtime.GetWorld();
        auto* intent = world.TryGetInputIntent(controlled);
        auto* command = world.TryGetCharacterCommand(controlled);
        auto* action = world.TryGetAction(controlled);
        ASSERT_NE(intent, nullptr);
        ASSERT_NE(command, nullptr);
        ASSERT_NE(action, nullptr);

        intent->moveX = -1.0f;
        intent->moveY = 1.0f;
        command->moveInputX = -1.0f;
        command->moveInputY = 1.0f;
        action->pending.kind = GameplayActionKind::LightAttack;

        StepFrame(runtime, GameplayRuntimeMode::Game, levelAsset, levelInstance, scene);
        EXPECT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Game);
        EXPECT_EQ(runtime.GetControlledEntity(), controlled);

        StepFrame(runtime, GameplayRuntimeMode::Editor, levelAsset, levelInstance, scene);
        EXPECT_EQ(runtime.GetCurrentMode(), GameplayRuntimeMode::Editor);
        EXPECT_EQ(runtime.GetControlledEntity(), controlled);

        intent = world.TryGetInputIntent(controlled);
        command = world.TryGetCharacterCommand(controlled);
        action = world.TryGetAction(controlled);
        ASSERT_NE(intent, nullptr);
        ASSERT_NE(command, nullptr);
        ASSERT_NE(action, nullptr);

        EXPECT_FLOAT_EQ(intent->moveX, 0.0f);
        EXPECT_FLOAT_EQ(intent->moveY, 0.0f);
        EXPECT_FLOAT_EQ(command->moveInputX, 0.0f);
        EXPECT_FLOAT_EQ(command->moveInputY, 0.0f);
        EXPECT_EQ(action->pending.kind, GameplayActionKind::None);
    };

    runTransitionCycle();
    runTransitionCycle();
}