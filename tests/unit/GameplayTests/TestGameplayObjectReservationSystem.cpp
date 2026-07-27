#include <gtest/gtest.h>

#include "TestSupport/TestThreadAffinity.h"

import core;

using namespace rendern;

namespace
{
    EntityHandle MakeObject(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddInteractionPoint(entity, {});
        return entity;
    }

    EntityHandle MakeAgent(GameplayWorld& world)
    {
        const EntityHandle entity = world.CreateEntity();
        world.AddAI(entity);
        return entity;
    }
}

// Exclusive ownership is idempotent for its owner and cannot be stolen or released by a competitor.
TEST(GameplayObjectReservationSystem, EnforcesExclusiveOwnerSemantics)
{
    GameplayWorld world;
    GameplayObjectReservationSystem reservations;
    const EntityHandle object = MakeObject(world);
    const EntityHandle first = MakeAgent(world);
    const EntityHandle second = MakeAgent(world);
    EXPECT_TRUE(reservations.TryReserve(world, object, first));
    EXPECT_TRUE(reservations.TryReserve(world, object, first));
    EXPECT_FALSE(reservations.TryReserve(world, object, second));
    EXPECT_EQ(reservations.GetReservationOwner(object), first);
    EXPECT_TRUE(reservations.IsReserved(object));
    EXPECT_TRUE(reservations.IsReservedBy(object, first));
    EXPECT_FALSE(reservations.Release(object, second));
    EXPECT_TRUE(reservations.Release(object, first));
    EXPECT_EQ(reservations.GetReservationOwner(object), kNullEntity);
    EXPECT_FALSE(reservations.Release(object, first));
}

// Validation requires distinct valid entities, an AI owner, and an interaction-point object only.
TEST(GameplayObjectReservationSystem, ValidatesReservationParticipantsWithoutMovementRequirements)
{
    GameplayWorld world;
    GameplayObjectReservationSystem reservations;
    const EntityHandle object = MakeObject(world);
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle nonAI = world.CreateEntity();
    const EntityHandle noPoint = world.CreateEntity();
    EXPECT_TRUE(reservations.TryReserve(world, object, agent));
    EXPECT_FALSE(reservations.TryReserve(world, kNullEntity, agent));
    EXPECT_FALSE(reservations.TryReserve(world, noPoint, agent));
    EXPECT_FALSE(reservations.TryReserve(world, object, kNullEntity));
    EXPECT_FALSE(reservations.TryReserve(world, object, nonAI));
    world.AddAI(object);
    EXPECT_FALSE(reservations.TryReserve(world, object, object));
    EXPECT_EQ(reservations.GetReservationOwner(object), agent);
}

// Reservation cardinality is per object, allowing one agent multiple objects and agents separate objects.
TEST(GameplayObjectReservationSystem, AllowsIndependentReservationsPerObject)
{
    GameplayWorld world;
    GameplayObjectReservationSystem reservations;
    const EntityHandle firstObject = MakeObject(world);
    const EntityHandle secondObject = MakeObject(world);
    const EntityHandle thirdObject = MakeObject(world);
    const EntityHandle firstAgent = MakeAgent(world);
    const EntityHandle secondAgent = MakeAgent(world);
    EXPECT_TRUE(reservations.TryReserve(world, firstObject, firstAgent));
    EXPECT_TRUE(reservations.TryReserve(world, secondObject, firstAgent));
    EXPECT_TRUE(reservations.TryReserve(world, thirdObject, secondAgent));
}

// Synchronous cleanup removes all stale ownership reasons while preserving valid entries and exact counts.
TEST(GameplayObjectReservationSystem, CleansStaleReservationsDeterministically)
{
    GameplayWorld world;
    GameplayObjectReservationSystem reservations;
    const EntityHandle destroyedObject = MakeObject(world);
    const EntityHandle destroyedAgentObject = MakeObject(world);
    const EntityHandle removedAIObject = MakeObject(world);
    const EntityHandle removedPointObject = MakeObject(world);
    const EntityHandle validObject = MakeObject(world);
    const EntityHandle commonAgent = MakeAgent(world);
    const EntityHandle destroyedAgent = MakeAgent(world);
    const EntityHandle removedAIAgent = MakeAgent(world);
    EXPECT_TRUE(reservations.TryReserve(world, destroyedObject, commonAgent));
    EXPECT_TRUE(reservations.TryReserve(world, destroyedAgentObject, destroyedAgent));
    EXPECT_TRUE(reservations.TryReserve(world, removedAIObject, removedAIAgent));
    EXPECT_TRUE(reservations.TryReserve(world, removedPointObject, commonAgent));
    EXPECT_TRUE(reservations.TryReserve(world, validObject, commonAgent));
    world.DestroyEntity(destroyedObject);
    world.DestroyEntity(destroyedAgent);
    world.RemoveAI(removedAIAgent);
    world.RemoveInteractionPoint(removedPointObject);
    EXPECT_EQ(reservations.CleanupInvalidReservations(world), 4u);
    EXPECT_TRUE(reservations.IsReservedBy(validObject, commonAgent));
}

// Reset clears every reservation without changing world component state.
TEST(GameplayObjectReservationSystem, ResetClearsEveryReservation)
{
    GameplayWorld world;
    GameplayObjectReservationSystem reservations;
    const EntityHandle agent = MakeAgent(world);
    const EntityHandle first = MakeObject(world);
    const EntityHandle second = MakeObject(world);
    EXPECT_TRUE(reservations.TryReserve(world, first, agent));
    EXPECT_TRUE(reservations.TryReserve(world, second, agent));
    reservations.Reset();
    EXPECT_FALSE(reservations.IsReserved(first));
    EXPECT_FALSE(reservations.IsReserved(second));
}

// The runtime facade forwards reservation queries and owner-only release on the runtime thread.
TEST(GameplayRuntime, ReservesAndReleasesGameplayObject)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime runtime;
    GameplayWorld& world = runtime.GetWorld();
    const EntityHandle object = MakeObject(world);
    const EntityHandle agent = MakeAgent(world);
    EXPECT_TRUE(runtime.TryReserveGameplayObject(object, agent));
    EXPECT_TRUE(runtime.IsGameplayObjectReserved(object));
    EXPECT_TRUE(runtime.IsGameplayObjectReservedBy(object, agent));
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(object), agent);
    EXPECT_TRUE(runtime.ReleaseGameplayObject(object, agent));
    EXPECT_EQ(runtime.GetGameplayObjectReservationOwner(object), kNullEntity);
}

// Runtime shutdown resets owned reservation state along with its gameplay world.
TEST(GameplayRuntime, ClearsObjectReservationsDuringShutdown)
{
    InlineThreadOwnerRolesGuard guard{};
    GameplayRuntime runtime;
    GameplayWorld& world = runtime.GetWorld();
    const EntityHandle object = MakeObject(world);
    const EntityHandle agent = MakeAgent(world);
    ASSERT_TRUE(runtime.TryReserveGameplayObject(object, agent));
    runtime.Shutdown();
    EXPECT_FALSE(runtime.IsGameplayObjectReserved(object));
}