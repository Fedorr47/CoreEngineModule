#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

import core;

using namespace rendern;

TEST(AIPlanner, PreservesContextForRepeatedSemanticAction)
{
    constexpr AIWorldFactId hasKey{40u};
    constexpr AIWorldFactId atGoal{41u};
    constexpr AIActionContextId keyTarget{1u};
    constexpr AIActionContextId finalTarget{2u};

    AIAgentWorldState initial{};

    const AIGoalDefinition goal{
        AIGoalId{50u},
        {
            {atGoal, true}
        }
    };

    const std::array actions{
        AIActionDefinition{
            .actionId = kAIMoveToActionId,
            .preconditions = {
                {hasKey, false}
            },
            .effects = {
                {hasKey, true}
            },
            .contextId = keyTarget,
            .baseCost = 1.0f
        },
        AIActionDefinition{
            .actionId = kAIMoveToActionId,
            .preconditions = {
                {hasKey, true}
            },
            .effects = {
                {atGoal, true}
            },
            .contextId = finalTarget,
            .baseCost = 1.0f
        }
    };

    const auto plan = FindAIPlan(initial, goal, actions);

    ASSERT_TRUE(plan);
    ASSERT_EQ(plan->steps.size(), 2u);

    EXPECT_EQ(
        plan->steps[0],
        (AIPlanStep{kAIMoveToActionId, keyTarget}));

    EXPECT_EQ(
        plan->steps[1],
        (AIPlanStep{kAIMoveToActionId, finalTarget}));

    EXPECT_FALSE(initial.IsFactSet(hasKey));
    EXPECT_FALSE(initial.IsFactSet(atGoal));
}

namespace
{
    constexpr AIWorldFactId Fact(const std::uint16_t value)
    {
        return AIWorldFactId{value};
    }

    AIGoalDefinition Goal(
        const std::uint16_t id,
        const AIWorldFactId fact)
    {
        return {
            AIGoalId{id},
            {
                AIFactCondition{fact, true}
            }
        };
    }

    AIActionDefinition Action(
        const std::uint16_t id,
        std::vector<AIFactCondition> preconditions,
        std::vector<AIFactEffect> effects,
        const float cost = 1.0f)
    {
        return {
            .actionId = AIActionId{id},
            .preconditions = std::move(preconditions),
            .effects = std::move(effects),
            .contextId = {},
            .baseCost = cost
        };
    }
}

TEST(AIPlanner, AlreadySatisfiedGoalReturnsEmptyPlanWithoutMutatingState)
{
    AIAgentWorldState state{};
    state.SetFact(Fact(0));
    const AIAgentWorldState original = state;
    const auto plan = FindAIPlan(state, Goal(1, Fact(0)), {});
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->goalId, AIGoalId{ 1 });
    EXPECT_TRUE(plan->steps.empty());
    EXPECT_EQ(state, original);
}

TEST(AIPlanner, BuildsOrderedPrerequisiteChainAndPreservesUnrelatedFacts)
{
    AIAgentWorldState state{};
    state.SetFact(Fact(7));

    const std::array actions{
        Action(
            20,
            {
                {Fact(1), true},
                {Fact(7), true}
            },
            {
                {Fact(2), true}
            }),
        Action(
            10,
            {
                {Fact(0), false}
            },
            {
                {Fact(1), true}
            })
    };

    const auto plan = FindAIPlan(
        state,
        Goal(1, Fact(2)),
        actions);

    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 2u);

    EXPECT_EQ(
        plan->steps[0].actionId,
        AIActionId{10});

    EXPECT_EQ(
        plan->steps[1].actionId,
        AIActionId{20});

    EXPECT_TRUE(state.IsFactSet(Fact(7)));
    EXPECT_FALSE(state.IsFactSet(Fact(1)));
}

TEST(AIPlanner, UnreachableGoalAndInapplicableActionReturnNoPlan)
{
    const std::array actions{
        Action(1, { { Fact(0), true } }, { { Fact(1), true } })
    };
    EXPECT_FALSE(FindAIPlan({}, Goal(1, Fact(1)), actions).has_value());
}

TEST(AIPlanner, CheapestRouteIncludingCheaperRevisitWins)
{
    const std::array actions{
        Action(40, { { Fact(2), true } }, { { Fact(3), true } }, 1.0f),
        Action(20, { { Fact(1), true } },
            { { Fact(1), false }, { Fact(2), true } }, 1.0f),
        Action(10, {}, { { Fact(1), true } }, 1.0f),
        Action(5, {}, { { Fact(2), true } }, 6.0f)
    };
    const auto plan = FindAIPlan({}, Goal(1, Fact(3)), actions);
    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 3u);
    EXPECT_EQ(plan->steps[0].actionId, AIActionId{ 10 });
    EXPECT_EQ(plan->steps[1].actionId, AIActionId{ 20 });
    EXPECT_EQ(plan->steps[2].actionId, AIActionId{ 40 });
}

TEST(AIPlanner, FalseEffectCanSatisfyFalseGoalCondition)
{
    AIAgentWorldState state{};
    state.SetFact(Fact(0));
    const AIGoalDefinition goal{
        AIGoalId{ 1 }, { AIFactCondition{ Fact(0), false } } };
    const std::array actions{
        Action(7, { { Fact(0), true } }, { { Fact(0), false } })
    };

    const auto plan = FindAIPlan(state, goal, actions);
    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 1u);
    EXPECT_EQ(plan->steps[0].actionId, AIActionId{ 7 });
    EXPECT_TRUE(state.IsFactSet(Fact(0)));
}

TEST(AIPlanner, ZeroCostCycleTerminatesAndAllowsExit)
{
    const std::array actions{
        Action(1, {}, { { Fact(0), true } }, 0.0f),
        Action(2, { { Fact(0), true } }, { { Fact(0), false } }, 0.0f),
        Action(3, { { Fact(0), true } }, { { Fact(1), true } }, 0.0f)
    };
    const auto plan = FindAIPlan({}, Goal(1, Fact(1)), actions);
    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 2u);
    EXPECT_EQ(plan->steps[0].actionId, AIActionId{ 1 });
    EXPECT_EQ(plan->steps[1].actionId, AIActionId{ 3 });

    EXPECT_FALSE(FindAIPlan({}, Goal(1, Fact(9)), actions).has_value());
}

TEST(AIPlanner, EqualCostChoiceIsDeterministicByActionId)
{
    const std::array actions{
        Action(9, {}, { { Fact(0), true } }),
        Action(3, {}, { { Fact(0), true } })
    };
    for (int run = 0; run < 10; run)
    {
        const auto plan = FindAIPlan({}, Goal(1, Fact(0)), actions);
        ASSERT_TRUE(plan.has_value());
        ASSERT_EQ(plan->steps.size(), 1u);
        EXPECT_EQ(plan->steps[0].actionId, AIActionId{ 3 });
    }
}

TEST(AIPlanner, RejectsMalformedDefinitionsAndCosts)
{
    const auto goal = Goal(1, Fact(0));
    EXPECT_FALSE(FindAIPlan({}, AIGoalDefinition{}, {}).has_value());
    EXPECT_FALSE(FindAIPlan({}, { AIGoalId{ 1 }, { { AIWorldFactId{}, true } } }, {}).has_value());

    const std::array invalidId{ Action(AIActionId::InvalidValue, {}, { { Fact(0), true } }) };
    const std::array duplicateIds{
        Action(1, {}, { { Fact(0), true } }), Action(1, {}, { { Fact(1), true } }) };
    const std::array negative{ Action(1, {}, { { Fact(0), true } }, -1.0f) };
    const std::array nanCost{ Action(1, {}, { { Fact(0), true } }, std::numeric_limits<float>::quiet_NaN()) };
    const std::array infinite{ Action(1, {}, { { Fact(0), true } }, std::numeric_limits<float>::infinity()) };
    EXPECT_FALSE(FindAIPlan({}, goal, invalidId).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, duplicateIds).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, negative).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, nanCost).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, infinite).has_value());
}

// Protects integer world-state identity in the planner. The repeated collect
// action changes no boolean facts, so reaching the threshold requires the
// planner to distinguish otherwise-identical states by the Coins value alone.
TEST(AIPlanner, IntegerValueParticipatesInPlannerStateIdentity)
{
    constexpr AIWorldIntegerFactId coins{ 0u };
   
    const std::array actions{
        AIActionDefinition{
            .actionId = AIActionId{ 1 },
            .numericPreconditions = {
                                { coins, AINumericConditionOperator::Less, 3 }
            },
             .numericEffects = {
                                                    { coins, AINumericEffectOperation::Add, 1 }
             }
        },
        AIActionDefinition{
            .actionId = AIActionId{ 2 },
            .effects = {
                                                    { Fact(20), true }
            },
            .numericPreconditions = {
                                                        { coins, AINumericConditionOperator::GreaterOrEqual, 3 }
            }
        }
    };

    const auto plan = FindAIPlan({}, Goal(1, Fact(20)), actions);

    ASSERT_TRUE(plan.has_value());
    ASSERT_EQ(plan->steps.size(), 4u);
    EXPECT_EQ(plan->steps[0].actionId, AIActionId{ 1 });
    EXPECT_EQ(plan->steps[1].actionId, AIActionId{ 1 });
    EXPECT_EQ(plan->steps[2].actionId, AIActionId{ 1 });
    EXPECT_EQ(plan->steps[3].actionId, AIActionId{ 2 });
}

TEST(AIPlanner, RejectsMalformedNumericDefinitions)
{
    const auto goal = Goal(1, Fact(0));
    const std::array invalidConditionFact{ AIActionDefinition{
        .actionId = AIActionId{ 1 },
        .numericPreconditions = { { {}, AINumericConditionOperator::Equal, 0 } }
    } };
    const std::array invalidConditionOperator{ AIActionDefinition{
        .actionId = AIActionId{ 1 },
        .numericPreconditions = {
                            {
                                AIWorldIntegerFactId{ 0 },
                                static_cast<AINumericConditionOperator>(255),
                                0
                            }
        }
    } };
    
    const std::array invalidEffectFact{ AIActionDefinition{
        .actionId = AIActionId{ 1 },
        .numericEffects = {
                            { {}, AINumericEffectOperation::Set, 0 }
        }
    } };
    
    const std::array invalidEffectOperator{ AIActionDefinition{
        .numericEffects = {
                                {
                                    AIWorldIntegerFactId{ 0 },
                                    static_cast<AINumericEffectOperation>(255),
                                    0
                                }
        }
    } };

    EXPECT_FALSE(FindAIPlan({}, goal, invalidConditionFact).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, invalidConditionOperator).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, invalidEffectFact).has_value());
    EXPECT_FALSE(FindAIPlan({}, goal, invalidEffectOperator).has_value());
}