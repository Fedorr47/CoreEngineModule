module;

#include <vector>

export module core:ai_goal_selection_contracts;

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

}
