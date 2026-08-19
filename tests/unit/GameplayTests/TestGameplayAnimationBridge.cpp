#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

import core;

namespace
{
    rendern::AnimationParameterDesc BoolParameter(const std::string& name)
    {
        return {
            .name = name,
            .defaultValue = {
                .type = rendern::AnimationParameterType::Bool,
                .boolValue = false
            }
        };
    }

    bool HasCondition(
        const rendern::AnimationTransitionDesc& transition,
        const std::string_view parameter,
        const rendern::AnimationConditionOp op)
    {
        for (const rendern::AnimationConditionDesc& condition : transition.conditions)
        {
            if (condition.parameter == parameter && condition.op == op)
            {
                return true;
            }
        }
        return false;
    }
}

TEST(GameplayAnimationBridge, JumpPresentationFollowsPhysicalMovementState)
{
    rendern::AnimationControllerAsset asset{};
    asset.parameters = {
        BoolParameter("IsGrounded"),
        BoolParameter("IsFalling"),
        BoolParameter("IsJumping")
    };

    rendern::AnimationControllerRuntime controller{};
    controller.stateMachineAsset = &asset;

    rendern::GameplayCharacterMovementStateComponent movementState{};
    movementState.grounded = false;
    movementState.falling = false;
    movementState.jumping = true;
    rendern::WriteGameplayMovementAnimationParameters(controller, movementState);

    const auto* grounded = rendern::FindAnimationParameter(controller.parameters, "IsGrounded");
    const auto* falling = rendern::FindAnimationParameter(controller.parameters, "IsFalling");
    const auto* jumping = rendern::FindAnimationParameter(controller.parameters, "IsJumping");
    ASSERT_NE(grounded, nullptr);
    ASSERT_NE(falling, nullptr);
    ASSERT_NE(jumping, nullptr);
    EXPECT_FALSE(grounded->boolValue);
    EXPECT_FALSE(falling->boolValue);
    EXPECT_TRUE(jumping->boolValue);

    movementState.grounded = true;
    movementState.falling = false;
    movementState.jumping = false;
    rendern::WriteGameplayMovementAnimationParameters(controller, movementState);

    EXPECT_TRUE(grounded->boolValue);
    EXPECT_FALSE(falling->boolValue);
    EXPECT_FALSE(jumping->boolValue);
}

TEST(GameplayAnimationBridge, ActionStateDoesNotOverridePhysicalJumpPresentation)
{
    rendern::AnimationControllerAsset asset{};
    asset.parameters = { BoolParameter("IsJumping") };

    rendern::AnimationControllerRuntime controller{};
    controller.stateMachineAsset = &asset;

    rendern::GameplayCharacterMovementStateComponent movementState{};
    movementState.grounded = true;
    movementState.jumping = false;
    rendern::WriteGameplayMovementAnimationParameters(controller, movementState);

    rendern::GameplayActionComponent action{};
    action.busy = true;
    action.current = rendern::kGameplayActionJump;
    rendern::WriteGameplayActionAnimationParameters(controller, action, rendern::MakeDefaultGameplayActionAnimationBindings());

    ASSERT_NE(rendern::FindAnimationParameter(controller.parameters, "IsJumping"), nullptr);
    EXPECT_FALSE(rendern::FindAnimationParameter(controller.parameters, "IsJumping")->boolValue);
}

TEST(GameplayAnimationBridge, ProductionControllerUsesPhysicalJumpLifecycle)
{
    const rendern::LevelAsset level = rendern::LoadLevelAssetFromJson(
        "levels/demo.level.with_fsm_test.locomotion.phaseB.json");
    ASSERT_TRUE(level.animationControllers.contains("fsm_test_locomotion_action"));
    const rendern::AnimationControllerAsset& controller =
        level.animationControllers.at("fsm_test_locomotion_action");

    const auto parameter = std::find_if(
        controller.parameters.begin(),
        controller.parameters.end(),
        [](const rendern::AnimationParameterDesc& desc)
        {
            return desc.name == "IsJumping";
        });
    ASSERT_NE(parameter, controller.parameters.end());
    EXPECT_EQ(parameter->defaultValue.type, rendern::AnimationParameterType::Bool);

    std::size_t jumpEntryCount = 0;
    std::size_t jumpExitCount = 0;
    bool hasRunningLanding = false;
    for (const rendern::AnimationTransitionDesc& transition : controller.transitions)
    {
        if (transition.toState == "Jump")
        {
            ++jumpEntryCount;
            EXPECT_TRUE(HasCondition(
                transition, "IsJumping", rendern::AnimationConditionOp::IfTrue));
            EXPECT_FALSE(HasCondition(
                transition, "Jump", rendern::AnimationConditionOp::Triggered));
        }
        if (transition.fromState == "Jump")
        {
            ++jumpExitCount;
            EXPECT_TRUE(HasCondition(
                transition, "IsJumping", rendern::AnimationConditionOp::IfFalse));
            EXPECT_FALSE(HasCondition(
                transition, "ActionBusy", rendern::AnimationConditionOp::IfFalse));
            if (transition.toState == "LocomotionRun")
            {
                hasRunningLanding =
                    HasCondition(transition, "IsMoving", rendern::AnimationConditionOp::IfTrue) &&
                    HasCondition(transition, "IsRunning", rendern::AnimationConditionOp::IfTrue);
            }
        }
    }

    EXPECT_EQ(jumpEntryCount, 8u);
    EXPECT_EQ(jumpExitCount, 3u);
    EXPECT_TRUE(hasRunningLanding);
}
TEST(GameplayAnimationBridge, UsesExplicitPresentationBindingForArbitraryAction)
{
    rendern::AnimationControllerAsset asset{};
    asset.parameters = { rendern::AnimationParameterDesc{
        .name = "PunchingAttack",
        .defaultValue = { .type = rendern::AnimationParameterType::Trigger }
    } };
    rendern::AnimationControllerRuntime controller{};
    controller.stateMachineAsset = &asset;
    rendern::GameplayActionComponent action{};
    action.pending = { rendern::GameplayActionId{ "Combat.PunchingAttack" },
        rendern::GameplayActionRequestSource::Combat, 10 };
    const rendern::GameplayActionAnimationBindings bindings{
        { rendern::GameplayActionId{ "Combat.PunchingAttack" }, "PunchingAttack" }
    };
    rendern::WriteGameplayActionAnimationParameters(controller, action, bindings);
    const auto* parameter = rendern::FindAnimationParameter(controller.parameters, "PunchingAttack");
    ASSERT_NE(parameter, nullptr);
    EXPECT_TRUE(parameter->triggerValue);
}
