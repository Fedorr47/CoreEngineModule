module;

#include <vector>

export module core:combat_system;

import :gameplay;

export namespace rendern
{
    inline void UpdateGameplayCombatRequests(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities,
        const GameplayActionDefinitions& definitions)
    {
        UpdateGameplayActionRequestsFromPolicies(world, entities, GameplayActionPolicyGroup::Combat, definitions);
    }
}
