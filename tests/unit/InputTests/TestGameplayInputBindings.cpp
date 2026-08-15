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
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionJump));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionLightAttack));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionInteract));
}

TEST(GameplayInputBindings, ConfiguredMappingsAndUnboundKeysAreDataDriven)
{
    const std::vector<GameplayActionKeyBinding> actions{
        { 'J', kGameplayActionJump }, { 0x78, kGameplayActionLightAttack },
        { 'I', kGameplayActionInteract }
    };
    const auto jumpAndAttack = ReadPressed(actions, { 'J', 0x78, 'U' });
    EXPECT_TRUE(HasGameplayActionIntent(jumpAndAttack.actionIntents, kGameplayActionJump));
    EXPECT_TRUE(HasGameplayActionIntent(jumpAndAttack.actionIntents, kGameplayActionLightAttack));
    EXPECT_FALSE(HasGameplayActionIntent(jumpAndAttack.actionIntents, kGameplayActionInteract));

    const auto interact = ReadPressed(actions, { 'I' });
    EXPECT_TRUE(HasGameplayActionIntent(interact.actionIntents, kGameplayActionInteract));
    const auto unbound = ReadPressed(actions, { 'U' });
    EXPECT_TRUE(unbound.actionIntents.empty());
}

TEST(GameplayInputBindings, InvalidEntriesAreIgnored)
{
    const auto intent = ReadPressed({ { 0, kGameplayActionJump }, { 'J', GameplayActionId{} } }, { 'J' });
    EXPECT_TRUE(intent.actionIntents.empty());
}

TEST(GameplayInputBindings, KeyboardCaptureSuppressesGameplayBindings)
{
    InputState input{};
    input.capture.captureKeyboard = true;
    input.keyDown[static_cast<std::uint8_t>('W')] = 1;
    input.keyDown[static_cast<std::uint8_t>(0x10)] = 1;
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;

    GameplayKeyboardMouseBindings bindings{};
    bindings.actions = { { 'J', kGameplayActionJump } };
    GameplayInputIntentComponent intent{};
    ReadKeyboardMouseGameplayIntent(input, bindings, intent);

    EXPECT_FLOAT_EQ(intent.moveX, 0.0f);
    EXPECT_FLOAT_EQ(intent.moveY, 0.0f);
    EXPECT_FALSE(intent.runHeld);
    EXPECT_TRUE(intent.actionIntents.empty());
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
    reserved.actions = { { 0x74, kGameplayActionJump } };
    EXPECT_FALSE(runtime.ApplyKeyboardMouseBindings(reserved));
    EXPECT_FALSE(runtime.GetKeyboardMouseBindings().actions.empty());

    GameplayKeyboardMouseBindings first{};
    first.actions = { { 'J', kGameplayActionJump } };
    ASSERT_TRUE(runtime.ApplyKeyboardMouseBindings(first));

    InputState input{};
    context.mode = GameplayRuntimeMode::Game;
    context.input = &input;
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;
    runtime.PrePhysicsUpdate(context);
    const GameplayInputIntentComponent* intent = runtime.GetWorld().TryGetInputIntent(entity);
    ASSERT_NE(intent, nullptr);
    EXPECT_TRUE(HasGameplayActionIntent(intent->actionIntents, kGameplayActionJump));

    GameplayKeyboardMouseBindings second{};
    second.actions = { { 'K', kGameplayActionJump } };
    ASSERT_TRUE(runtime.ApplyKeyboardMouseBindings(second));
    input.keyPressed.fill(0);
    input.keyPressed[static_cast<std::uint8_t>('J')] = 1;
    runtime.PrePhysicsUpdate(context);
    EXPECT_FALSE(HasGameplayActionIntent(intent->actionIntents, kGameplayActionJump));

    input.keyPressed.fill(0);
    input.keyPressed[static_cast<std::uint8_t>('K')] = 1;
    runtime.PrePhysicsUpdate(context);
    EXPECT_TRUE(HasGameplayActionIntent(intent->actionIntents, kGameplayActionJump));
}

TEST(GameplayInputBindings, ArbitrarySemanticActionHasNoMaskLimitAndValidates)
{
    GameplayActionDefinitions definitions = MakeDefaultGameplayActionDefinitions();
    const GameplayActionId punching{ "Combat.PunchingAttack" };
    definitions.push_back({ punching, GameplayActionPolicyGroup::Combat, GameplayActionRequestSource::Combat, GameplayActionExecutorKind::CombatAttack, 10, GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded) });
    std::string diagnostic;
    EXPECT_TRUE(ValidateGameplayInputAction(definitions, punching, diagnostic));
    EXPECT_FALSE(ValidateGameplayInputAction(definitions, GameplayActionId{ "Combat.DoesNotExist" }, diagnostic));
    EXPECT_NE(diagnostic.find("Combat.DoesNotExist"), std::string::npos);
    const auto intent = ReadPressed({ { 0x78, punching }, { 'K', GameplayActionId{ "Combat.HeavyAttack" } } }, { 0x78, 'K' });
    ASSERT_EQ(intent.actionIntents.size(), 2u);
    EXPECT_EQ(intent.actionIntents[0], punching);
}

TEST(GameplayInputBindings, RuntimeOwnedCatalogsAreIsolated)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime first{};
    GameplayRuntime second{};
    GameplayActionDefinitions firstDefinitions = MakeDefaultGameplayActionDefinitions();
    firstDefinitions.push_back({ GameplayActionId{ "Combat.FirstOnly" },
        GameplayActionPolicyGroup::Combat, GameplayActionRequestSource::Combat,
        GameplayActionExecutorKind::CombatAttack, 10,
        GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded) });
    GameplayActionAnimationBindings firstBindings = MakeDefaultGameplayActionAnimationBindings();
    firstBindings.push_back({ GameplayActionId{ "Combat.FirstOnly" }, "FirstOnly" });
    std::string diagnostic;
    ASSERT_TRUE(first.ApplyGameplayActionConfiguration(firstDefinitions, firstBindings, diagnostic));
    EXPECT_NE(FindGameplayActionDefinition(first.GetGameplayActionDefinitions(),
        GameplayActionId{ "Combat.FirstOnly" }), nullptr);
    EXPECT_EQ(FindGameplayActionDefinition(second.GetGameplayActionDefinitions(),
        GameplayActionId{ "Combat.FirstOnly" }), nullptr);
}
