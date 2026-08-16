#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

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

TEST(GameplayInputBindings, DefaultsLeaveLightAttackUnbound)
{
    GameplayKeyboardMouseBindings defaults{};
    const auto intent = ReadPressed(defaults.actions, { 0x20, 'F', 'E' });
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionJump));
    EXPECT_FALSE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionLightAttack));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionInteract));
}

TEST(GameplayInputBindings, MousePressedEdgeAndDeviceCaptureAreIndependent)
{
    GameplayKeyboardMouseBindings bindings{};
    bindings.actions = { { kGameplayMouseLeft, kGameplayActionLightAttack }, { 'J', kGameplayActionJump } };
    InputState input{};
    input.keyPressed[kGameplayMouseLeft] = 1;
    input.keyPressed['J'] = 1;
    GameplayInputIntentComponent intent{};
    input.capture.captureMouse = true;
    ReadKeyboardMouseGameplayIntent(input, bindings, intent);
    EXPECT_FALSE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionLightAttack));
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionJump));

    intent = {};
    input.capture = { true, false };
    ReadKeyboardMouseGameplayIntent(input, bindings, intent);
    EXPECT_TRUE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionLightAttack));
    EXPECT_FALSE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionJump));

    intent = {};
    input.capture = {};
    input.keyPressed[kGameplayMouseLeft] = 0;
    input.keyDown[kGameplayMouseLeft] = 1;
    ReadKeyboardMouseGameplayIntent(input, bindings, intent);
    EXPECT_FALSE(HasGameplayActionIntent(intent.actionIntents, kGameplayActionLightAttack));
}

TEST(GameplayInputBindings, LevelBindingsRoundTripKeyboardAndMouse)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "gameplay_input_roundtrip.level.json";
    LevelAsset source{};
    source.name = "InputRoundTrip";
    const GameplayActionId punching{ "Combat.PunchingAttack" };
    source.gameplayActions = MakeDefaultGameplayActionDefinitions();
    source.gameplayActions.push_back({ punching, GameplayActionPolicyGroup::Combat,
        GameplayActionRequestSource::Combat, GameplayActionExecutorKind::CombatAttack, 10, 0 });
    source.gameplayKeyboardMouseBindings.actions = {
        { 'F', punching }, { kGameplayMouseLeft, punching } };
    SaveLevelAssetToJson(path.string(), source);
    const LevelAsset loaded = LoadLevelAssetFromJson(path.string());
    ASSERT_EQ(loaded.gameplayKeyboardMouseBindings.actions.size(), 2u);
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[0].key, 'F');
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[1].key, kGameplayMouseLeft);
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[1].action, punching);
    std::filesystem::remove(path);
}

TEST(GameplayInputBindings, PersistedMouseMovementAndRunBindingsAreRejected)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "gameplay_input_invalid_keyboard_fields.level.json";
    LevelAsset source{};
    source.gameplayKeyboardMouseBindings.moveX.negativeKey = kGameplayMouseLeft;
    SaveLevelAssetToJson(path.string(), source);
    EXPECT_THROW(static_cast<void>(LoadLevelAssetFromJson(path.string())), std::runtime_error);

    source.gameplayKeyboardMouseBindings = {};
    source.gameplayKeyboardMouseBindings.run.key = kGameplayMouseMiddle;
    SaveLevelAssetToJson(path.string(), source);
    EXPECT_THROW(static_cast<void>(LoadLevelAssetFromJson(path.string())), std::runtime_error);

    source.gameplayKeyboardMouseBindings = {};
    source.gameplayKeyboardMouseBindings.run.key = 0x74;
    SaveLevelAssetToJson(path.string(), source);
    EXPECT_THROW(static_cast<void>(LoadLevelAssetFromJson(path.string())), std::runtime_error);
    std::filesystem::remove(path);
}

TEST(GameplayInputBindings, PersistedKeyboardMovementAndMouseActionLoadSuccessfully)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "gameplay_input_valid_devices.level.json";
    LevelAsset source{};
    source.gameplayKeyboardMouseBindings.moveX = { 'A', 'D' };
    source.gameplayKeyboardMouseBindings.moveY = { 'S', 'W' };
    source.gameplayKeyboardMouseBindings.run.key = 0x10;
    source.gameplayKeyboardMouseBindings.actions = { { kGameplayMouseLeft, kGameplayActionLightAttack } };
    SaveLevelAssetToJson(path.string(), source);
    const LevelAsset loaded = LoadLevelAssetFromJson(path.string());
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.moveX.negativeKey, 'A');
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.moveX.positiveKey, 'D');
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.moveY.negativeKey, 'S');
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.moveY.positiveKey, 'W');
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.run.key, 0x10);
    ASSERT_EQ(loaded.gameplayKeyboardMouseBindings.actions.size(), 1u);
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[0].key, kGameplayMouseLeft);
    std::filesystem::remove(path);
}

TEST(GameplayInputBindings, LevelWithoutBindingsUsesDefaults)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "gameplay_input_legacy.level.json";
    {
        std::ofstream file(path);
        file << "{\"name\":\"Legacy\"}";
    }
    const LevelAsset loaded = LoadLevelAssetFromJson(path.string());
    ASSERT_FALSE(loaded.gameplayKeyboardMouseBindings.actions.empty());
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[0].key, 0x20);
    ASSERT_EQ(loaded.gameplayKeyboardMouseBindings.actions.size(), 2u);
    EXPECT_EQ(loaded.gameplayKeyboardMouseBindings.actions[1].key, 'E');
    std::filesystem::remove(path);
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
    EXPECT_TRUE(IsGameplayActionBindingKeyReserved(kGameplayMouseRight));
    EXPECT_FALSE(IsGameplayActionBindingKeyReserved(kGameplayMouseLeft));
}

TEST(GameplayInputBindings, MovementAndRunSupportKeyboardKeysOnly)
{
    EXPECT_TRUE(IsSupportedGameplayKeyboardKey('W'));
    EXPECT_FALSE(IsSupportedGameplayKeyboardKey(kGameplayMouseLeft));
    EXPECT_TRUE(IsSupportedGameplayActionInput('F'));
    EXPECT_TRUE(IsSupportedGameplayActionInput(kGameplayMouseLeft));
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

    GameplayKeyboardMouseBindings mouseMovement{};
    mouseMovement.moveX.positiveKey = kGameplayMouseLeft;
    EXPECT_FALSE(runtime.ApplyKeyboardMouseBindings(mouseMovement));
    EXPECT_EQ(runtime.GetKeyboardMouseBindings().moveX.positiveKey, 'A');
    
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
    const std::filesystem::path persistedPath = std::filesystem::temp_directory_path() / "gameplay_input_apply.level.json";
    levelAsset.sourcePath = persistedPath.string();
    ASSERT_TRUE(runtime.ApplyKeyboardMouseBindings(first));
    const LevelAsset persisted = LoadLevelAssetFromJson(persistedPath.string());
    ASSERT_EQ(persisted.gameplayKeyboardMouseBindings.actions.size(), 1u);
    EXPECT_EQ(persisted.gameplayKeyboardMouseBindings.actions[0].key, 'J');

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
    std::filesystem::remove(persistedPath);
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
