#include <gtest/gtest.h>
#include <array>
#include <vector>

import core;

using namespace rendern;

TEST(GameplayPickupSystem, EmitsExactlyOnceWhenCollectorEntersRadius)
{
    GameplayWorld world;
    const EntityHandle collector = world.CreateEntity();
    const EntityHandle pickupEntity = world.CreateEntity();
    world.AddTransform(collector, {.position={2.0f, 0.0f, 0.0f}});
    world.AddTransform(pickupEntity, {.position={0.0f, 0.0f, 0.0f}});
    world.AddPickup(pickupEntity, {.collectionRadius=0.5f});
    const std::array collectors{collector};
    std::vector<GameplayWorldEvent> events;
    GameplayPickupSystem system;

    system.Update(world, collectors, events);
    EXPECT_TRUE(events.empty());
    ASSERT_NE(world.TryGetPickup(pickupEntity), nullptr);
    EXPECT_FALSE(world.TryGetPickup(pickupEntity)->collected);

    world.TryGetTransform(collector)->position = {0.25f, 0.0f, 0.0f};
    system.Update(world, collectors, events);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, GameplayWorldEventType::PickupCollected);
    EXPECT_EQ(events[0].instigator, collector);
    EXPECT_EQ(events[0].subject, pickupEntity);
    EXPECT_TRUE(world.TryGetPickup(pickupEntity)->collected);

    system.Update(world, collectors, events);
    EXPECT_EQ(events.size(), 1u);
}