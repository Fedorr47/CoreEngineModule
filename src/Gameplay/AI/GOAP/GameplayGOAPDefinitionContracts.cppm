module;

#include <string>
#include <vector>

export module core:gameplay_goap_definition_contracts;

import :ai_action_contracts;
export import :ai_goal_selection;

export namespace rendern
{
    struct AINamedBooleanFact { AIWorldFactId id{}; std::string name{}; };
    struct AINamedIntegerFact { AIWorldIntegerFactId id{}; std::string name{}; };
    struct AINamedGoal { AIGoalId id{}; std::string name{}; };
    struct AINamedAction
    {
        AIActionId actionId{};
        AIActionContextId contextId{};
        std::string actionName{};
        std::string contextName{};
    };
    struct AIDefinitionMetadata
    {
        std::vector<AINamedBooleanFact> booleanFacts{};
        std::vector<AINamedIntegerFact> integerFacts{};
        std::vector<AINamedGoal> goals{};
        std::vector<AINamedAction> actions{};
    };

    struct GameplayGOAPDecisionDefinition
    {
        std::vector<AIGoalSelectionCandidate> goals{};
        std::vector<AIActionDefinition> actions{};
        AIDefinitionMetadata metadata{};
    };
}