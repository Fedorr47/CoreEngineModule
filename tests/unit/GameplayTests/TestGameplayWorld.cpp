#include <gtest/gtest.h>

import core;

using namespace rendern;

TEST(GameplayWorld, CreateAddGetRemoveDestroyAndClear)
{
	GameplayWorld world{};
	const EntityHandle entity = world.CreateEntity();

	EXPECT_TRUE(world.IsEntityValid(entity));
	EXPECT_EQ(world.GetAliveCount(), 1u);

	GameplayTransformComponent transform{};
	transform.position = { 1.0f, 2.0f, 3.0f };
	world.AddTransform(entity, transform);

	GameplayInputIntentComponent intent{};
	intent.moveX = 0.5f;
	intent.runHeld = true;
	world.AddInputIntent(entity, intent);

	world.AddCharacterCommand(entity);
	world.AddCharacterMotor(entity);
	world.AddCharacterMovementState(entity);
	world.AddLocomotion(entity);
	world.AddAction(entity);

	ASSERT_NE(world.TryGetTransform(entity), nullptr);
	EXPECT_TRUE(world.HasTransform(entity));
	EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->position.x, 1.0f);

	ASSERT_NE(world.TryGetInputIntent(entity), nullptr);
	EXPECT_TRUE(world.HasInputIntent(entity));
	EXPECT_FLOAT_EQ(world.TryGetInputIntent(entity)->moveX, 0.5f);
	EXPECT_TRUE(world.TryGetInputIntent(entity)->runHeld);

	GameplayTransformComponent updated = *world.TryGetTransform(entity);
	updated.rotationDegrees.y = 90.0f;
	world.SetTransform(entity, updated);
	EXPECT_FLOAT_EQ(world.TryGetTransform(entity)->rotationDegrees.y, 90.0f);

	world.RemoveLocomotion(entity);
	EXPECT_FALSE(world.HasLocomotion(entity));
	EXPECT_EQ(world.TryGetLocomotion(entity), nullptr);

	world.DestroyEntity(entity);
	EXPECT_FALSE(world.IsEntityValid(entity));
	EXPECT_EQ(world.GetAliveCount(), 0u);

	world.Clear();
	EXPECT_EQ(world.GetAliveCount(), 0u);
}

TEST(GameplayWorld, MultipleEntitiesMaintainIndependentComponentState)
{
	GameplayWorld world{};
	const EntityHandle first = world.CreateEntity();
	const EntityHandle second = world.CreateEntity();

	world.AddTransform(first, GameplayTransformComponent{ .position = { 1.0f, 0.0f, 0.0f } });
	world.AddTransform(second, GameplayTransformComponent{ .position = { 0.0f, 2.0f, 0.0f } });
	world.AddAction(first, GameplayActionComponent{ .requested = GameplayActionKind::Interact });
	world.AddAction(second, GameplayActionComponent{ .requested = GameplayActionKind::Jump });

	ASSERT_NE(world.TryGetTransform(first), nullptr);
	ASSERT_NE(world.TryGetTransform(second), nullptr);
	EXPECT_FLOAT_EQ(world.TryGetTransform(first)->position.x, 1.0f);
	EXPECT_FLOAT_EQ(world.TryGetTransform(second)->position.y, 2.0f);
	EXPECT_EQ(world.TryGetAction(first)->requested, GameplayActionKind::Interact);
	EXPECT_EQ(world.TryGetAction(second)->requested, GameplayActionKind::Jump);
}
