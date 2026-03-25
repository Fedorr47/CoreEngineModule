module;

#include <vector>

export module core:combat_system;

import :gameplay;

export namespace rendern
{
    inline void UpdateGameplayCombatRequests(
        GameplayWorld& world,
        const std::vector<EntityHandle>& entities)
    {
        UpdateGameplayActionRequestsFromPolicies(world, entities, GameplayActionPolicyGroup::Combat);
    }
}
