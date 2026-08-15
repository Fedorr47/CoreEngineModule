module;

#include <vector>

export module core:interaction_system;

import :gameplay;

export namespace rendern
{
    inline void UpdateGameplayInteractionRequests(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayActionDefinitions& definitions)
    {
        UpdateGameplayActionRequestsFromPolicies(world, entities, GameplayActionPolicyGroup::Interaction, definitions);
    }
}
