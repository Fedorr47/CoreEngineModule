module;

#include <algorithm>
#include <span>
#include <vector>

export module core:gameplay_pickup_system;

import :gameplay;
export import :gameplay_world_event;

export namespace rendern
{
    class GameplayPickupSystem
    {
    public:
        void Update(GameplayWorld& world, std::span<const EntityHandle> collectors,
            std::vector<GameplayWorldEvent>& outEvents) const
        {
            std::vector<EntityHandle> pickups;
            world.CollectPickupEntities(pickups);
            for (const EntityHandle pickupEntity : pickups)
            {
                GameplayPickupComponent* pickup = world.TryGetPickup(pickupEntity);
                const GameplayTransformComponent* pickupTransform = world.TryGetTransform(pickupEntity);
                if (pickup == nullptr || pickupTransform == nullptr || pickup->collected)
                {
                    continue;
                }
                for (const EntityHandle collector : collectors)
                {
                    if (collector == pickupEntity || !world.IsEntityValid(collector))
                    {
                        continue;
                    }
                    const GameplayTransformComponent* collectorTransform = world.TryGetTransform(collector);
                    if (collectorTransform == nullptr)
                    {
                        continue;
                    }
                    const mathUtils::Vec3 delta = collectorTransform->position - pickupTransform->position;
                    if (mathUtils::Dot(delta, delta) <= pickup->collectionRadius * pickup->collectionRadius)
                    {
                        pickup->collected = true;
                        outEvents.push_back({GameplayWorldEventType::PickupCollected,
                            collector, pickupEntity});
                        break;
                    }
                }
            }
        }
    };
}