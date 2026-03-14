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
        for (const EntityHandle entity : entities)
        {
            const GameplayCharacterCommandComponent* command = world.TryGetCharacterCommand(entity);
            GameplayActionComponent* action = world.TryGetAction(entity);
            if (command == nullptr || action == nullptr)
            {
                continue;
            }

            if (action->busy || action->requested != GameplayActionKind::None)
            {
                continue;
            }

            if (command->wantsJump)
            {
                action->requested = GameplayActionKind::Jump;
            }
            else if (command->wantsAttack)
            {
                action->requested = GameplayActionKind::LightAttack;
            }
        }
    }
}
