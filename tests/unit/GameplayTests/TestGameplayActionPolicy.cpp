#include <array>
#include <filesystem>
#include <gtest/gtest.h>

import core;

using namespace rendern;

namespace
{
    struct PolicySelectionCase
    {
        const char* name{};
        GameplayActionPolicyGroup group{ GameplayActionPolicyGroup::None };
        std::vector<GameplayActionId> intents{};
        bool grounded{ true };
        GameplayActionId expectedPending{ GameplayActionId{} };
        GameplayActionId expectedBuffered{ GameplayActionId{} };
        bool expectedQueuedAny { false };
    };
    
    [[nodiscard]] GameplayActionComponent RunPolicySelectionCase(const PolicySelectionCase& testPolicy, bool& queuedAny)
    {
        GameplayActionComponent actionComponent;
        GameplayCharacterMovementStateComponent characterMovementStateComponent;
        characterMovementStateComponent.grounded = testPolicy.grounded;
        
        queuedAny = QueueGameplayActionRequestsFromPolicies(
            actionComponent,
            &characterMovementStateComponent,
            testPolicy.intents,
            testPolicy.group,
            MakeDefaultGameplayActionDefinitions());
        
        return actionComponent;
    }
}

TEST(GameplayActionPolicy, PolicyTableSelectionIsDeterministic)
{
    const std::array<PolicySelectionCase, 6> cases{ {
        {
            .name = "NoIntentMaskProducesNoRequest",
            .group = GameplayActionPolicyGroup::Combat,
            .intents = {},
            .grounded = true,
            .expectedPending = GameplayActionId{},
            .expectedBuffered = GameplayActionId{},
            .expectedQueuedAny = false
        },
        {
            .name = "SingleAttackIntentQueuesAttack",
            .group = GameplayActionPolicyGroup::Combat,
            .intents = { kGameplayActionLightAttack },
            .grounded = true,
            .expectedPending = kGameplayActionLightAttack,
            .expectedBuffered = GameplayActionId{},
            .expectedQueuedAny = true
        },
        {
            .name = "JumpAndAttackSelectJumpAsHighestPriority",
            .group = GameplayActionPolicyGroup::Combat,
            .intents = { kGameplayActionJump, kGameplayActionLightAttack },
            .grounded = true,
            .expectedPending = kGameplayActionJump,
            .expectedBuffered = kGameplayActionLightAttack,
            .expectedQueuedAny = true
        },
        {
            .name = "InteractionGroupIgnoresCombatIntents",
            .group = GameplayActionPolicyGroup::Interaction,
            .intents = { kGameplayActionJump, kGameplayActionLightAttack },
            .grounded = true,
            .expectedPending = GameplayActionId{},
            .expectedBuffered = GameplayActionId{},
            .expectedQueuedAny = false
        },
        {
            .name = "AnyGroupChoosesCombatThenBuffersInteractionByPriority",
            .group = GameplayActionPolicyGroup::Any,
            .intents = { kGameplayActionJump, kGameplayActionInteract },
            .grounded = true,
            .expectedPending = kGameplayActionJump,
            .expectedBuffered = kGameplayActionInteract,
            .expectedQueuedAny = true
        },
        {
            .name = "GroundedGateBlocksAllWhenAirborne",
            .group = GameplayActionPolicyGroup::Any,
            .intents = { kGameplayActionJump, kGameplayActionLightAttack, kGameplayActionInteract },
            .grounded = false,
            .expectedPending = GameplayActionId{},
            .expectedBuffered = GameplayActionId{},
            .expectedQueuedAny = false
        }
    } };

    for (const PolicySelectionCase& testCase : cases)
    {
        SCOPED_TRACE(testCase.name);

        bool queuedAny = false;
        const GameplayActionComponent action = RunPolicySelectionCase(testCase, queuedAny);

        EXPECT_EQ(queuedAny, testCase.expectedQueuedAny);
        EXPECT_EQ(GetGameplayRequestedActionId(action), testCase.expectedPending);
        EXPECT_EQ(GetGameplayBufferedActionId(action), testCase.expectedBuffered);
    }
}

TEST(GameplayActionPolicy, LowerPriorityRequestCannotOverrideHigherPriorityPending)
{
    GameplayActionComponent action{};

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .id = kGameplayActionJump,
        .source = GameplayActionRequestSource::Input,
        .priority = 200
    }));

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .id = kGameplayActionLightAttack,
        .source = GameplayActionRequestSource::Input,
        .priority = 10
    }));

    EXPECT_EQ(GetGameplayRequestedActionId(action), kGameplayActionJump);
    EXPECT_EQ(GetGameplayBufferedActionId(action), kGameplayActionLightAttack);
}

TEST(GameplayActionPolicy, CommitConsumesPendingAndIsIdempotentWhenNoRequestRemains)
{
    GameplayActionComponent action{};
    action.pending = GameplayActionRequest{
        .id = kGameplayActionInteract,
        .source = GameplayActionRequestSource::Interaction,
        .priority = 50
    };

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, kGameplayActionInteract);
    EXPECT_TRUE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, kGameplayActionInteract);
    EXPECT_TRUE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));
}

TEST(GameplayActionPolicy, FinishPromotesBufferedThenConsumeAndResetAreSafe)
{
    GameplayActionComponent action{};
    action.current = kGameplayActionJump;
    action.busy = true;
    action.buffered = GameplayActionRequest{
        .id = kGameplayActionLightAttack,
        .source = GameplayActionRequestSource::Combat,
        .priority = 10
    };

    FinishGameplayActionState(action);
    EXPECT_FALSE(action.busy);
    EXPECT_EQ(action.current, GameplayActionId{});
    EXPECT_EQ(GetGameplayRequestedActionId(action), kGameplayActionLightAttack);
    EXPECT_EQ(GetGameplayBufferedActionId(action), GameplayActionId{});

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, kGameplayActionLightAttack);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));

    ResetGameplayActionState(action);
    EXPECT_EQ(action.current, GameplayActionId{});
    EXPECT_FALSE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));
    EXPECT_FALSE(HasGameplayBufferedActionRequest(action));

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .id = kGameplayActionInteract,
        .source = GameplayActionRequestSource::Interaction,
        .priority = 50
    }));
    EXPECT_EQ(GetGameplayRequestedActionId(action), kGameplayActionInteract);
}

TEST(GameplayActionPolicy, ArbitraryDefinitionDrivesPolicyAndRuntimeState)
{
    const GameplayActionId punching{ "Combat.PunchingAttack" };
    GameplayActionDefinitions definitions{
        { punching, GameplayActionPolicyGroup::Combat, GameplayActionRequestSource::Combat,
          GameplayActionExecutorKind::CombatAttack, 37,
          GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded) }
    };
    const auto* policy = FindGameplayActionPolicy(definitions, GameplayActionPolicyGroup::Combat, punching);
    ASSERT_NE(policy, nullptr);
    EXPECT_EQ(policy->priority, 37);
    GameplayCharacterMovementStateComponent movement{};
    movement.grounded = true;
    GameplayActionComponent action{};
    EXPECT_TRUE(QueueGameplayActionRequestsFromPolicies(action, &movement, { punching }, GameplayActionPolicyGroup::Combat, definitions));
    EXPECT_EQ(action.pending.id, punching);
    PrimeGameplayActionState(action);
    EXPECT_EQ(action.current, punching);
    CommitGameplayActionState(action);
    QueueGameplayActionRequest(action, { GameplayActionId{ "Combat.ArbitraryBuffered" }, GameplayActionRequestSource::Combat, 1 });
    EXPECT_TRUE(HasGameplayBufferedActionRequest(action));
}

TEST(GameplayActionPolicy, CatalogAndPresentationRoundTripThroughLevelJson)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
        "core_gameplay_action_catalog_roundtrip.level.json";
    LevelAsset source{};
    source.name = "Gameplay action round trip";
    source.gameplayActions = MakeDefaultGameplayActionDefinitions();
    source.gameplayActions.push_back({ GameplayActionId{ "Combat.PunchingAttack" },
        GameplayActionPolicyGroup::Combat, GameplayActionRequestSource::Combat,
        GameplayActionExecutorKind::CombatAttack, 10,
        GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireGrounded) });
    source.gameplayActionAnimationBindings = MakeDefaultGameplayActionAnimationBindings();
    source.gameplayActionAnimationBindings.push_back({
        GameplayActionId{ "Combat.PunchingAttack" }, "PunchingAttack" });

    SaveLevelAssetToJson(path.string(), source);
    const LevelAsset loaded = LoadLevelAssetFromJson(path.string());
    const auto* action = FindGameplayActionDefinition(
        loaded.gameplayActions, GameplayActionId{ "Combat.PunchingAttack" });
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->group, GameplayActionPolicyGroup::Combat);
    EXPECT_EQ(action->source, GameplayActionRequestSource::Combat);
    EXPECT_EQ(action->executor, GameplayActionExecutorKind::CombatAttack);
    EXPECT_EQ(action->priority, 10);
    EXPECT_TRUE(HasGameplayActionPolicyGate(action->gates, GameplayActionPolicyGate::RequireGrounded));
    const auto* presentation = FindGameplayActionAnimationBinding(
        loaded.gameplayActionAnimationBindings, action->id);
    ASSERT_NE(presentation, nullptr);
    EXPECT_EQ(presentation->triggerParameter, "PunchingAttack");
    std::filesystem::remove(path);
}

TEST(GameplayActionPolicy, ValidationRejectsInvalidCatalogs)
{
    std::string diagnostic;
    GameplayActionDefinitions emptyId = MakeDefaultGameplayActionDefinitions();
    emptyId.push_back({});
    EXPECT_FALSE(ValidateGameplayActionDefinitions(emptyId, diagnostic));

    GameplayActionDefinitions duplicate = MakeDefaultGameplayActionDefinitions();
    duplicate.push_back(duplicate.front());
    EXPECT_FALSE(ValidateGameplayActionDefinitions(duplicate, diagnostic));

    GameplayActionDefinitions contradictory = MakeDefaultGameplayActionDefinitions();
    contradictory.front().gates |= GameplayActionPolicyGateMask(GameplayActionPolicyGate::RequireAirborne);
    EXPECT_FALSE(ValidateGameplayActionDefinitions(contradictory, diagnostic));

    GameplayActionDefinitions missingRequired = MakeDefaultGameplayActionDefinitions();
    missingRequired.erase(missingRequired.begin());
    EXPECT_FALSE(ValidateGameplayActionDefinitions(missingRequired, diagnostic));
    EXPECT_NE(diagnostic.find("Movement.Jump"), std::string::npos);
}
