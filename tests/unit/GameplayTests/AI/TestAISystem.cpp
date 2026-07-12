#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

import core;

using namespace rendern;

// Protects the ownership contract that AI-agent membership is derived from
// AIComponent presence and does not affect unrelated gameplay components.
TEST(AISystem, AIComponentLifecycleControlsAgentMembership)
{
    GameplayWorld world{};
    const EntityHandle entity = world.CreateEntity();

    world.AddTransform(entity, GameplayTransformComponent{ .position = { 1.0f, 2.0f, 3.0f } });
    world.AddAI(entity);

    std::vector<EntityHandle> discoveredEntities{};
    world.CollectAIEntities(discoveredEntities);

    EXPECT_TRUE(world.HasAI(entity));
    EXPECT_EQ(discoveredEntities, std::vector<EntityHandle>{ entity });

    world.RemoveAI(entity);
    world.CollectAIEntities(discoveredEntities);

    EXPECT_FALSE(world.HasAI(entity));
    EXPECT_TRUE(discoveredEntities.empty());
    ASSERT_NE(world.TryGetTransform(entity), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.x, 1.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.y, 2.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.z, 3.0f);
}

// Protects the filtering boundary so non-AI and destroyed entities cannot
// remain in a stale registration list and reach future AI behavior.
TEST(AISystem, DiscoveryContainsOnlyLiveEntitiesWithAIComponent)
{
    GameplayWorld world{};

    const EntityHandle liveAI = world.CreateEntity();
    const EntityHandle nonAI = world.CreateEntity();
    const EntityHandle destroyedAI = world.CreateEntity();

    world.AddAI(liveAI);
    world.AddAI(destroyedAI);
    world.DestroyEntity(destroyedAI);

    std::vector<EntityHandle> discoveredEntities{};
    world.CollectAIEntities(discoveredEntities);

    EXPECT_EQ(discoveredEntities, std::vector<EntityHandle>{ liveAI });
    EXPECT_TRUE(world.IsEntityValid(nonAI));
    EXPECT_FALSE(world.HasAI(nonAI));
}

// Protects deterministic AI traversal so later planning does not depend on
// EnTT storage order or change between otherwise identical frames.
TEST(AISystem, AgentDiscoveryUsesDeterministicEntityHandleOrder)
{
    GameplayWorld world{};

    const EntityHandle first = world.CreateEntity();
    const EntityHandle second = world.CreateEntity();
    const EntityHandle third = world.CreateEntity();

    world.AddAI(third);
    world.AddAI(first);
    world.AddAI(second);

    std::vector<EntityHandle> firstDiscovery{};
    std::vector<EntityHandle> secondDiscovery{};
    world.CollectAIEntities(firstDiscovery);
    world.CollectAIEntities(secondDiscovery);

    const std::vector<EntityHandle> expectedOrder{ first, second, third };
    EXPECT_EQ(firstDiscovery, expectedOrder);
    EXPECT_EQ(secondDiscovery, expectedOrder);
    EXPECT_TRUE(std::is_sorted(firstDiscovery.begin(), firstDiscovery.end()));
}

// Protects the initial no-op update contract so empty AI agents can be
// stepped repeatedly without mutating unrelated gameplay state.
TEST(AISystem, EmptyAgentsUpdateWithoutGameplaySideEffects)
{
    GameplayWorld world{};
    AISystem aiSystem{};

    EXPECT_EQ(aiSystem.Update(world), 0u);

    const EntityHandle aiEntity = world.CreateEntity();
    const EntityHandle nonAIEntity = world.CreateEntity();
    world.AddAI(aiEntity);
    world.AddTransform(aiEntity, GameplayTransformComponent{ .position = { 4.0f, 5.0f, 6.0f } });

    EXPECT_EQ(aiSystem.Update(world), 1u);
    EXPECT_EQ(aiSystem.Update(world), 1u);
    EXPECT_EQ(world.GetAliveCount(), 2u);
    EXPECT_TRUE(world.IsEntityValid(aiEntity));
    EXPECT_TRUE(world.IsEntityValid(nonAIEntity));
    EXPECT_TRUE(world.HasAI(aiEntity));
    EXPECT_FALSE(world.HasAI(nonAIEntity));

    ASSERT_NE(world.TryGetTransform(aiEntity), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.x, 4.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.y, 5.0f);
    EXPECT_FLOAT_EQ(world.TryGetTransform(aiEntity)->position.z, 6.0f);
}