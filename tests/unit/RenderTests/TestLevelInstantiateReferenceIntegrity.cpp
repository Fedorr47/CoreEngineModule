#include <gtest/gtest.h>
#include <filesystem>

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

TEST(LevelInstantiate, SemanticMotionSourceIsImportedBeforeControllerBinding)
{
	test::LevelInstantiateHarness harness{};
	LevelAsset level{};
	level.skinnedMeshes.emplace("character", LevelSkinnedMeshDef{
		.path = "models/Character.fbx", .debugName = "SemanticSourceCharacter" });
	level.animations.emplace("test_external_source", LevelAnimationDef{
		.path = "animations/idle.fbx", .debugName = "SemanticExternalSource" });
	AnimationControllerAsset controller{ .id = "TestController", .defaultState = "SemanticState" };
	controller.states.push_back(AnimationStateDesc{
		.name = "SemanticState", .motionId = MotionId{ "Test.CustomMotion" } });
	level.animationControllers.emplace(controller.id, controller);
	AnimationProfileAsset profile{ .id = "TestProfile" };
	profile.motions.emplace("Test.CustomMotion", AnimationClipRef{ "test_external_source", "" });
	level.animationProfiles.emplace(profile.id, profile);
	level.nodes.push_back(LevelNode{ .name = "SemanticCharacter", .skinnedMesh = "character",
		.animationController = "TestController", .animationProfile = "TestProfile" });

	[[maybe_unused]] const LevelInstance instance = harness.Instantiate(level);
	ASSERT_EQ(harness.GetScene().skinnedDrawItems.size(), 1u);
	const SkinnedDrawItem& draw = harness.GetScene().skinnedDrawItems.front();
	ASSERT_EQ(draw.asset->externalAnimationSources.size(), 1u);
	EXPECT_EQ(draw.asset->externalAnimationSources.front().assetId, "test_external_source");
	ASSERT_EQ(draw.controller.resolvedStateClipIndices.size(), 1u);
	EXPECT_GE(draw.controller.resolvedStateClipIndices.front(), 0);
}

namespace
{
	LevelPhysicsBodyDef BodyWithMotion(const physics::PhysicsMotionType motionType)
	{
		LevelPhysicsBodyDef body{};
		body.motionType = motionType;
		return body;
	}

	LevelAsset MakeMeshParticipationLevel()
	{
		LevelAsset level{};
		level.meshes.emplace("mesh", LevelMeshDef{ .path = "models/cube.obj", .debugName = "participation" });

		auto AddNode = [&](std::string name, const bool visible, std::optional<LevelPhysicsBodyDef> body)
		{
			LevelNode node{};
			node.name = std::move(name);
			node.mesh = "mesh";
			node.alive = true;
			node.visible = visible;
			node.physicsBody = std::move(body);
			level.nodes.push_back(std::move(node));
		};

		AddNode("NoBody", true, std::nullopt);
		AddNode("Static", true, BodyWithMotion(physics::PhysicsMotionType::Static));
		AddNode("Dynamic", true, BodyWithMotion(physics::PhysicsMotionType::Dynamic));
		AddNode("Kinematic", true, BodyWithMotion(physics::PhysicsMotionType::Kinematic));
		AddNode("InvisibleStatic", false, BodyWithMotion(physics::PhysicsMotionType::Static));
		return level;
	}
}

TEST(LevelInstantiate, StaticNavigationMeshSourcesExcludeMovingBodiesWithoutChangingRendering)
{
	test::LevelInstantiateHarness harness{};
	const LevelAsset level = MakeMeshParticipationLevel();
	const LevelInstance instance = harness.Instantiate(level);

	const auto& sources = instance.GetStaticMeshSources();
	ASSERT_EQ(sources.size(), 3u);
	EXPECT_EQ(sources[0].nodeIndex, 0);
	EXPECT_EQ(sources[1].nodeIndex, 1);
	EXPECT_EQ(sources[2].nodeIndex, 4);
	EXPECT_EQ(harness.GetScene().drawItems.size(), 4u);
}

TEST(LevelInstantiate, StaticNavigationModelSourcesUseMotionRuleWithoutChangingRendering)
{
	test::LevelInstantiateHarness harness{};
	LevelAsset level{};
	level.models.emplace("model", LevelModelDef{
		.path = std::filesystem::path(CORE_TEST_FIXTURE_ROOT).append("assimp/single_triangle.obj").string(),
		.debugName = "participation-model"
	});
	LevelNode noBodyNode{};
	noBodyNode.model = "model";
	level.nodes.push_back(std::move(noBodyNode));
	for (const physics::PhysicsMotionType motionType : {
		physics::PhysicsMotionType::Static,
		physics::PhysicsMotionType::Dynamic,
		physics::PhysicsMotionType::Kinematic })
	{
		LevelNode node{};
		node.model = "model";
		node.physicsBody = BodyWithMotion(motionType);
		level.nodes.push_back(std::move(node));
	}

	const LevelInstance instance = harness.Instantiate(level);
	ASSERT_EQ(instance.GetStaticMeshSources().size(), 2u);
	EXPECT_EQ(instance.GetStaticMeshSources().front().nodeIndex, 0);
	EXPECT_EQ(instance.GetStaticMeshSources()[1].nodeIndex, 1);
	EXPECT_EQ(harness.GetScene().drawItems.size(), 4u);
}

TEST(LevelInstantiate, FallingBodySmokeCollectsFloorButNotDynamicSphere)
{
	test::LevelInstantiateHarness harness{};
	const LevelAsset level = LoadLevelAssetFromJson("levels/physics_falling_body_smoke.level.json");
	const LevelInstance instance = harness.Instantiate(level);

	ASSERT_EQ(instance.GetStaticMeshSources().size(), 1u);
	EXPECT_EQ(level.nodes[static_cast<std::size_t>(instance.GetStaticMeshSources().front().nodeIndex)].name,
		"PhysicsSmokeFloor");
	EXPECT_EQ(harness.GetScene().drawItems.size(), 2u);
}