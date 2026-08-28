#include <gtest/gtest.h>

#include <limits>
#include <vector>

import core;

using namespace rendern;

namespace
{
    constexpr AIWorldFactId kGoalA{0u};
    constexpr AIWorldFactId kGoalB{1u};
    constexpr AIWorldFactId kModifier{2u};

    AIGoalSelectionCandidate Candidate(
        const AIGoalId id,
        const AIWorldFactId desiredFact,
        const float baseScore)
    {
        return AIGoalSelectionCandidate{
            .goal = AIGoalDefinition{
                .goalId = id,
                .desiredFacts = {AIFactCondition{desiredFact, true}}},
            .baseScore = baseScore};
    }
}

TEST(AIGoalSelection, EmptyCandidateListProducesNoGoal)
{
    const AIAgentWorldState state{};
    const std::vector<AIGoalSelectionCandidate> candidates{};

    EXPECT_FALSE(SelectAIGoal(state, candidates).IsValid());
}

TEST(AIGoalSelection, SingleEligibleCandidateIsSelectedWithItsBaseScore)
{
    const AIAgentWorldState state{};
    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 2.5f)};

    const AIGoalSelectionResult result = SelectAIGoal(state, candidates);

    EXPECT_EQ(result.goalId, AIGoalId{1u});
    EXPECT_FLOAT_EQ(result.score, 2.5f);
    EXPECT_FLOAT_EQ(ScoreAIGoal(state, candidates.front()), 2.5f);
}

TEST(AIGoalSelection, HighestBaseScoreWins)
{
    const AIAgentWorldState state{};
    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 1.0f),
        Candidate(AIGoalId{2u}, kGoalB, 3.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, SatisfiedFactModifierChangesWinningGoal)
{
    AIAgentWorldState state{};
    auto first = Candidate(AIGoalId{1u}, kGoalA, 2.0f);
    auto second = Candidate(AIGoalId{2u}, kGoalB, 1.0f);
    second.scoreRules.push_back(
        AIGoalScoreRule{AIFactCondition{kModifier, true}, 2.0f});

    const std::vector candidates{first, second};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{1u});

    state.SetFact(kModifier);

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, ExpectedFalseConditionContributesWhenFactIsUnset)
{
    const AIAgentWorldState state{};
    auto candidate = Candidate(AIGoalId{1u}, kGoalA, 1.0f);
    candidate.scoreRules.push_back(
        AIGoalScoreRule{AIFactCondition{kModifier, false}, 2.0f});

    EXPECT_FLOAT_EQ(ScoreAIGoal(state, candidate), 3.0f);
}

TEST(AIGoalSelection, NegativeModifierCanLowerCandidateBelowCompetitor)
{
    AIAgentWorldState state{};
    state.SetFact(kModifier);

    auto first = Candidate(AIGoalId{1u}, kGoalA, 3.0f);
    first.scoreRules.push_back(
        AIGoalScoreRule{AIFactCondition{kModifier, true}, -2.0f});

    const std::vector candidates{
        first,
        Candidate(AIGoalId{2u}, kGoalB, 2.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, EqualScoresPreserveFirstEligibleCandidate)
{
    const AIAgentWorldState state{};
    const std::vector candidates{
        Candidate(AIGoalId{2u}, kGoalB, 2.0f),
        Candidate(AIGoalId{1u}, kGoalA, 2.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, RepeatedEvaluationIsStableAndDoesNotMutateWorldState)
{
    AIAgentWorldState state{};
    state.SetFact(kModifier);

    const AIAgentWorldState original = state;
    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 1.0f)};

    const AIGoalSelectionResult first = SelectAIGoal(state, candidates);
    const AIGoalSelectionResult second = SelectAIGoal(state, candidates);

    EXPECT_EQ(first.goalId, second.goalId);
    EXPECT_FLOAT_EQ(first.score, second.score);
    EXPECT_EQ(state, original);
}

TEST(AIGoalSelection, InvalidCandidatesCannotBecomeSelectedGoals)
{
    AIAgentWorldState state{};
    auto invalidId = Candidate(AIGoalId{}, kGoalA, 100.0f);
    auto invalidFact = Candidate(AIGoalId{2u}, AIWorldFactId{}, 100.0f);
    auto invalidScore = Candidate(
        AIGoalId{3u},
        kGoalB,
        std::numeric_limits<float>::infinity());

    const std::vector candidates{
        invalidId,
        invalidFact,
        invalidScore,
        Candidate(AIGoalId{1u}, kGoalA, 1.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{1u});
}

TEST(AIGoalSelection, AlreadySatisfiedGoalIsSkipped)
{
    AIAgentWorldState state{};
    state.SetFact(kGoalA);

    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 10.0f),
        Candidate(AIGoalId{2u}, kGoalB, 1.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, GoalBecomesIneligibleWhenItBecomesSatisfied)
{
    AIAgentWorldState state{};
    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 10.0f),
        Candidate(AIGoalId{2u}, kGoalB, 1.0f)};

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{1u});

    state.SetFact(kGoalA);

    EXPECT_EQ(SelectAIGoal(state, candidates).goalId, AIGoalId{2u});
}

TEST(AIGoalSelection, AllSatisfiedGoalsProduceNoSelection)
{
    AIAgentWorldState state{};
    state.SetFact(kGoalA);
    state.SetFact(kGoalB);

    const std::vector candidates{
        Candidate(AIGoalId{1u}, kGoalA, 1.0f),
        Candidate(AIGoalId{2u}, kGoalB, 2.0f)};

    EXPECT_FALSE(SelectAIGoal(state, candidates).IsValid());
}

TEST(AIGoalSelection, EmptyDesiredFactsAreAlreadySatisfied)
{
    const AIAgentWorldState state{};
    const std::vector candidates{
        AIGoalSelectionCandidate{
            .goal = AIGoalDefinition{
                .goalId = AIGoalId{1u}},
            .baseScore = 10.0f}};

    EXPECT_FALSE(SelectAIGoal(state, candidates).IsValid());
}
