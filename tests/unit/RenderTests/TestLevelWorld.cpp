#include <gtest/gtest.h>
#include <vector>

import core;

using namespace rendern;

TEST(LevelWorld, EmplaceUpsertEnumerateAndDestroy)
{
	LevelWorld world{};
	const EntityHandle entity = world.CreateEntity();
	EXPECT_TRUE(world.IsEntityValid(entity));

	Transform local{};
	local.position = { 1.0f, 2.0f, 3.0f };
	Flags flags{};
	flags.alive = true;
	flags.visible = true;

	world.EmplaceNodeData(entity, 7, -1, local, mathUtils::Mat4(1.0f), flags);

	LevelNodeId nodeId{};
	ParentIndex parent{};
	LocalTransform localOut{};
	WorldTransform worldOut{};
	Flags flagsOut{};

	EXPECT_TRUE(world.TryGetLevelNodeId(entity, nodeId));
	EXPECT_TRUE(world.TryGetParentIndex(entity, parent));
	EXPECT_TRUE(world.TryGetLocalTransform(entity, localOut));
	EXPECT_TRUE(world.TryGetWorldTransform(entity, worldOut));
	EXPECT_TRUE(world.TryGetFlags(entity, flagsOut));
	EXPECT_EQ(nodeId.index, 7);
	EXPECT_EQ(parent.parent, -1);
	EXPECT_FLOAT_EQ(localOut.local.position.x, 1.0f);
	EXPECT_TRUE(flagsOut.visible);

	Renderable renderable{};
	renderable.drawIndex = 5;
	renderable.skinnedDrawIndex = 3;
	renderable.isSkinned = true;
	world.EmplaceRenderable(entity, renderable);

	EXPECT_TRUE(world.HasRenderable(entity));
	EXPECT_EQ(world.GetRenderableCount(), 1u);

	Renderable renderableOut{};
	EXPECT_TRUE(world.TryGetRenderable(entity, renderableOut));
	EXPECT_EQ(renderableOut.drawIndex, 5);
	EXPECT_TRUE(renderableOut.isSkinned);

	std::vector<int> visitedNodeIndices{};
	std::vector<int> visitedRenderableIndices{};
	world.ForEachNode([&](EntityHandle,
		const LevelNodeId& inNodeId,
		const ParentIndex&,
		const LocalTransform&,
		const WorldTransform&,
		const Flags&)
	{
		visitedNodeIndices.push_back(inNodeId.index);
	});
	world.ForEachRenderable([&](EntityHandle,
		const LevelNodeId& inNodeId,
		const WorldTransform&,
		const Renderable& inRenderable,
		const Flags&)
	{
		visitedRenderableIndices.push_back(inNodeId.index + inRenderable.drawIndex);
	});

	ASSERT_EQ(visitedNodeIndices.size(), 1u);
	ASSERT_EQ(visitedRenderableIndices.size(), 1u);
	EXPECT_EQ(visitedNodeIndices.front(), 7);
	EXPECT_EQ(visitedRenderableIndices.front(), 12);

	Transform updatedLocal = local;
	updatedLocal.position.z = 42.0f;
	world.UpsertNodeData(entity, 7, -1, updatedLocal, mathUtils::Mat4(1.0f), flags);
	EXPECT_TRUE(world.TryGetLocalTransform(entity, localOut));
	EXPECT_FLOAT_EQ(localOut.local.position.z, 42.0f);

	world.RemoveRenderable(entity);
	EXPECT_FALSE(world.HasRenderable(entity));
	EXPECT_EQ(world.GetRenderableCount(), 0u);

	world.DestroyEntity(entity);
	EXPECT_FALSE(world.IsEntityValid(entity));
}
