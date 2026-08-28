#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>
#include <vector>

import core;

namespace
{
    rendern::GameplayGOAPDefinitionAsset MakeDefinition()
    {
        rendern::GameplayGOAPDefinitionAsset asset{};
        asset.id = "compiler_test";
        asset.source = "compiler-test";

        asset.facts = {
            {"enabled", rendern::GameplayGOAPFactType::Boolean},
            {"resource", rendern::GameplayGOAPFactType::Integer}
        };

        asset.goals = {
            rendern::GameplayGOAPAuthoredGoal{
                .name = "reach_target",
                .score = 10.0f,
                .facts = {
                    {"enabled", true}
                }}
        };

        asset.actions = {
            rendern::GameplayGOAPAuthoredAction{
                .action = "move_to",
                .context = "target_a",
                .cost = 3.0f,
                .preconditions = {
                    {"enabled", false}
                },
                .effects = {
                    {"enabled", true}
                },
                .numericPreconditions = {
                    {"resource", ">=", 2}
                },
                .numericEffects = {
                    {"resource", "add", -1}
                }}
        };

        return asset;
    }

    template <typename TCallable>
    void ExpectRuntimeError(TCallable&& callable, const std::string_view expectedMessage)
    {
        try
        {
            callable();
            FAIL() << "Expected std::runtime_error";
        }
        catch (const std::runtime_error& error)
        {
            EXPECT_EQ(std::string_view{error.what()}, expectedMessage);
        }
    }
}

TEST(GameplayGOAPDefinitionCompiler, CompilePreservesDeterministicIdsMetadataAndLookups)
{
    const rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}}
    };

    const rendern::GameplayGOAPCompiledDefinition compiled =
        rendern::CompileGameplayGOAPDefinition(asset, semanticActions);

    ASSERT_TRUE(compiled.FindBooleanFact("enabled").has_value());
    EXPECT_EQ(*compiled.FindBooleanFact("enabled"), rendern::AIWorldFactId{0u});

    ASSERT_TRUE(compiled.FindIntegerFact("resource").has_value());
    EXPECT_EQ(*compiled.FindIntegerFact("resource"), rendern::AIWorldIntegerFactId{0u});

    ASSERT_TRUE(compiled.FindActionContext("target_a").has_value());
    EXPECT_EQ(*compiled.FindActionContext("target_a"), rendern::AIActionContextId{0u});

    EXPECT_FALSE(compiled.FindBooleanFact("missing").has_value());
    EXPECT_FALSE(compiled.FindIntegerFact("missing").has_value());
    EXPECT_FALSE(compiled.FindActionContext("missing").has_value());

    ASSERT_EQ(compiled.definition.goals.size(), 1u);
    const rendern::AIGoalSelectionCandidate& goal = compiled.definition.goals[0];
    EXPECT_EQ(goal.goal.goalId, rendern::AIGoalId{0u});
    EXPECT_FLOAT_EQ(goal.baseScore, 10.0f);
    ASSERT_EQ(goal.goal.desiredFacts.size(), 1u);
    EXPECT_EQ(goal.goal.desiredFacts[0].factId, rendern::AIWorldFactId{0u});
    EXPECT_TRUE(goal.goal.desiredFacts[0].bExpectedValue);

    ASSERT_EQ(compiled.definition.actions.size(), 1u);
    const rendern::AIActionDefinition& action = compiled.definition.actions[0];
    EXPECT_EQ(action.actionId, rendern::AIActionId{7u});
    EXPECT_EQ(action.contextId, rendern::AIActionContextId{0u});
    EXPECT_FLOAT_EQ(action.baseCost, 3.0f);

    ASSERT_EQ(action.preconditions.size(), 1u);
    EXPECT_EQ(action.preconditions[0].factId, rendern::AIWorldFactId{0u});
    EXPECT_FALSE(action.preconditions[0].bExpectedValue);

    ASSERT_EQ(action.effects.size(), 1u);
    EXPECT_EQ(action.effects[0].factId, rendern::AIWorldFactId{0u});
    EXPECT_TRUE(action.effects[0].bValue);

    ASSERT_EQ(action.numericPreconditions.size(), 1u);
    EXPECT_EQ(action.numericPreconditions[0].factId, rendern::AIWorldIntegerFactId{0u});
    EXPECT_EQ(
        action.numericPreconditions[0].comparison,
        rendern::AINumericConditionOperator::GreaterOrEqual);
    EXPECT_EQ(action.numericPreconditions[0].value, 2);

    ASSERT_EQ(action.numericEffects.size(), 1u);
    EXPECT_EQ(action.numericEffects[0].factId, rendern::AIWorldIntegerFactId{0u});
    EXPECT_EQ(
        action.numericEffects[0].operation,
        rendern::AINumericEffectOperation::Add);
    EXPECT_EQ(action.numericEffects[0].value, -1);

    const auto& metadata = compiled.definition.metadata;

    ASSERT_EQ(metadata.booleanFacts.size(), 1u);
    EXPECT_EQ(metadata.booleanFacts[0].id, rendern::AIWorldFactId{0u});
    EXPECT_EQ(metadata.booleanFacts[0].name, "enabled");

    ASSERT_EQ(metadata.integerFacts.size(), 1u);
    EXPECT_EQ(metadata.integerFacts[0].id, rendern::AIWorldIntegerFactId{0u});
    EXPECT_EQ(metadata.integerFacts[0].name, "resource");

    ASSERT_EQ(metadata.goals.size(), 1u);
    EXPECT_EQ(metadata.goals[0].id, rendern::AIGoalId{0u});
    EXPECT_EQ(metadata.goals[0].name, "reach_target");

    ASSERT_EQ(metadata.actions.size(), 1u);
    EXPECT_EQ(metadata.actions[0].actionId, rendern::AIActionId{7u});
    EXPECT_EQ(metadata.actions[0].contextId, rendern::AIActionContextId{0u});
    EXPECT_EQ(metadata.actions[0].actionName, "move_to");
    EXPECT_EQ(metadata.actions[0].contextName, "target_a");
}

TEST(GameplayGOAPDefinitionCompiler, CostOverrideChangesOnlyMatchingAction)
{
    rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    asset.actions.push_back(
        rendern::GameplayGOAPAuthoredAction{
            .action = "move_to",
            .context = "target_b",
            .cost = 5.0f});

    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}}
    };

    const std::vector overrides{
        rendern::GameplayGOAPActionCostOverride{
            .action = "move_to",
            .context = "target_b",
            .cost = 0.75f}
    };

    const rendern::GameplayGOAPCompiledDefinition compiled =
        rendern::CompileGameplayGOAPDefinition(asset, semanticActions, overrides);

    ASSERT_EQ(compiled.definition.actions.size(), 2u);
    EXPECT_FLOAT_EQ(compiled.definition.actions[0].baseCost, 3.0f);
    EXPECT_FLOAT_EQ(compiled.definition.actions[1].baseCost, 0.75f);

    EXPECT_EQ(compiled.definition.actions[0].contextId, rendern::AIActionContextId{0u});
    EXPECT_EQ(compiled.definition.actions[1].contextId, rendern::AIActionContextId{1u});
}

TEST(GameplayGOAPDefinitionCompiler, RejectsUnknownSemanticActionWithStableDiagnostic)
{
    rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    asset.actions[0].action = "missing_action";

    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}}
    };

    ExpectRuntimeError(
        [&]
        {
            (void)rendern::CompileGameplayGOAPDefinition(asset, semanticActions);
        },
        "GOAP definition 'compiler-test', actions 'target_a': "
        "unknown semantic action 'missing_action'");
}

TEST(GameplayGOAPDefinitionCompiler, RejectsBooleanUseOfIntegerFactWithStableDiagnostic)
{
    rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    asset.goals[0].facts = {
        {"resource", true}
    };

    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}}
    };

    ExpectRuntimeError(
        [&]
        {
            (void)rendern::CompileGameplayGOAPDefinition(asset, semanticActions);
        },
        "GOAP definition 'compiler-test', goals 'reach_target': "
        "fact 'resource' has wrong type; expected bool");
}

TEST(GameplayGOAPDefinitionCompiler, RejectsDuplicateSemanticActionName)
{
    const rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}},
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{8u}}
    };

    ExpectRuntimeError(
        [&]
        {
            (void)rendern::CompileGameplayGOAPDefinition(asset, semanticActions);
        },
        "GOAP definition 'compiler-test', semanticActions 'move_to': "
        "duplicate semantic action name");
}

TEST(GameplayGOAPDefinitionCompiler, RejectsDuplicateSemanticActionId)
{
    const rendern::GameplayGOAPDefinitionAsset asset = MakeDefinition();
    const std::vector semanticActions{
        rendern::GameplayGOAPSemanticAction{
            .name = "move_to",
            .actionId = rendern::AIActionId{7u}},
        rendern::GameplayGOAPSemanticAction{
            .name = "buy_key",
            .actionId = rendern::AIActionId{7u}}
    };

    ExpectRuntimeError(
        [&]
        {
            (void)rendern::CompileGameplayGOAPDefinition(asset, semanticActions);
        },
        "GOAP definition 'compiler-test', semanticActions 'buy_key': "
        "duplicate semantic action id");
}
