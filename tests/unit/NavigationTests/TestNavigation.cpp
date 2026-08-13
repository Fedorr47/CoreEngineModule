#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

import core;

namespace
{
	navigation::Geometry MakePlane(const float minimumX = 0.0f, const float maximumX = 10.0f)
	{
		return {
			{
				{ minimumX, 0.0f, 0.0f },
				{ maximumX, 0.0f, 0.0f },
				{ maximumX, 0.0f, 10.0f },
				{ minimumX, 0.0f, 10.0f }
			},
			{ 0, 2, 1, 0, 3, 2 }
		};
	}

	void AddHorizontalQuad(
		navigation::Geometry& geometry,
		const float minimumX,
		const float maximumX,
		const float minimumZ,
		const float maximumZ)
	{
		const auto firstVertex = static_cast<std::uint32_t>(geometry.vertices.size());
		geometry.vertices.push_back({ minimumX, 0.0f, minimumZ });
		geometry.vertices.push_back({ maximumX, 0.0f, minimumZ });
		geometry.vertices.push_back({ maximumX, 0.0f, maximumZ });
		geometry.vertices.push_back({ minimumX, 0.0f, maximumZ });
		geometry.indices.insert(
			geometry.indices.end(),
			{
				firstVertex,
				firstVertex + 2,
				firstVertex + 1,
				firstVertex,
				firstVertex + 3,
				firstVertex + 2
			});
	}
}

TEST(Navigation, FlatPlaneBuildsAndFindsPath)
{
	navigation::World world;
	ASSERT_EQ(world.Build(MakePlane()), navigation::BuildStatus::Succeeded);
	EXPECT_TRUE(world.IsInitialized());

	const navigation::ProjectionResult projection = world.ProjectPoint(
		{ 2.0f, 1.0f, 2.0f },
		{ 1.0f, 2.0f, 1.0f });
	ASSERT_EQ(projection.status, navigation::QueryStatus::Succeeded);

	const navigation::PathResult path = world.FindPath({
		{ 2.0f, 0.0f, 2.0f },
		{ 8.0f, 0.0f, 8.0f },
		{ 1.0f, 2.0f, 1.0f }
	});
	ASSERT_EQ(path.status, navigation::QueryStatus::Succeeded);
	ASSERT_GE(path.points.size(), 2u);
	EXPECT_NEAR(path.points.front().x, 2.0f, 0.25f);
	EXPECT_NEAR(path.points.back().z, 8.0f, 0.25f);
}

TEST(Navigation, UninitializedWorldHasNoDebugGeometry)
{
	navigation::World world;
	EXPECT_TRUE(world.BuildDebugGeometry().lines.empty());
}

TEST(Navigation, LevelMeshConversionTransformsAndRebasesGeometry)
{
	auto firstCpu = std::make_shared<rendern::MeshCPU>();
	firstCpu->vertices = {
		{ 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }
	};
	firstCpu->indices = { 0, 1, 2 };
	auto secondCpu = std::make_shared<rendern::MeshCPU>(*firstCpu);
	auto first = std::make_shared<rendern::MeshResource>();
	auto second = std::make_shared<rendern::MeshResource>();
	first->SetCpuGeometry(firstCpu);
	second->SetCpuGeometry(secondCpu);
	first->SetLoadState(ResourceState::Loaded);
	second->SetLoadState(ResourceState::Loaded);

	const std::vector<rendern::LevelStaticMeshSource> sources = {
		{ first, mathUtils::Mat4(1.0f), 0 },
		{ second, mathUtils::Translate(mathUtils::Mat4(1.0f), { 5.0f, 2.0f, -1.0f }), 1 }
	};
	const auto result = app::navigationRuntime::BuildNavigationGeometry(sources);
	ASSERT_EQ(result.status, app::navigationRuntime::GeometryStatus::Ready);
	ASSERT_EQ(result.geometry.vertices.size(), 6u);
	EXPECT_EQ(result.geometry.indices, (std::vector<std::uint32_t>{ 0, 1, 2, 3, 4, 5 }));
	EXPECT_FLOAT_EQ(result.geometry.vertices[3].x, 5.0f);
	EXPECT_FLOAT_EQ(result.geometry.vertices[3].y, 2.0f);
	EXPECT_FLOAT_EQ(result.geometry.vertices[3].z, -1.0f);
}

TEST(Navigation, LevelMeshConversionWaitsWithoutPublishingPartialGeometry)
{
	auto cpu = std::make_shared<rendern::MeshCPU>();
	cpu->vertices = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 } };
	cpu->indices = { 0, 1, 2 };
	auto ready = std::make_shared<rendern::MeshResource>();
	ready->SetCpuGeometry(cpu);
	ready->SetLoadState(ResourceState::Loaded);
	auto loading = std::make_shared<rendern::MeshResource>();
	loading->SetLoadState(ResourceState::Loading);
	const std::vector<rendern::LevelStaticMeshSource> sources = {
		{ ready, mathUtils::Mat4(1.0f), 0 },
		{ loading, mathUtils::Mat4(1.0f), 1 }
	};
	const auto result = app::navigationRuntime::BuildNavigationGeometry(sources);
	EXPECT_EQ(result.status, app::navigationRuntime::GeometryStatus::WaitingForMeshes);
	EXPECT_TRUE(result.geometry.vertices.empty());
	EXPECT_TRUE(result.geometry.indices.empty());
}

TEST(Navigation, LevelMeshConversionCorrectsOnlyMirroredWinding)
{
	auto cpu = std::make_shared<rendern::MeshCPU>();
	cpu->vertices = { { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 } };
	cpu->indices = { 0, 1, 2 };
	auto mesh = std::make_shared<rendern::MeshResource>();
	mesh->SetCpuGeometry(cpu);
	mesh->SetLoadState(ResourceState::Loaded);

	mathUtils::Mat4 positive = mathUtils::Translate(mathUtils::Mat4(1.0f), { 2.0f, 3.0f, 4.0f });
	positive = mathUtils::Rotate(positive, mathUtils::DegToRad(30.0f), { 0.0f, 1.0f, 0.0f });
	positive = mathUtils::Scale(positive, { 2.0f, 1.0f, 3.0f });
	const auto positiveResult = app::navigationRuntime::BuildNavigationGeometry(
		std::vector<rendern::LevelStaticMeshSource>{ { mesh, positive, 0 } });
	ASSERT_EQ(positiveResult.status, app::navigationRuntime::GeometryStatus::Ready);
	EXPECT_EQ(positiveResult.geometry.indices, (std::vector<std::uint32_t>{ 0, 1, 2 }));

	const mathUtils::Mat4 mirrored = mathUtils::Scale(mathUtils::Mat4(1.0f), { -1.0f, 1.0f, 1.0f });
	const auto mirroredResult = app::navigationRuntime::BuildNavigationGeometry(
		std::vector<rendern::LevelStaticMeshSource>{ { mesh, mirrored, 0 } });
	ASSERT_EQ(mirroredResult.status, app::navigationRuntime::GeometryStatus::Ready);
	EXPECT_EQ(mirroredResult.geometry.indices, (std::vector<std::uint32_t>{ 0, 2, 1 }));
	const auto& vertices = mirroredResult.geometry.vertices;
	const mathUtils::Vec3 normal = mathUtils::Cross(
		vertices[mirroredResult.geometry.indices[1]] - vertices[mirroredResult.geometry.indices[0]],
		vertices[mirroredResult.geometry.indices[2]] - vertices[mirroredResult.geometry.indices[0]]);
	EXPECT_GT(normal.y, 0.0f);
}

TEST(Navigation, FailedOrInconsistentMeshIsInvalidGeometry)
{
	auto failed = std::make_shared<rendern::MeshResource>();
	failed->SetLoadState(ResourceState::Failed);
	auto inconsistent = std::make_shared<rendern::MeshResource>();
	inconsistent->SetLoadState(ResourceState::Loaded);

	for (const auto& mesh : { failed, inconsistent })
	{
		const auto result = app::navigationRuntime::BuildNavigationGeometry(
			std::vector<rendern::LevelStaticMeshSource>{ { mesh, mathUtils::Mat4(1.0f), 0 } });
		EXPECT_EQ(result.status, app::navigationRuntime::GeometryStatus::InvalidGeometry);
		EXPECT_TRUE(result.geometry.vertices.empty());
	}
}

TEST(Navigation, BuiltNavMeshProducesDebugLinesAndResetClearsThem)
{
	navigation::World world;
	ASSERT_EQ(world.Build(MakePlane()), navigation::BuildStatus::Succeeded);
	EXPECT_FALSE(world.BuildDebugGeometry().lines.empty());

	world.Reset();
	EXPECT_TRUE(world.BuildDebugGeometry().lines.empty());
}

TEST(Navigation, PathDebugGeometryIsALineStrip)
{
	navigation::PathResult path;
	path.points = { { 1.0f, 2.0f, 3.0f }, { 4.0f, 5.0f, 6.0f }, { 7.0f, 8.0f, 9.0f } };
	navigation::DebugGeometry geometry;
	navigation::AppendPathDebugGeometry(path, geometry, 0x12345678u);

	ASSERT_EQ(geometry.lines.size(), 2u);
	EXPECT_FLOAT_EQ(geometry.lines[0].start.x, path.points[0].x);
	EXPECT_FLOAT_EQ(geometry.lines[0].start.y, path.points[0].y);
	EXPECT_FLOAT_EQ(geometry.lines[0].start.z, path.points[0].z);
	EXPECT_FLOAT_EQ(geometry.lines[0].end.x, path.points[1].x);
	EXPECT_FLOAT_EQ(geometry.lines[0].end.y, path.points[1].y);
	EXPECT_FLOAT_EQ(geometry.lines[0].end.z, path.points[1].z);
	EXPECT_EQ(geometry.lines[0].rgba, 0x12345678u);
	EXPECT_FLOAT_EQ(geometry.lines[1].start.x, path.points[1].x);
	EXPECT_FLOAT_EQ(geometry.lines[1].end.x, path.points[2].x);
}

TEST(Navigation, DisconnectedRegionsReturnNoPath)
{
	navigation::Geometry geometry = MakePlane();
	const navigation::Geometry secondIsland = MakePlane(20.0f, 30.0f);
	const auto firstSecondIslandVertex = static_cast<std::uint32_t>(geometry.vertices.size());
	geometry.vertices.insert(
		geometry.vertices.end(),
		secondIsland.vertices.begin(),
		secondIsland.vertices.end());
	for (const std::uint32_t index : secondIsland.indices)
	{
		geometry.indices.push_back(firstSecondIslandVertex + index);
	}

	navigation::World world;
	ASSERT_EQ(world.Build(geometry), navigation::BuildStatus::Succeeded);
	EXPECT_EQ(
		world.FindPath({
			{ 2.0f, 0.0f, 2.0f },
			{ 22.0f, 0.0f, 2.0f },
			{ 1.0f, 2.0f, 1.0f }
		}).status,
		navigation::QueryStatus::NoPath);
}

TEST(Navigation, ObstacleProducesDetourPath)
{
	// Four floor strips form a connected ring around the absent central square.
	navigation::Geometry geometry;
	AddHorizontalQuad(geometry, 0.0f, 4.0f, 0.0f, 10.0f);
	AddHorizontalQuad(geometry, 6.0f, 10.0f, 0.0f, 10.0f);
	AddHorizontalQuad(geometry, 4.0f, 6.0f, 0.0f, 4.0f);
	AddHorizontalQuad(geometry, 4.0f, 6.0f, 6.0f, 10.0f);

	navigation::BuildSettings settings;
	settings.agent.radius = 0.2f;
	settings.regionMinSize = 0.0f;
	settings.regionMergeSize = 0.0f;

	navigation::World world;
	ASSERT_EQ(world.Build(geometry, settings), navigation::BuildStatus::Succeeded);
	const navigation::PathResult path = world.FindPath({
		{ 2.0f, 0.0f, 5.0f },
		{ 8.0f, 0.0f, 5.0f },
		{ 1.0f, 2.0f, 1.0f }
	});

	ASSERT_EQ(path.status, navigation::QueryStatus::Succeeded);
	ASSERT_GT(path.points.size(), 2u);

	bool leavesDirectCorridor = false;
	for (const mathUtils::Vec3& point : path.points)
	{
		if (point.z < 4.5f || point.z > 5.5f)
		{
			leavesDirectCorridor = true;
			break;
		}
	}
	EXPECT_TRUE(leavesDirectCorridor);
}

TEST(Navigation, InvalidGeometryAndSettingsAreRejectedAndClearState)
{
	navigation::World world;
	ASSERT_EQ(world.Build(MakePlane()), navigation::BuildStatus::Succeeded);
	EXPECT_EQ(world.Build({}), navigation::BuildStatus::InvalidGeometry);
	EXPECT_FALSE(world.IsInitialized());

	navigation::Geometry geometry = MakePlane();
	geometry.indices.push_back(0);
	EXPECT_EQ(world.Build(geometry), navigation::BuildStatus::InvalidGeometry);

	geometry = MakePlane();
	geometry.indices[0] = 99;
	EXPECT_EQ(world.Build(geometry), navigation::BuildStatus::InvalidGeometry);

	geometry = MakePlane();
	geometry.vertices[0].x = std::numeric_limits<float>::infinity();
	EXPECT_EQ(world.Build(geometry), navigation::BuildStatus::InvalidGeometry);

	navigation::BuildSettings settings;
	settings.cellSize = 0.0f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.cellHeight = 0.0f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.agent.radius = 0.0f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.agent.height = 0.0f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.agent.maximumStepHeight = -0.1f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.agent.maximumStepHeight = settings.agent.height;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);
	
	settings = {};
	settings.agent.maximumSlopeAngleDegrees = 90.0f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.edgeMaxError = std::numeric_limits<float>::infinity();
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);

	settings = {};
	settings.agent.height = static_cast<float>(std::numeric_limits<int>::max());
	settings.cellHeight = 0.5f;
	EXPECT_EQ(world.Build(MakePlane(), settings), navigation::BuildStatus::InvalidSettings);
	EXPECT_FALSE(world.IsInitialized());
}

TEST(Navigation, ProfileRegistryReusesCompatibleBakeAndSeparatesNarrowPassageAgents)
{
	// Two rooms are connected only by a 1.2 m wide floor strip. Recast erosion removes
	// that strip from the large-agent bake while preserving it for the small agent.
	navigation::Geometry geometry;
	AddHorizontalQuad(geometry, 0.0f, 4.0f, 0.0f, 6.0f);
	AddHorizontalQuad(geometry, 4.0f, 8.0f, 2.4f, 3.6f);
	AddHorizontalQuad(geometry, 8.0f, 12.0f, 0.0f, 6.0f);

	navigation::BuildSettings smallSettings;
	smallSettings.agent = { .radius = 0.2f, .height = 1.2f, .maximumStepHeight = 0.2f,
		.maximumSlopeAngleDegrees = 40.0f };
	smallSettings.cellSize = 0.1f;
	smallSettings.regionMinSize = 0.0f;
	smallSettings.regionMergeSize = 0.0f;
	navigation::BuildSettings largeSettings = smallSettings;
	largeSettings.agent.radius = 0.7f;
	largeSettings.agent.height = 2.2f;

	navigation::ProfileRegistry profiles;
	const navigation::ProfileResolution smallProfile = profiles.Initialize(geometry, smallSettings);
	ASSERT_EQ(smallProfile.status, navigation::BuildStatus::Succeeded);
	const navigation::ProfileResolution reusedSmallProfile =
		profiles.ResolveProfile(smallSettings.agent);
	ASSERT_EQ(reusedSmallProfile.status, navigation::BuildStatus::Succeeded);
	EXPECT_EQ(reusedSmallProfile.profile, smallProfile.profile);
	EXPECT_EQ(profiles.GetProfileCount(), 1u);

	const navigation::ProfileResolution largeProfile = profiles.ResolveProfile(largeSettings.agent);
	ASSERT_EQ(largeProfile.status, navigation::BuildStatus::Succeeded);
	EXPECT_NE(largeProfile.profile, smallProfile.profile);
	EXPECT_EQ(profiles.GetProfileCount(), 2u);
	const navigation::PathRequest request{
		.start = { 2.0f, 0.0f, 3.0f },
		.goal = { 10.0f, 0.0f, 3.0f },
		.searchExtents = { 1.0f, 2.0f, 1.0f }
	};
	ASSERT_NE(profiles.TryGetWorld(smallProfile.profile), nullptr);
	ASSERT_NE(profiles.TryGetWorld(largeProfile.profile), nullptr);
	EXPECT_EQ(profiles.TryGetWorld(smallProfile.profile)->FindPath(request).status,
		navigation::QueryStatus::Succeeded);
	EXPECT_EQ(profiles.TryGetWorld(largeProfile.profile)->FindPath(request).status,
		navigation::QueryStatus::NoPath);
}

TEST(Navigation, ResetAndRebuildReplaceState)
{
	navigation::World world;
	ASSERT_EQ(world.Build(MakePlane()), navigation::BuildStatus::Succeeded);

	world.Reset();
	world.Reset();
	EXPECT_FALSE(world.IsInitialized());

	ASSERT_EQ(world.Build(MakePlane(20.0f, 30.0f)), navigation::BuildStatus::Succeeded);
	EXPECT_EQ(
		world.ProjectPoint({ 2.0f, 0.0f, 2.0f }, { 1.0f, 2.0f, 1.0f }).status,
		navigation::QueryStatus::StartNotOnNavMesh);
	EXPECT_EQ(
		world.ProjectPoint({ 22.0f, 0.0f, 2.0f }, { 1.0f, 2.0f, 1.0f }).status,
		navigation::QueryStatus::Succeeded);
}

TEST(Navigation, MovedFromWorldCanBeResetAndRebuilt)
{
	navigation::World source;
	ASSERT_EQ(source.Build(MakePlane()), navigation::BuildStatus::Succeeded);

	navigation::World destination(std::move(source));
	EXPECT_TRUE(destination.IsInitialized());

	source.Reset();
	EXPECT_FALSE(source.IsInitialized());
	EXPECT_EQ(source.Build(MakePlane(20.0f, 30.0f)), navigation::BuildStatus::Succeeded);
	EXPECT_TRUE(source.IsInitialized());
}

TEST(Navigation, SlopeLimitAffectsWalkability)
{
	navigation::Geometry geometry{
		{
			{ 0.0f, 0.0f, 0.0f },
			{ 10.0f, 0.0f, 0.0f },
			{ 10.0f, 2.0f, 10.0f },
			{ 0.0f, 2.0f, 10.0f },
			{ 20.0f, 0.0f, 0.0f },
			{ 30.0f, 0.0f, 0.0f },
			{ 30.0f, 20.0f, 10.0f },
			{ 20.0f, 20.0f, 10.0f }
		},
		{ 0, 2, 1, 0, 3, 2, 4, 6, 5, 4, 7, 6 }
	};

	navigation::BuildSettings settings;
	settings.agent.maximumSlopeAngleDegrees = 30.0f;

	navigation::World world;
	ASSERT_EQ(world.Build(geometry, settings), navigation::BuildStatus::Succeeded);
	EXPECT_EQ(
		world.ProjectPoint({ 5.0f, 1.0f, 5.0f }, { 2.0f, 3.0f, 2.0f }).status,
		navigation::QueryStatus::Succeeded);
	EXPECT_EQ(
		world.ProjectPoint({ 25.0f, 10.0f, 5.0f }, { 2.0f, 3.0f, 2.0f }).status,
		navigation::QueryStatus::StartNotOnNavMesh);
}