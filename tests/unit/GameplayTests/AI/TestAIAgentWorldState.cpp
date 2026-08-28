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

TEST(AIAgentWorldState, IntegerFactsAreIndependentAndPartOfStateIdentity)
{
    constexpr AIWorldFactId booleanFact{ 3u };
    constexpr AIWorldIntegerFactId coins{ 3u };
    constexpr AIWorldIntegerFactId unrelatedResource{ 9u };
    AIAgentWorldState first{};
    AIAgentWorldState equivalent{};

    EXPECT_EQ(first.GetIntegerFact(coins), 0);
    EXPECT_EQ(first.GetIntegerFact(unrelatedResource), 0);

    first.SetFact(booleanFact);
    first.SetIntegerFact(coins, 3);
    equivalent.SetFact(booleanFact);
    equivalent.SetIntegerFact(coins, 3);

    EXPECT_TRUE(first.IsFactSet(booleanFact));
    EXPECT_EQ(first.GetIntegerFact(coins), 3);
    EXPECT_EQ(first.GetIntegerFact(unrelatedResource), 0);
    EXPECT_EQ(first, equivalent);
    EXPECT_EQ(AIAgentWorldStateHash{}(first), AIAgentWorldStateHash{}(equivalent));

    equivalent.SetIntegerFact(coins, 2);
    EXPECT_NE(first, equivalent);

    first.Clear();
    EXPECT_FALSE(first.IsFactSet(booleanFact));
    EXPECT_EQ(first.GetIntegerFact(coins), 0);
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

// AIComponent is a membership tag, but it retains the uniform GameplayWorld
// component-access API used by the rest of the gameplay components.
TEST(AIAgentWorldState, AIComponentSupportsGameplayWorldMembershipAccess)
{
    GameplayWorld world{};
    const EntityHandle entity = world.CreateEntity();
    
    EXPECT_FALSE(world.HasAI(entity));
    
    world.AddAI(entity);
    
    EXPECT_TRUE(world.HasAI(entity));
    std::vector<EntityHandle> aiEntities{};
    world.CollectAIEntities(aiEntities);

    ASSERT_EQ(aiEntities.size(), 1u);
    EXPECT_EQ(aiEntities.front(), entity);

    world.RemoveAI(entity);
    
    EXPECT_FALSE(world.HasAI(entity));
   
    world.CollectAIEntities(aiEntities);
    EXPECT_TRUE(aiEntities.empty());
}