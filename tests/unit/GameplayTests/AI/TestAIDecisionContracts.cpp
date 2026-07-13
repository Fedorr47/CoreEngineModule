#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

import core;

using namespace rendern;

namespace
{
    static_assert(!std::is_same_v<AIGoalId, AIActionId>);
    static_assert(!std::is_convertible_v<AIGoalId, AIActionId>);
    static_assert(!std::is_convertible_v<AIActionId, AIGoalId>);
    static_assert(!std::is_assignable_v<AIGoalId&, AIActionId>);
    static_assert(!std::is_assignable_v<AIActionId&, AIGoalId>);
}

// Protects the type boundary between goals and executable actions so later
// planners and runtimes cannot accidentally exchange unrelated identifiers,
// preventing regressions where a goal handle is treated as an action handle.
TEST(
    AIDecisionContracts,
    GoalAndActionIdsUseIndependentStrongTypes)
{
    constexpr AIGoalId defaultGoalId{};
    constexpr AIActionId defaultActionId{};
    constexpr AIGoalId firstGoalId{ 7u };
    constexpr AIGoalId matchingFirstGoalId{ 7u };
    constexpr AIGoalId secondGoalId{ 8u };
    constexpr AIActionId firstActionId{ 11u };
    constexpr AIActionId matchingFirstActionId{ 11u };
    constexpr AIActionId secondActionId{ 12u };

    EXPECT_FALSE(defaultGoalId.IsValid());
    EXPECT_FALSE(defaultActionId.IsValid());
    EXPECT_TRUE(firstGoalId.IsValid());
    EXPECT_TRUE(firstActionId.IsValid());
    EXPECT_EQ(firstGoalId, matchingFirstGoalId);
    EXPECT_NE(firstGoalId, secondGoalId);
    EXPECT_EQ(firstActionId, matchingFirstActionId);
    EXPECT_NE(firstActionId, secondActionId);
}

// Protects default value-type construction so sparse contract objects do not
// accidentally target valid fact or action slots before an author assigns IDs,
// preventing later catalogs from treating placeholder definitions as real data.
TEST(
    AIDecisionContracts,
    DefaultContractIdsAreInvalid)
{
    const AIFactCondition defaultCondition{};
    const AIFactEffect defaultEffect{};
    const AIPlanStep defaultStep{};
    const AIPlan defaultPlan{};

    EXPECT_EQ(
        defaultCondition.factId.index,
        AIWorldFactId::InvalidIndex);
    EXPECT_EQ(
        defaultEffect.factId.index,
        AIWorldFactId::InvalidIndex);
    EXPECT_FALSE(defaultStep.actionId.IsValid());
    EXPECT_FALSE(defaultPlan.goalId.IsValid());
    EXPECT_TRUE(defaultPlan.steps.empty());
}

// Protects the all-of condition contract where an action without
// preconditions remains applicable to any valid world-state snapshot,
// preventing later planners from rejecting empty sparse condition lists.
TEST(
    AIDecisionContracts,
    EmptyConditionSetIsSatisfied)
{
    const AIAgentWorldState worldState{};
    const std::vector<AIFactCondition> emptyConditions{};

    EXPECT_TRUE(AreFactConditionsSatisfied(worldState, emptyConditions));
}

// Protects sparse condition semantics so irrelevant facts remain ignored
// while required true and required false values are evaluated explicitly,
// preventing regressions that confuse unset facts with don't-care facts.
TEST(
    AIDecisionContracts,
    ConditionsMatchExpectedTrueAndFalseValues)
{
    AIAgentWorldState worldState{};
    constexpr AIWorldFactId firstFact{ 1u };
    constexpr AIWorldFactId secondFact{ 2u };

    worldState.SetFact(firstFact);

    const std::array matchingConditions{
        AIFactCondition{ firstFact, true },
        AIFactCondition{ secondFact, false }
    };
    const std::array firstMismatch{
        AIFactCondition{ firstFact, false }
    };
    const std::array secondMismatch{
        AIFactCondition{ secondFact, true }
    };

    EXPECT_TRUE(
        AreFactConditionsSatisfied(worldState, matchingConditions));
    EXPECT_FALSE(
        AreFactConditionsSatisfied(worldState, firstMismatch));
    EXPECT_FALSE(
        AreFactConditionsSatisfied(worldState, secondMismatch));
}

// Protects planner simulation boundaries by ensuring condition checks are
// pure reads and cannot change the source agent snapshot, preventing future
// planning queries from corrupting facts produced by independent systems.
TEST(
    AIDecisionContracts,
    ConditionEvaluationDoesNotMutateWorldState)
{
    AIAgentWorldState worldState{};
    constexpr AIWorldFactId requiredFact{ 4u };
    constexpr AIWorldFactId unrelatedFact{ 5u };

    worldState.SetFact(requiredFact);

    const std::array matchingConditions{
        AIFactCondition{ requiredFact, true }
    };
    const std::array nonMatchingConditions{
        AIFactCondition{ unrelatedFact, true }
    };

    EXPECT_TRUE(
        AreFactConditionsSatisfied(worldState, matchingConditions));
    EXPECT_FALSE(
        AreFactConditionsSatisfied(worldState, nonMatchingConditions));
    EXPECT_TRUE(worldState.IsFactSet(requiredFact));
    EXPECT_FALSE(worldState.IsFactSet(unrelatedFact));
}

// Protects deterministic state simulation so an action effect changes only
// explicitly declared facts and preserves unrelated planner knowledge,
// preventing future simulated actions from clearing facts they do not own.
TEST(
    AIDecisionContracts,
    EffectsModifyOnlyReferencedFacts)
{
    AIAgentWorldState worldState{};
    constexpr AIWorldFactId firstFact{ 6u };
    constexpr AIWorldFactId secondFact{ 7u };
    constexpr AIWorldFactId unrelatedFact{ 8u };

    worldState.SetFact(secondFact);
    worldState.SetFact(unrelatedFact);

    const std::array effects{
        AIFactEffect{ firstFact, true },
        AIFactEffect{ secondFact, false }
    };

    ApplyFactEffects(worldState, effects);

    EXPECT_TRUE(worldState.IsFactSet(firstFact));
    EXPECT_FALSE(worldState.IsFactSet(secondFact));
    EXPECT_TRUE(worldState.IsFactSet(unrelatedFact));
}

// Protects the passive-definition boundary that later catalogs and planners
// consume without embedding runtime execution behavior, preventing authored
// sparse fact contracts from being reordered or replaced by runtime state.
TEST(
    AIDecisionContracts,
    GoalAndActionDefinitionsOwnSparseFactContracts)
{
    constexpr AIGoalId goalId{ 2u };
    constexpr AIActionId actionId{ 3u };
    constexpr AIWorldFactId firstFact{ 9u };
    constexpr AIWorldFactId secondFact{ 10u };
    constexpr AIWorldFactId requiredFact{ 11u };

    const AIGoalDefinition goalDefinition{
        goalId,
        {
            AIFactCondition{ firstFact, true },
            AIFactCondition{ secondFact, false }
        }
    };
    const AIActionDefinition actionDefinition{
        actionId,
        {
            AIFactCondition{ requiredFact, true },
            AIFactCondition{ firstFact, false }
        },
        {
            AIFactEffect{ secondFact, true },
            AIFactEffect{ requiredFact, false }
        },
        2.5f
    };

    ASSERT_EQ(goalDefinition.goalId, goalId);
    ASSERT_EQ(goalDefinition.desiredFacts.size(), 2u);
    EXPECT_EQ(
        goalDefinition.desiredFacts[0],
        (AIFactCondition{ firstFact, true }));
    EXPECT_EQ(
        goalDefinition.desiredFacts[1],
        (AIFactCondition{ secondFact, false }));

    ASSERT_EQ(actionDefinition.actionId, actionId);
    ASSERT_EQ(actionDefinition.preconditions.size(), 2u);
    ASSERT_EQ(actionDefinition.effects.size(), 2u);
    EXPECT_EQ(
        actionDefinition.preconditions[0],
        (AIFactCondition{ requiredFact, true }));
    EXPECT_EQ(
        actionDefinition.preconditions[1],
        (AIFactCondition{ firstFact, false }));
    EXPECT_EQ(
        actionDefinition.effects[0],
        (AIFactEffect{ secondFact, true }));
    EXPECT_EQ(
        actionDefinition.effects[1],
        (AIFactEffect{ requiredFact, false }));
    EXPECT_FLOAT_EQ(actionDefinition.baseCost, 2.5f);
}

// Protects plan ordering so a future executor observes the exact action
// sequence produced by a deterministic planner, preventing regressions that
// drop the goal link or reorder owned steps during storage.
TEST(
    AIDecisionContracts,
    PlanPreservesOrderedActionSequence)
{
    constexpr AIGoalId goalId{ 4u };
    constexpr AIActionId firstActionId{ 21u };
    constexpr AIActionId secondActionId{ 22u };
    constexpr AIActionId thirdActionId{ 23u };

    const AIPlan defaultPlan{};
    const AIPlan plan{
        goalId,
        {
            AIPlanStep{ firstActionId },
            AIPlanStep{ secondActionId },
            AIPlanStep{ thirdActionId }
        }
    };

    EXPECT_TRUE(defaultPlan.steps.empty());
    ASSERT_EQ(plan.goalId, goalId);
    ASSERT_EQ(plan.steps.size(), 3u);
    EXPECT_EQ(plan.steps[0].actionId, firstActionId);
    EXPECT_EQ(plan.steps[1].actionId, secondActionId);
    EXPECT_EQ(plan.steps[2].actionId, thirdActionId);
}