#include <gtest/gtest.h>

#include <limits>

import core;

using namespace rendern;

// The world stores and returns interaction-point component data for a live entity.
TEST(GameplayInteractionPoint, ComponentCanBeAddedAndQueried)
{
    GameplayWorld world;
    const EntityHandle entity = world.CreateEntity();
    world.AddInteractionPoint(entity, { .localPosition = { 1.0f, 2.0f, 3.0f }, .localFacingYawDegrees = 25.0f });
    ASSERT_TRUE(world.HasInteractionPoint(entity));
    ASSERT_NE(world.TryGetInteractionPoint(entity), nullptr);
    EXPECT_FLOAT_EQ(world.TryGetInteractionPoint(entity)->localPosition.z, 3.0f);
}

// Missing and destroyed entities never expose interaction-point storage.
TEST(GameplayInteractionPoint, MissingAndDestroyedComponentsAreInaccessible)
{
    GameplayWorld world;
    const EntityHandle entity = world.CreateEntity();
    EXPECT_FALSE(world.HasInteractionPoint(entity));
    EXPECT_EQ(world.TryGetInteractionPoint(entity), nullptr);
    world.AddInteractionPoint(entity, {});
    world.DestroyEntity(entity);
    EXPECT_FALSE(world.HasInteractionPoint(entity));
    EXPECT_EQ(world.TryGetInteractionPoint(entity), nullptr);
}

// Resolution translates a local point and combines its facing with object yaw.
TEST(GameplayInteractionPoint, ResolvesPositionAndFacingInWorldSpace)
{
    GameplayWorld world;
    const EntityHandle object = world.CreateEntity();
    world.AddTransform(object, { .position = { 10.0f, 4.0f, 20.0f }, .rotationDegrees = { 0.0f, 0.0f, 0.0f } });
    world.AddInteractionPoint(object, { .localPosition = { 1.0f, 2.0f, 3.0f }, .localFacingYawDegrees = 30.0f });
    const auto resolved = ResolveGameplayInteractionPoint(world, object);
    ASSERT_TRUE(resolved.has_value());
    EXPECT_FLOAT_EQ(resolved->worldPosition.x, 11.0f);
    EXPECT_FLOAT_EQ(resolved->worldPosition.y, 6.0f);
    EXPECT_FLOAT_EQ(resolved->worldPosition.z, 23.0f);
    EXPECT_FLOAT_EQ(resolved->worldFacingYawDegrees, 30.0f);
}

// Positive yaw rotates local forward toward positive world X, matching character forward math.
TEST(GameplayInteractionPoint, RotatesLocalOffsetWithObjectYaw)
{
    GameplayWorld world;
    const EntityHandle object = world.CreateEntity();
    world.AddTransform(object, { .position = { 10.0f, 0.0f, 20.0f }, .rotationDegrees = { 0.0f, 90.0f, 0.0f } });
    world.AddInteractionPoint(object, { .localPosition = { 0.0f, 0.0f, 2.0f }, .localFacingYawDegrees = 15.0f });
    const auto resolved = ResolveGameplayInteractionPoint(world, object);
    ASSERT_TRUE(resolved.has_value());
    EXPECT_NEAR(resolved->worldPosition.x, 12.0f, 0.0001f);
    EXPECT_NEAR(resolved->worldPosition.z, 20.0f, 0.0001f);
    EXPECT_FLOAT_EQ(resolved->worldFacingYawDegrees, 105.0f);
}

// Resolution reports absence for invalid entities and either required missing component.
TEST(GameplayInteractionPoint, RejectsInvalidOrIncompleteObjects)
{
    GameplayWorld world;
    const EntityHandle noComponents = world.CreateEntity();
    const EntityHandle noPoint = world.CreateEntity();
    world.AddTransform(noPoint, {});
    const EntityHandle noTransform = world.CreateEntity();
    world.AddInteractionPoint(noTransform, {});
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, kNullEntity).has_value());
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, noComponents).has_value());
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, noPoint).has_value());
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, noTransform).has_value());
}

// Resolution rejects every non-finite authored or transform value used by the calculation.
TEST(GameplayInteractionPoint, RejectsNonFiniteValues)
{
    GameplayWorld world;
    const float infinity = std::numeric_limits<float>::infinity();
    const EntityHandle object = world.CreateEntity();
    world.AddTransform(object, {});
    world.AddInteractionPoint(object, { .localPosition = { infinity, 0.0f, 0.0f } });
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, object).has_value());
    world.SetInteractionPoint(object, { .localFacingYawDegrees = infinity });
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, object).has_value());
    world.SetInteractionPoint(object, {});
    world.SetTransform(object, { .position = { infinity, 0.0f, 0.0f } });
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, object).has_value());
    world.SetTransform(object, { .rotationDegrees = { 0.0f, infinity, 0.0f } });
    EXPECT_FALSE(ResolveGameplayInteractionPoint(world, object).has_value());
}