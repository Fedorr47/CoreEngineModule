module;

#include <vector>

export module core:interaction_system;

import :gameplay;

export namespace rendern
{
    inline void UpdateGameplayInteractionRequests(
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

            if (command->wantsInteract)
            {
                action->requested = GameplayActionKind::Interact;
            }
        }
    }
}
