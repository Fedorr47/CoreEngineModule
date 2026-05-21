#include <gtest/gtest.h>

#include "LevelInstantiateTestHelper.h"

import core;

using namespace rendern;

TEST(LevelInstantiate, RejectsUnknownNodeReference)
{
	test::LevelInstantiateHarness harness{};

	LevelAsset level{};
	LevelNode node{};
	node.name = "child_with_invalid_parent";
	node.parent = 99;
	node.alive = true;
	node.visible = true;
	level.nodes.push_back(node);

	// Parent index references are resolved during instantiate world-building.
	// An out-of-range parent must fail at the boundary instead of being silently treated as root.
	harness.ExpectInstantiateThrowsWithFragments(level, { "unknown parent node", "99", "0" });
}

TEST(LevelInstantiate, RejectsUnknownMeshReference)
{
	test::LevelInstantiateHarness harness{};

	LevelAsset level{};
	LevelNode node{};
	node.name = "renderable_with_missing_mesh";
	node.mesh = "missingMesh";
	node.alive = true;
	node.visible = true;
	level.nodes.push_back(node);

	harness.ExpectInstantiateThrowsWithFragments(level, { "unknown meshId", "missingMesh" });
}

TEST(LevelInstantiate, RejectsUnknownMaterialReference)
{
	test::LevelInstantiateHarness harness{};

	LevelAsset level{};
	level.meshes.emplace("knownMesh", LevelMeshDef{ .path = "tests/assets/assimp/single_triangle.obj", .debugName = "known" });

	LevelNode node{};
	node.name = "renderable_with_missing_material";
	node.mesh = "knownMesh";
	node.material = "missingMaterial";
	node.alive = true;
	node.visible = true;
	level.nodes.push_back(node);

	harness.ExpectInstantiateThrowsWithFragments(level, { "unknown materialId", "missingMaterial" });
}