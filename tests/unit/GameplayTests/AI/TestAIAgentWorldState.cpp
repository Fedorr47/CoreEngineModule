#include <gtest/gtest.h>

#include <cstdint>

import core;

using namespace rendern;

// Protects the empty-snapshot contract so a newly created agent does not
// satisfy any condition before a producer explicitly writes a world fact.
TEST(AIAgentWorldState, DefaultStateContainsNoSetFacts)
{
    const AIAgentWorldState worldState{};

    constexpr AIWorldFactId firstFact{ 0u };
    constexpr AIWorldFactId middleFact{ AIAgentWorldState::FactCapacity / 2 };
    constexpr AIWorldFactId lastFact{
        static_cast<std::uint16_t>(
            AIAgentWorldState::FactCapacity - 1u)
    };

    EXPECT_FALSE(worldState.IsFactSet(firstFact));
    EXPECT_FALSE(worldState.IsFactSet(middleFact));
    EXPECT_FALSE(worldState.IsFactSet(lastFact));
}

// Protects independent fact storage so changing one condition cannot alter
// another condition consumed by future goals, plans, or action validation.
TEST(AIAgentWorldState, FactsCanBeSetAndClearedIndependently)
{
    AIAgentWorldState worldState{};

    constexpr AIWorldFactId firstFact{ 3u };
    constexpr AIWorldFactId secondFact{ 9u };

    worldState.SetFact(firstFact);
    worldState.SetFact(secondFact, true);

    EXPECT_TRUE(worldState.IsFactSet(firstFact));
    EXPECT_TRUE(worldState.IsFactSet(secondFact));

    worldState.SetFact(firstFact, false);

    EXPECT_FALSE(worldState.IsFactSet(firstFact));
    EXPECT_TRUE(worldState.IsFactSet(secondFact));

    worldState.ClearFact(secondFact);

    EXPECT_FALSE(worldState.IsFactSet(firstFact));
    EXPECT_FALSE(worldState.IsFactSet(secondFact));
}

// Protects the bounded-storage contract by verifying that both edge slots in
// the declared capacity remain usable without relying on hard-coded limits.
TEST(AIAgentWorldState, FirstAndLastFactSlotsCanBeStored)
{
    AIAgentWorldState worldState{};

    constexpr AIWorldFactId firstFact{ 0u };
    constexpr AIWorldFactId lastFact{
        static_cast<std::uint16_t>(
            AIAgentWorldState::FactCapacity - 1u)
    };

    worldState.SetFact(firstFact);
    worldState.SetFact(lastFact);

    EXPECT_TRUE(worldState.IsFactSet(firstFact));
    EXPECT_TRUE(worldState.IsFactSet(lastFact));

    worldState.ClearFact(firstFact);
    worldState.ClearFact(lastFact);

    EXPECT_FALSE(worldState.IsFactSet(firstFact));
    EXPECT_FALSE(worldState.IsFactSet(lastFact));
}

TEST(AIAgentWorldState, EqualityAndHashCoverCompleteFactState)
{
    AIAgentWorldState first{};
    AIAgentWorldState equivalent{};
    AIAgentWorldState different{};
    constexpr AIWorldFactId firstFact{ 0u };
    constexpr AIWorldFactId lastFact{
        static_cast<std::uint16_t>(AIAgentWorldState::FactCapacity - 1u) };

    first.SetFact(firstFact);
    first.SetFact(lastFact);
    equivalent.SetFact(firstFact);
    equivalent.SetFact(lastFact);
    different.SetFact(firstFact);

    EXPECT_EQ(first, equivalent);
    EXPECT_NE(first, different);
    EXPECT_EQ(AIAgentWorldStateHash{}(first), AIAgentWorldStateHash{}(equivalent));
}

// Protects complete snapshot replacement so producers can rebuild an agent's
// facts without leaving conditions from a previous update in the state.
TEST(AIAgentWorldState, ClearRemovesEverySetFact)
{
    AIAgentWorldState worldState{};

    constexpr AIWorldFactId firstFact{ 1u };
    constexpr AIWorldFactId secondFact{ AIAgentWorldState::FactCapacity / 2 };
    constexpr AIWorldFactId thirdFact{
        static_cast<std::uint16_t>(
            AIAgentWorldState::FactCapacity - 1u)
    };

    worldState.SetFact(firstFact);
    worldState.SetFact(secondFact);
    worldState.SetFact(thirdFact);

    worldState.Clear();

    EXPECT_FALSE(worldState.IsFactSet(firstFact));
    EXPECT_FALSE(worldState.IsFactSet(secondFact));
    EXPECT_FALSE(worldState.IsFactSet(thirdFact));
}

// Protects the ECS ownership boundary that each agent's fact snapshot lives in
// AIComponent and remains accessible without exposing the EnTT registry.
TEST(AIAgentWorldState, AIComponentOwnsWorldStateThroughGameplayWorld)
{
    GameplayWorld world{};
    const EntityHandle entity = world.CreateEntity();

    constexpr AIWorldFactId firstFact{ 4u };
    constexpr AIWorldFactId secondFact{ 5u };

    AIComponent component{};
    component.worldState.SetFact(firstFact);

    world.AddAI(entity, component);

    EXPECT_TRUE(world.HasAI(entity));

    AIComponent* storedComponent = world.TryGetAI(entity);
    ASSERT_NE(storedComponent, nullptr);

    EXPECT_TRUE(storedComponent->worldState.IsFactSet(firstFact));
    EXPECT_FALSE(storedComponent->worldState.IsFactSet(secondFact));

    storedComponent->worldState.SetFact(secondFact);

    const GameplayWorld& constWorld = world;
    const AIComponent* constStoredComponent =
        constWorld.TryGetAI(entity);

    ASSERT_NE(constStoredComponent, nullptr);
    EXPECT_TRUE(
        constStoredComponent->worldState.IsFactSet(firstFact));
    EXPECT_TRUE(
        constStoredComponent->worldState.IsFactSet(secondFact));

    AIComponent replacement{};
    replacement.worldState.SetFact(secondFact);

    world.SetAI(entity, replacement);

    const AIComponent* replacedComponent =
        constWorld.TryGetAI(entity);

    ASSERT_NE(replacedComponent, nullptr);
    EXPECT_FALSE(
        replacedComponent->worldState.IsFactSet(firstFact));
    EXPECT_TRUE(
        replacedComponent->worldState.IsFactSet(secondFact));
}