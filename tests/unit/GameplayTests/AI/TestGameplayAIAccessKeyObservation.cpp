#include <gtest/gtest.h>

#include <array>

import core;

using namespace rendern;

namespace
{
    struct ObservationFixture
    {
        GameplayWorld world{};
        EntityHandle agent{world.CreateEntity()};
        EntityHandle otherAgent{world.CreateEntity()};
        std::array<EntityHandle, 3> coins{
            world.CreateEntity(), world.CreateEntity(), world.CreateEntity()};
        EntityHandle key{world.CreateEntity()};
        EntityHandle unrelated{world.CreateEntity()};
        ai_access_key_detail::AccessKeyFactBindings bindings{
            .hasAccessKey = AIWorldFactId{0u},
            .atDestination = AIWorldFactId{1u},
            .collected = {AIWorldFactId{2u}, AIWorldFactId{3u}, AIWorldFactId{4u}},
            .available = {AIWorldFactId{5u}, AIWorldFactId{6u}, AIWorldFactId{7u}},
            .spatial = {AIWorldFactId{8u}, AIWorldFactId{9u}, AIWorldFactId{10u},
                AIWorldFactId{11u}, AIWorldFactId{12u}, AIWorldFactId{13u}},
            .coins = AIWorldIntegerFactId{0u}};
        std::array<mathUtils::Vec3, ai_access_key_detail::kSpatialLocationCount> positions{
            mathUtils::Vec3{0, 0, 0}, {2, 0, 0}, {4, 0, 0}, {6, 0, 0},
            {8, 0, 0}, {10, 0, 0}};

        ObservationFixture()
        {
            world.AddTransform(agent, {.position = positions[0]});
            world.AddTransform(otherAgent, {});
            world.AddAI(agent);
            world.AddAI(otherAgent);
            for (const EntityHandle coin : coins)
            {
                world.AddPickup(coin);
                world.AddInteractionPoint(coin, {});
            }
        }
    };
}

TEST(GameplayAIAccessKeyObservation, FiltersAndIdempotentlyMapsDomainEvents)
{
    ObservationFixture fixture;
    AIAgentWorldState facts;
    ai_access_key_detail::AccessKeyObservationAdapter observer{fixture.agent, fixture.coins,
        fixture.key, fixture.bindings, fixture.positions, nullptr};

    const std::array ignored{
        GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
            fixture.otherAgent, fixture.coins[0]},
        GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
            fixture.agent, fixture.unrelated}};
    observer.Observe(ignored, fixture.world, facts);
    EXPECT_FALSE(facts.IsFactSet(fixture.bindings.collected[0]));
    EXPECT_EQ(facts.GetIntegerFact(fixture.bindings.coins), 0);

    const std::array collected{GameplayWorldEvent{GameplayWorldEventType::PickupCollected,
        fixture.agent, fixture.coins[0]}};
    observer.Observe(collected, fixture.world, facts);
    observer.Observe(collected, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.collected[0]));
    EXPECT_EQ(facts.GetIntegerFact(fixture.bindings.coins), 1);

    facts.SetIntegerFact(fixture.bindings.coins, kAccessKeyPrice + 2);
    const std::array wrongPurchases{
        GameplayWorldEvent{GameplayWorldEventType::AccessKeyPurchased,
            fixture.otherAgent, fixture.key},
        GameplayWorldEvent{GameplayWorldEventType::AccessKeyPurchased,
            fixture.agent, fixture.unrelated}};
    observer.Observe(wrongPurchases, fixture.world, facts);
    EXPECT_FALSE(facts.IsFactSet(fixture.bindings.hasAccessKey));
    const std::array purchase{GameplayWorldEvent{GameplayWorldEventType::AccessKeyPurchased,
        fixture.agent, fixture.key}};
    observer.Observe(purchase, fixture.world, facts);
    observer.Observe(purchase, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.hasAccessKey));
    EXPECT_EQ(facts.GetIntegerFact(fixture.bindings.coins), 2);
}

TEST(GameplayAIAccessKeyObservation, MapsPickupAndReservationAvailability)
{
    ObservationFixture fixture;
    GameplayObjectReservationSystem reservations;
    AIAgentWorldState facts;
    ai_access_key_detail::AccessKeyObservationAdapter observer{fixture.agent, fixture.coins,
        fixture.key, fixture.bindings, fixture.positions, &reservations};

    observer.Observe({}, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.available[0]));
    ASSERT_TRUE(reservations.TryReserve(fixture.world, fixture.coins[0], fixture.agent));
    observer.Observe({}, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.available[0]));
    ASSERT_TRUE(reservations.Release(fixture.coins[0], fixture.agent));
    ASSERT_TRUE(reservations.TryReserve(fixture.world, fixture.coins[0], fixture.otherAgent));
    observer.Observe({}, fixture.world, facts);
    EXPECT_FALSE(facts.IsFactSet(fixture.bindings.available[0]));
    ASSERT_TRUE(reservations.Release(fixture.coins[0], fixture.otherAgent));
    observer.Observe({}, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.available[0]));
    fixture.world.TryGetPickup(fixture.coins[0])->collected = true;
    observer.Observe({}, fixture.world, facts);
    EXPECT_FALSE(facts.IsFactSet(fixture.bindings.available[0]));
}

TEST(GameplayAIAccessKeyObservation, MapsExclusiveSpatialAndDestinationFacts)
{
    ObservationFixture fixture;
    AIAgentWorldState facts;
    ai_access_key_detail::AccessKeyObservationAdapter observer{fixture.agent, fixture.coins,
        fixture.key, fixture.bindings, fixture.positions, nullptr};

    for (const std::size_t location : {0u, 1u, 4u, 5u})
    {
        fixture.world.TryGetTransform(fixture.agent)->position = fixture.positions[location];
        observer.Observe({}, fixture.world, facts);
        for (std::size_t index = 0; index < fixture.bindings.spatial.size(); ++index)
        {
            EXPECT_EQ(facts.IsFactSet(fixture.bindings.spatial[index]), index == location);
        }
    }
    EXPECT_FALSE(facts.IsFactSet(fixture.bindings.atDestination));
    facts.SetFact(fixture.bindings.hasAccessKey, true);
    observer.Observe({}, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.atDestination));

    fixture.world.RemoveTransform(fixture.agent);
    facts.SetFact(fixture.bindings.spatial[0], true);
    observer.Observe({}, fixture.world, facts);
    EXPECT_TRUE(facts.IsFactSet(fixture.bindings.spatial[0]));
}