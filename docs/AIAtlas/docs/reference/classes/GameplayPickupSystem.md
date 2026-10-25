# GameplayPickupSystem

**class** · домен «Ресурсы и события» · `src/Gameplay/Pickup/GameplayPickupSystem.cppm:14–52` · экспортируется модулем · <span class="ev ev-source">SOURCE_VERIFIED</span> (снимок `8c6c0dc`)

Ручной разбор для этой сущности не выполнялся; ниже — автоматическая часть. Смысл и место в архитектуре см. в главах по домену «Ресурсы и события».

## Члены (автоматически)

| Строка | Вид | Доступ | Сигнатура / поле |
|---:|---|---|---|
| 17 | метод | public | `void Update(GameplayWorld& world, std::span<const EntityHandle> collectors,` |

## Объявление

Включено из исходника при сборке; `src/Gameplay/Pickup/GameplayPickupSystem.cppm:14–52`.

```cpp linenums="14"
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
```
