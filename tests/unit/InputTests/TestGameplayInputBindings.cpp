#include <gtest/gtest.h>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    GameplayInputIntentComponent ReadPressed(
        const std::vector<GameplayActionKeyBinding>& actions,
        const std::initializer_list<int> pressed)
    {
        InputState input{};
        for (const int key : pressed) input.keyPressed[static_cast<std::uint8_t>(key)] = 1;
        GameplayKeyboardMouseBindings bindings{};
        bindings.actions = actions;
        GameplayInputIntentComponent intent{};
        ReadKeyboardMouseGameplayIntent(input, bindings, intent);
        return intent;
    }
}

TEST(GameplayInputBindings, DefaultMappingsPreserveJumpAttackAndInteract)
{
    GameplayKeyboardMouseBindings defaults{};
    const auto intent = ReadPressed(defaults.actions, { 0x20, 0x78, 'E' });
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntentMask, GameplayActionKind::Jump));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntentMask, GameplayActionKind::LightAttack));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntentMask, GameplayActionKind::Interact));
}

TEST(GameplayInputBindings, ConfiguredMappingsAndUnboundKeysAreDataDriven)
{
    const std::vector<GameplayActionKeyBinding> actions{
        { 'J', GameplayActionKind::Jump }, { 0x78, GameplayActionKind::LightAttack },
        { 'I', GameplayActionKind::Interact }
    };
    const auto jumpAndAttack = ReadPressed(actions, { 'J', 0x78, 'U' });
    EXPECT_TRUE(HasGameplayActionIntent(jumpAndAttack.actionIntentMask, GameplayActionKind::Jump));
    EXPECT_TRUE(HasGameplayActionIntent(jumpAndAttack.actionIntentMask, GameplayActionKind::LightAttack));
    EXPECT_FALSE(HasGameplayActionIntent(jumpAndAttack.actionIntentMask, GameplayActionKind::Interact));

    const auto interact = ReadPressed(actions, { 'I' });
    EXPECT_TRUE(HasGameplayActionIntent(interact.actionIntentMask, GameplayActionKind::Interact));
    const auto unbound = ReadPressed(actions, { 'U' });
    EXPECT_EQ(unbound.actionIntentMask, 0u);
}

TEST(GameplayInputBindings, InvalidEntriesAreIgnored)
{
    const auto intent = ReadPressed({ { 0, GameplayActionKind::Jump }, { 'J', GameplayActionKind::None } }, { 'J' });
    EXPECT_EQ(intent.actionIntentMask, 0u);
}

TEST(GameplayInputBindings, KeyboardCaptureSuppressesGameplayBindings)
{
    InputState input{};
    input.capture.captureKeyboard = true;
    input.keyDown[static_cast<std::uint8_t>('W')] = 1;
    input.keyDown[static_cast<std::uint8_t>(0x10)] = 1;
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;

    GameplayKeyboardMouseBindings bindings{};
    bindings.actions = { { 'J', GameplayActionKind::Jump } };
    GameplayInputIntentComponent intent{};
    ReadKeyboardMouseGameplayIntent(input, bindings, intent);

    EXPECT_FLOAT_EQ(intent.moveX, 0.0f);
    EXPECT_FLOAT_EQ(intent.moveY, 0.0f);
    EXPECT_FALSE(intent.runHeld);
    EXPECT_EQ(intent.actionIntentMask, 0u);
}

TEST(GameplayInputBindings, ApplicationHotkeysAreReservedForAuthoring)
{
    EXPECT_TRUE(IsGameplayActionBindingKeyReserved(0x74));
    EXPECT_TRUE(IsGameplayActionBindingKeyReserved(0x75));
    EXPECT_TRUE(IsGameplayActionBindingKeyReserved(0x76));
    EXPECT_FALSE(IsGameplayActionBindingKeyReserved(0x77));
    EXPECT_FALSE(IsGameplayActionBindingKeyReserved(0x78));
}

TEST(GameplayInputBindings, RuntimeApplyReplacesCapturedBindings)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime runtime{};
    LevelAsset levelAsset{};
    LevelNode node{};
    node.name = "Player";
    node.alive = true;
    node.visible = true;
    levelAsset.nodes.push_back(node);
    LevelInstance levelInstance{};
    Scene scene{};
    runtime.Initialize(levelAsset, levelInstance, scene);

    GameplayUpdateContext context{};
    context.mode = GameplayRuntimeMode::Editor;
    context.levelAsset = &levelAsset;
    context.levelInstance = &levelInstance;
    context.scene = &scene;
    const EntityHandle entity = runtime.SpawnNodeBoundEntity(context, 0, true);
    ASSERT_NE(entity, kNullEntity);

    GameplayKeyboardMouseBindings reserved{};
    reserved.actions = { { 0x74, GameplayActionKind::Jump } };
    EXPECT_FALSE(runtime.ApplyKeyboardMouseBindings(reserved));
    EXPECT_FALSE(runtime.GetKeyboardMouseBindings().actions.empty());

    GameplayKeyboardMouseBindings first{};
    first.actions = { { 'J', GameplayActionKind::Jump } };
    ASSERT_TRUE(runtime.ApplyKeyboardMouseBindings(first));

    InputState input{};
    context.mode = GameplayRuntimeMode::Game;
    context.input = &input;
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;
    runtime.PrePhysicsUpdate(context);
    const GameplayInputIntentComponent* intent = runtime.GetWorld().TryGetInputIntent(entity);
    ASSERT_NE(intent, nullptr);
    EXPECT_TRUE(HasGameplayActionIntent(intent->actionIntentMask, GameplayActionKind::Jump));

    GameplayKeyboardMouseBindings second{};
    second.actions = { { 'K', GameplayActionKind::Jump } };
    ASSERT_TRUE(runtime.ApplyKeyboardMouseBindings(second));
    input.keyPressed.fill(0);
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;
    runtime.PrePhysicsUpdate(context);
    EXPECT_FALSE(HasGameplayActionIntent(intent->actionIntentMask, GameplayActionKind::Jump));

    input.keyPressed.fill(0);
    input.keyPressed[static_cast<std::uint8_t>('K')] = 1;
    runtime.PrePhysicsUpdate(context);
    EXPECT_TRUE(HasGameplayActionIntent(intent->actionIntentMask, GameplayActionKind::Jump));
}
