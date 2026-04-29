#include <array>
#include <gtest/gtest.h>

import core;

using namespace rendern;

namespace
{
    struct PolicySelectionCase
    {
        const char* name{};
        GameplayActionPolicyGroup group{ GameplayActionPolicyGroup::None };
        std::uint32_t intentMask{ 0u };
        bool grounded{ true };
        GameplayActionKind expectedPending{ GameplayActionKind::None };
        GameplayActionKind expectedBuffered{ GameplayActionKind::None };
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
            testPolicy.intentMask,
            testPolicy.group);
        
        return actionComponent;
    }
}

TEST(GameplayActionPolicy, PolicyTableSelectionIsDeterministic)
{
    const std::array<PolicySelectionCase, 6> cases{ {
        {
            .name = "NoIntentMaskProducesNoRequest",
            .group = GameplayActionPolicyGroup::Combat,
            .intentMask = 0u,
            .grounded = true,
            .expectedPending = GameplayActionKind::None,
            .expectedBuffered = GameplayActionKind::None,
            .expectedQueuedAny = false
        },
        {
            .name = "SingleAttackIntentQueuesAttack",
            .group = GameplayActionPolicyGroup::Combat,
            .intentMask = GameplayActionIntentMask(GameplayActionKind::LightAttack),
            .grounded = true,
            .expectedPending = GameplayActionKind::LightAttack,
            .expectedBuffered = GameplayActionKind::None,
            .expectedQueuedAny = true
        },
        {
            .name = "JumpAndAttackSelectJumpAsHighestPriority",
            .group = GameplayActionPolicyGroup::Combat,
            .intentMask = GameplayActionIntentMask(GameplayActionKind::Jump) |
                GameplayActionIntentMask(GameplayActionKind::LightAttack),
            .grounded = true,
            .expectedPending = GameplayActionKind::Jump,
            .expectedBuffered = GameplayActionKind::LightAttack,
            .expectedQueuedAny = true
        },
        {
            .name = "InteractionGroupIgnoresCombatIntents",
            .group = GameplayActionPolicyGroup::Interaction,
            .intentMask = GameplayActionIntentMask(GameplayActionKind::Jump) |
                GameplayActionIntentMask(GameplayActionKind::LightAttack),
            .grounded = true,
            .expectedPending = GameplayActionKind::None,
            .expectedBuffered = GameplayActionKind::None,
            .expectedQueuedAny = false
        },
        {
            .name = "AnyGroupChoosesCombatThenBuffersInteractionByPriority",
            .group = GameplayActionPolicyGroup::Any,
            .intentMask = GameplayActionIntentMask(GameplayActionKind::Jump) |
                GameplayActionIntentMask(GameplayActionKind::Interact),
            .grounded = true,
            .expectedPending = GameplayActionKind::Jump,
            .expectedBuffered = GameplayActionKind::Interact,
            .expectedQueuedAny = true
        },
        {
            .name = "GroundedGateBlocksAllWhenAirborne",
            .group = GameplayActionPolicyGroup::Any,
            .intentMask = GameplayActionIntentMask(GameplayActionKind::Jump) |
                GameplayActionIntentMask(GameplayActionKind::LightAttack) |
                GameplayActionIntentMask(GameplayActionKind::Interact),
            .grounded = false,
            .expectedPending = GameplayActionKind::None,
            .expectedBuffered = GameplayActionKind::None,
            .expectedQueuedAny = false
        }
    } };

    for (const PolicySelectionCase& testCase : cases)
    {
        SCOPED_TRACE(testCase.name);

        bool queuedAny = false;
        const GameplayActionComponent action = RunPolicySelectionCase(testCase, queuedAny);

        EXPECT_EQ(queuedAny, testCase.expectedQueuedAny);
        EXPECT_EQ(GetGameplayRequestedActionKind(action), testCase.expectedPending);
        EXPECT_EQ(GetGameplayBufferedActionKind(action), testCase.expectedBuffered);
    }
}

TEST(GameplayActionPolicy, LowerPriorityRequestCannotOverrideHigherPriorityPending)
{
    GameplayActionComponent action{};

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .kind = GameplayActionKind::Jump,
        .source = GameplayActionRequestSource::Input,
        .priority = 200
    }));

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .kind = GameplayActionKind::LightAttack,
        .source = GameplayActionRequestSource::Input,
        .priority = 10
    }));

    EXPECT_EQ(GetGameplayRequestedActionKind(action), GameplayActionKind::Jump);
    EXPECT_EQ(GetGameplayBufferedActionKind(action), GameplayActionKind::LightAttack);
}

TEST(GameplayActionPolicy, CommitConsumesPendingAndIsIdempotentWhenNoRequestRemains)
{
    GameplayActionComponent action{};
    action.pending = GameplayActionRequest{
        .kind = GameplayActionKind::Interact,
        .source = GameplayActionRequestSource::Interaction,
        .priority = 50
    };

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, GameplayActionKind::Interact);
    EXPECT_TRUE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, GameplayActionKind::Interact);
    EXPECT_TRUE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));
}

TEST(GameplayActionPolicy, FinishPromotesBufferedThenConsumeAndResetAreSafe)
{
    GameplayActionComponent action{};
    action.current = GameplayActionKind::Jump;
    action.busy = true;
    action.buffered = GameplayActionRequest{
        .kind = GameplayActionKind::LightAttack,
        .source = GameplayActionRequestSource::Combat,
        .priority = 10
    };

    FinishGameplayActionState(action);
    EXPECT_FALSE(action.busy);
    EXPECT_EQ(action.current, GameplayActionKind::None);
    EXPECT_EQ(GetGameplayRequestedActionKind(action), GameplayActionKind::LightAttack);
    EXPECT_EQ(GetGameplayBufferedActionKind(action), GameplayActionKind::None);

    CommitGameplayActionState(action);
    EXPECT_EQ(action.current, GameplayActionKind::LightAttack);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));

    ResetGameplayActionState(action);
    EXPECT_EQ(action.current, GameplayActionKind::None);
    EXPECT_FALSE(action.busy);
    EXPECT_FALSE(HasGameplayPendingActionRequest(action));
    EXPECT_FALSE(HasGameplayBufferedActionRequest(action));

    EXPECT_TRUE(QueueGameplayActionRequest(action, GameplayActionRequest{
        .kind = GameplayActionKind::Interact,
        .source = GameplayActionRequestSource::Interaction,
        .priority = 50
    }));
    EXPECT_EQ(GetGameplayRequestedActionKind(action), GameplayActionKind::Interact);
}
