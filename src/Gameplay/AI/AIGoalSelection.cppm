module;

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

export module core:ai_goal_selection;

export import :ai_decision_contracts;

export namespace rendern
{
    struct AIGoalScoreRule
    {
        AIFactCondition condition{};
        float scoreDelta{};
    };

    struct AIGoalSelectionCandidate
    {
        AIGoalDefinition goal{};
        float baseScore{};
        std::vector<AIGoalScoreRule> scoreRules{};
    };

    struct AIGoalSelectionResult
    {
        AIGoalId goalId{};
        float score{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return goalId.IsValid();
        }
    };

    [[nodiscard]] float ScoreAIGoal(
        const AIAgentWorldState& worldState,
        const AIGoalSelectionCandidate& candidate) noexcept
    {
        float score = candidate.baseScore;
        for (const AIGoalScoreRule& rule : candidate.scoreRules)
        {
            if (rule.condition.factId.index < AIAgentWorldState::FactCapacity
                && std::isfinite(rule.scoreDelta)
                && AreFactConditionsSatisfied(worldState, std::span{&rule.condition, 1u}))
            {
                score += rule.scoreDelta;
            }
        }
        return score;
    }

    [[nodiscard]] AIGoalSelectionResult SelectAIGoal(
        const AIAgentWorldState& worldState,
        const std::span<const AIGoalSelectionCandidate> candidates) noexcept
    {
        AIGoalSelectionResult selectedGoal{};
        for (const AIGoalSelectionCandidate& candidate : candidates)
        {
            const bool bValidGoal = candidate.goal.goalId.IsValid()
                && std::ranges::all_of(candidate.goal.desiredFacts, [](const AIFactCondition& condition)
                {
                    return condition.factId.index < AIAgentWorldState::FactCapacity;
                });
            if (!bValidGoal || !std::isfinite(candidate.baseScore)
                || AreFactConditionsSatisfied(worldState, candidate.goal.desiredFacts))
            {
                continue;
            }

            const bool bValidRules = std::ranges::all_of(candidate.scoreRules, 
                [](const AIGoalScoreRule& rule)
                {
                    return rule.condition.factId.index < AIAgentWorldState::FactCapacity
                        && std::isfinite(rule.scoreDelta);
                });
            const float score = ScoreAIGoal(worldState, candidate);
            if (!bValidRules || !std::isfinite(score))
            {
                continue;
            }

            if (!selectedGoal.IsValid() || score > selectedGoal.score)
            {
                selectedGoal = AIGoalSelectionResult{
                    .goalId=candidate.goal.goalId,
                    .score=score};
            }
        }
        return selectedGoal;
    }
}