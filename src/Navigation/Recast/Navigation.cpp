module;

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <DebugDraw.h>
#include <DetourDebugDraw.h>
#include <Recast.h>

#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

module core;

import :math_utils;
import :navigation;

namespace navigation
{
	namespace
	{
		// Prevent coplanar NavMesh debug geometry from fighting with authored walkable surfaces.
		constexpr float NavMeshDebugVerticalOffset = 0.01f;

		class DebugGeometryAdapter final : public duDebugDraw
		{
		public:
			explicit DebugGeometryAdapter(DebugGeometry& geometry) : geometry_(geometry) {}

			void depthMask(bool) override {}
			void texture(bool) override {}

			void begin(duDebugDrawPrimitives primitive, float = 1.0f) override
			{
				primitive_ = primitive;
				vertices_.clear();
			}

			void vertex(const float* position, unsigned int color) override
			{
				vertices_.push_back({
					{ position[0], position[1] + NavMeshDebugVerticalOffset, position[2] }, color });
			}

			void vertex(float x, float y, float z, unsigned int color) override
			{
				vertices_.push_back({ { x, y + NavMeshDebugVerticalOffset, z }, color });
			}

			void vertex(const float* position, unsigned int color, const float*) override
			{
				vertex(position, color);
			}

			void vertex(float x, float y, float z, unsigned int color, float, float) override
			{
				vertex(x, y, z, color);
			}

			void end() override
			{
				switch (primitive_)
				{
				case DU_DRAW_LINES:
					for (std::size_t i = 0; i + 1 < vertices_.size(); i += 2)
					{
						AddLine(i, i + 1);
					}
					break;
				case DU_DRAW_TRIS:
					for (std::size_t i = 0; i + 2 < vertices_.size(); i += 3)
					{
						AddTriangle(i, i + 1, i + 2);
					}
					break;
				case DU_DRAW_QUADS:
					for (std::size_t i = 0; i + 3 < vertices_.size(); i += 4)
					{
						AddTriangle(i, i + 1, i + 2);
						AddTriangle(i, i + 2, i + 3);
						AddLine(i, i + 1);
						AddLine(i + 1, i + 2);
						AddLine(i + 2, i + 3);
						AddLine(i + 3, i);
					}
					break;
				case DU_DRAW_POINTS:
					break;
				}
				vertices_.clear();
			}

		private:
			struct Vertex
			{
				mathUtils::Vec3 position{};
				unsigned int color{};
			};

			void AddLine(std::size_t start, std::size_t end)
			{
				geometry_.lines.push_back({ vertices_[start].position, vertices_[end].position, vertices_[start].color });
			}
			
			void AddTriangle(std::size_t a, std::size_t b, std::size_t c)
			{
				geometry_.triangles.push_back({
					vertices_[a].position, vertices_[b].position, vertices_[c].position,
					vertices_[a].color, vertices_[b].color, vertices_[c].color });
			}

			DebugGeometry& geometry_;
			duDebugDrawPrimitives primitive_{ DU_DRAW_LINES };
			std::vector<Vertex> vertices_;
		};
		
		template<class T, void(*Free)(T*)>
		struct RecastDeleter
		{
			void operator()(T* pointer) const
			{
				if (pointer != nullptr)
				{
					Free(pointer);
				}
			}
		};

		template<class T, void(*Free)(T*)>
		using RecastPtr = std::unique_ptr<T, RecastDeleter<T, Free>>;

		constexpr unsigned char WalkableArea = 1;
		constexpr unsigned short WalkableFlag = 1;
		constexpr int MaximumPathPolygons = 2048;
		constexpr int MaximumStraightPathPoints = 2048;

		bool AreBuildSettingsValid(const BuildSettings& settings)
		{
			return settings.agent.IsValid() &&
				std::isfinite(settings.cellSize) &&
				settings.cellSize > 0.0f &&
				std::isfinite(settings.cellHeight) &&
				settings.cellHeight > 0.0f &&
				std::isfinite(settings.regionMinSize) &&
				settings.regionMinSize >= 0.0f &&
				std::isfinite(settings.regionMergeSize) &&
				settings.regionMergeSize >= 0.0f &&
				std::isfinite(settings.edgeMaxLength) &&
				settings.edgeMaxLength >= 0.0f &&
				std::isfinite(settings.edgeMaxError) &&
				settings.edgeMaxError > 0.0f &&
				settings.verticesPerPolygon >= 3 &&
				settings.verticesPerPolygon <= DT_VERTS_PER_POLYGON &&
				std::isfinite(settings.detailSampleDistance) &&
				settings.detailSampleDistance >= 0.0f &&
				std::isfinite(settings.detailSampleMaxError) &&
				settings.detailSampleMaxError >= 0.0f;
		}
	}
	
	bool AgentSettings::IsValid() const noexcept
	{
		return std::isfinite(radius) && radius > 0.0f &&
			std::isfinite(height) && height > 0.0f &&
			std::isfinite(maximumStepHeight) && maximumStepHeight >= 0.0f &&
			maximumStepHeight < height &&
			std::isfinite(maximumSlopeAngleDegrees) &&
			maximumSlopeAngleDegrees >= 0.0f && maximumSlopeAngleDegrees < 90.0f;
	}

	namespace
	{
		[[nodiscard]] bool AreAgentSettingsEqual(
			const AgentSettings& left, const AgentSettings& right) noexcept
		{
			return left.radius == right.radius && left.height == right.height &&
				left.maximumStepHeight == right.maximumStepHeight &&
				left.maximumSlopeAngleDegrees == right.maximumSlopeAngleDegrees;
		}
	}

	struct World::Impl
	{
		RecastPtr<dtNavMesh, dtFreeNavMesh> mesh{ nullptr };
		RecastPtr<dtNavMeshQuery, dtFreeNavMeshQuery> query{ nullptr };
	};

	World::World()
		: impl_(std::make_unique<Impl>())
	{
	}

	World::~World() = default;
	World::World(World&&) noexcept = default;
	World& World::operator=(World&&) noexcept = default;

	void World::Reset() noexcept
	{
		if (impl_ == nullptr)
		{
			return;
		}

		impl_->query.reset();
		impl_->mesh.reset();
	}

	bool World::IsInitialized() const noexcept
	{
		return impl_ != nullptr && impl_->mesh != nullptr && impl_->query != nullptr;
	}
	
	DebugGeometry World::BuildDebugGeometry() const
	{
		DebugGeometry geometry;
		if (!IsInitialized())
		{
			return geometry;
		}

		DebugGeometryAdapter adapter(geometry);
		duDebugDrawNavMesh(&adapter, *impl_->mesh, 0);
		return geometry;
	}

	void AppendPathDebugGeometry(const PathResult& path, DebugGeometry& geometry, std::uint32_t rgba)
	{
		for (std::size_t i = 1; i < path.points.size(); ++i)
		{
			geometry.lines.push_back({ path.points[i - 1], path.points[i], rgba });
		}
	}

	BuildStatus World::Build(const Geometry& geometry, const BuildSettings& settings)
	{
		if (impl_ == nullptr)
		{
			impl_ = std::make_unique<Impl>();
		}

		Reset();

		if (geometry.vertices.empty() || geometry.indices.empty() || geometry.indices.size() % 3 != 0)
		{
			return BuildStatus::InvalidGeometry;
		}

		for (const mathUtils::Vec3& vertex : geometry.vertices)
		{
			if (!IsFinite(vertex))
			{
				return BuildStatus::InvalidGeometry;
			}
		}

		for (const std::uint32_t index : geometry.indices)
		{
			if (index >= geometry.vertices.size())
			{
				return BuildStatus::InvalidGeometry;
			}
		}

		if (!AreBuildSettingsValid(settings))
		{
			return BuildStatus::InvalidSettings;
		}

		std::vector<float> vertices;
		vertices.reserve(geometry.vertices.size() * 3);
		for (const mathUtils::Vec3& vertex : geometry.vertices)
		{
			vertices.push_back(vertex.x);
			vertices.push_back(vertex.y);
			vertices.push_back(vertex.z);
		}

		std::vector<int> triangles;
		triangles.reserve(geometry.indices.size());
		for (const std::uint32_t index : geometry.indices)
		{
			triangles.push_back(static_cast<int>(index));
		}

		// Configure the voxelization and polygon-generation stages.
		rcConfig config{};
		config.cs = settings.cellSize;
		config.ch = settings.cellHeight;
		config.walkableSlopeAngle = settings.agent.maximumSlopeAngleDegrees;
		if (!mathUtils::TryConvertToInt(std::ceil(settings.agent.height / config.ch), config.walkableHeight) ||
			!mathUtils::TryConvertToInt(std::floor(settings.agent.maximumStepHeight / config.ch), config.walkableClimb) ||
			!mathUtils::TryConvertToInt(std::ceil(settings.agent.radius / config.cs), config.walkableRadius) ||
			!mathUtils::TryConvertToInt(settings.edgeMaxLength / config.cs, config.maxEdgeLen) ||
			!mathUtils::TryConvertToInt(settings.regionMinSize * settings.regionMinSize, config.minRegionArea) ||
			!mathUtils::TryConvertToInt(settings.regionMergeSize * settings.regionMergeSize, config.mergeRegionArea))
		{
			return BuildStatus::InvalidSettings;
		}
		config.maxSimplificationError = settings.edgeMaxError;
		config.maxVertsPerPoly = settings.verticesPerPolygon;
		config.detailSampleDist = settings.detailSampleDistance < 0.9f
			? 0.0f
			: config.cs * settings.detailSampleDistance;
		config.detailSampleMaxError = config.ch * settings.detailSampleMaxError;

		rcCalcBounds(
			vertices.data(),
			static_cast<int>(geometry.vertices.size()),
			config.bmin,
			config.bmax);
		rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);
		if (config.width <= 0 || config.height <= 0)
		{
			return BuildStatus::BuildFailed;
		}

		rcContext context;

		// Rasterize walkable triangles into the initial heightfield.
		RecastPtr<rcHeightfield, rcFreeHeightField> heightfield(rcAllocHeightfield());
		if (heightfield == nullptr ||
			!rcCreateHeightfield(
				&context,
				*heightfield,
				config.width,
				config.height,
				config.bmin,
				config.bmax,
				config.cs,
				config.ch))
		{
			return BuildStatus::BuildFailed;
		}

		const int triangleCount = static_cast<int>(triangles.size() / 3);
		std::vector<unsigned char> triangleAreas(triangleCount);
		rcMarkWalkableTriangles(
			&context,
			config.walkableSlopeAngle,
			vertices.data(),
			static_cast<int>(geometry.vertices.size()),
			triangles.data(),
			triangleCount,
			triangleAreas.data());
		if (!rcRasterizeTriangles(
			&context,
			vertices.data(),
			static_cast<int>(geometry.vertices.size()),
			triangles.data(),
			triangleAreas.data(),
			triangleCount,
			*heightfield,
			config.walkableClimb))
		{
			return BuildStatus::BuildFailed;
		}

		rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightfield);
		rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightfield);
		rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightfield);

		// Compact the heightfield, erode it by the agent radius, and form regions.
		RecastPtr<rcCompactHeightfield, rcFreeCompactHeightfield> compactHeightfield(
			rcAllocCompactHeightfield());
		if (compactHeightfield == nullptr ||
			!rcBuildCompactHeightfield(
				&context,
				config.walkableHeight,
				config.walkableClimb,
				*heightfield,
				*compactHeightfield) ||
			!rcErodeWalkableArea(&context, config.walkableRadius, *compactHeightfield) ||
			!rcBuildDistanceField(&context, *compactHeightfield) ||
			!rcBuildRegions(
				&context,
				*compactHeightfield,
				0,
				config.minRegionArea,
				config.mergeRegionArea))
		{
			return BuildStatus::BuildFailed;
		}

		// Convert regions to contours, then to coarse and detailed polygon meshes.
		RecastPtr<rcContourSet, rcFreeContourSet> contours(rcAllocContourSet());
		if (contours == nullptr ||
			!rcBuildContours(
				&context,
				*compactHeightfield,
				config.maxSimplificationError,
				config.maxEdgeLen,
				*contours))
		{
			return BuildStatus::BuildFailed;
		}

		RecastPtr<rcPolyMesh, rcFreePolyMesh> polygonMesh(rcAllocPolyMesh());
		if (polygonMesh == nullptr ||
			!rcBuildPolyMesh(&context, *contours, config.maxVertsPerPoly, *polygonMesh) ||
			polygonMesh->npolys == 0)
		{
			return BuildStatus::BuildFailed;
		}

		RecastPtr<rcPolyMeshDetail, rcFreePolyMeshDetail> detailMesh(rcAllocPolyMeshDetail());
		if (detailMesh == nullptr ||
			!rcBuildPolyMeshDetail(
				&context,
				*polygonMesh,
				*compactHeightfield,
				config.detailSampleDist,
				config.detailSampleMaxError,
				*detailMesh))
		{
			return BuildStatus::BuildFailed;
		}

		for (int polygonIndex = 0; polygonIndex < polygonMesh->npolys; ++polygonIndex)
		{
			if (polygonMesh->areas[polygonIndex] == RC_WALKABLE_AREA)
			{
				polygonMesh->areas[polygonIndex] = WalkableArea;
				polygonMesh->flags[polygonIndex] = WalkableFlag;
			}
		}

		// Create the Detour data and initialize the runtime query objects.
		dtNavMeshCreateParams parameters{};
		parameters.verts = polygonMesh->verts;
		parameters.vertCount = polygonMesh->nverts;
		parameters.polys = polygonMesh->polys;
		parameters.polyAreas = polygonMesh->areas;
		parameters.polyFlags = polygonMesh->flags;
		parameters.polyCount = polygonMesh->npolys;
		parameters.nvp = polygonMesh->nvp;
		parameters.detailMeshes = detailMesh->meshes;
		parameters.detailVerts = detailMesh->verts;
		parameters.detailVertsCount = detailMesh->nverts;
		parameters.detailTris = detailMesh->tris;
		parameters.detailTriCount = detailMesh->ntris;
		parameters.walkableHeight = settings.agent.height;
		parameters.walkableRadius = settings.agent.radius;
		parameters.walkableClimb = settings.agent.maximumStepHeight;
		rcVcopy(parameters.bmin, polygonMesh->bmin);
		rcVcopy(parameters.bmax, polygonMesh->bmax);
		parameters.cs = config.cs;
		parameters.ch = config.ch;
		parameters.buildBvTree = true;

		unsigned char* navMeshData = nullptr;
		int navMeshDataSize = 0;
		if (!dtCreateNavMeshData(&parameters, &navMeshData, &navMeshDataSize))
		{
			return BuildStatus::BuildFailed;
		}

		RecastPtr<dtNavMesh, dtFreeNavMesh> mesh(dtAllocNavMesh());
		if (mesh == nullptr)
		{
			dtFree(navMeshData);
			return BuildStatus::BuildFailed;
		}

		if (dtStatusFailed(mesh->init(navMeshData, navMeshDataSize, DT_TILE_FREE_DATA)))
		{
			dtFree(navMeshData);
			return BuildStatus::BuildFailed;
		}

		RecastPtr<dtNavMeshQuery, dtFreeNavMeshQuery> query(dtAllocNavMeshQuery());
		if (query == nullptr || dtStatusFailed(query->init(mesh.get(), MaximumPathPolygons)))
		{
			return BuildStatus::BuildFailed;
		}

		impl_->mesh = std::move(mesh);
		impl_->query = std::move(query);
		return BuildStatus::Succeeded;
	}

	ProjectionResult World::ProjectPoint(
		const mathUtils::Vec3& position,
		const mathUtils::Vec3& searchExtents) const
	{
		if (!IsInitialized())
		{
			return {};
		}

		if (!IsFinite(position) || !IsFinite(searchExtents) ||
			searchExtents.x <= 0.0f || searchExtents.y <= 0.0f || searchExtents.z <= 0.0f)
		{
			return { QueryStatus::InvalidRequest, {} };
		}

		const float backendPosition[3]{ position.x, position.y, position.z };
		const float backendExtents[3]{ searchExtents.x, searchExtents.y, searchExtents.z };
		float nearestPosition[3]{};
		dtPolyRef nearestPolygon = 0;
		dtQueryFilter filter;
		const dtStatus status = impl_->query->findNearestPoly(
			backendPosition,
			backendExtents,
			&filter,
			&nearestPolygon,
			nearestPosition);

		if (dtStatusFailed(status) || nearestPolygon == 0)
		{
			return { QueryStatus::StartNotOnNavMesh, {} };
		}

		return {
			QueryStatus::Succeeded,
			{ nearestPosition[0], nearestPosition[1], nearestPosition[2] }
		};
	}

	PathResult World::FindPath(const PathRequest& request) const
	{
		if (!IsInitialized())
		{
			return {};
		}

		if (!IsFinite(request.start) || !IsFinite(request.goal) || !IsFinite(request.searchExtents) ||
			request.searchExtents.x <= 0.0f ||
			request.searchExtents.y <= 0.0f ||
			request.searchExtents.z <= 0.0f)
		{
			return { QueryStatus::InvalidRequest, {} };
		}

		const float start[3]{ request.start.x, request.start.y, request.start.z };
		const float goal[3]{ request.goal.x, request.goal.y, request.goal.z };
		const float extents[3]{
			request.searchExtents.x,
			request.searchExtents.y,
			request.searchExtents.z
		};
		float nearestStart[3]{};
		float nearestGoal[3]{};
		dtPolyRef startPolygon = 0;
		dtPolyRef goalPolygon = 0;
		dtQueryFilter filter;

		const dtStatus startStatus = impl_->query->findNearestPoly(
			start,
			extents,
			&filter,
			&startPolygon,
			nearestStart);
		if (dtStatusFailed(startStatus) || startPolygon == 0)
		{
			return { QueryStatus::StartNotOnNavMesh, {} };
		}

		const dtStatus goalStatus = impl_->query->findNearestPoly(
			goal,
			extents,
			&filter,
			&goalPolygon,
			nearestGoal);
		if (dtStatusFailed(goalStatus) || goalPolygon == 0)
		{
			return { QueryStatus::GoalNotOnNavMesh, {} };
		}

		dtPolyRef corridor[MaximumPathPolygons]{};
		int corridorCount = 0;
		const dtStatus corridorStatus = impl_->query->findPath(
			startPolygon,
			goalPolygon,
			nearestStart,
			nearestGoal,
			&filter,
			corridor,
			&corridorCount,
			MaximumPathPolygons);

		if (dtStatusFailed(corridorStatus) || corridorCount == 0)
		{
			return { QueryStatus::NoPath, {} };
		}
		if (dtStatusDetail(corridorStatus, DT_BUFFER_TOO_SMALL))
		{
			return { QueryStatus::BufferTooSmall, {} };
		}
		if (corridor[corridorCount - 1] != goalPolygon)
		{
			return { QueryStatus::NoPath, {} };
		}

		float straightPath[MaximumStraightPathPoints * 3]{};
		unsigned char straightPathFlags[MaximumStraightPathPoints]{};
		dtPolyRef straightPathPolygons[MaximumStraightPathPoints]{};
		int straightPathCount = 0;
		const dtStatus straightPathStatus = impl_->query->findStraightPath(
			nearestStart,
			nearestGoal,
			corridor,
			corridorCount,
			straightPath,
			straightPathFlags,
			straightPathPolygons,
			&straightPathCount,
			MaximumStraightPathPoints);

		if (dtStatusFailed(straightPathStatus))
		{
			return { QueryStatus::NoPath, {} };
		}
		if (dtStatusDetail(straightPathStatus, DT_BUFFER_TOO_SMALL))
		{
			return { QueryStatus::BufferTooSmall, {} };
		}

		PathResult result{ QueryStatus::Succeeded, {} };
		result.points.reserve(straightPathCount);
		for (int pointIndex = 0; pointIndex < straightPathCount; ++pointIndex)
		{
			const int coordinateIndex = pointIndex * 3;
			result.points.emplace_back(
				straightPath[coordinateIndex],
				straightPath[coordinateIndex + 1],
				straightPath[coordinateIndex + 2]);
		}
		return result;
	}
	
	struct ProfileRegistry::Impl
	{
		struct Profile
		{
			AgentSettings agent{};
			std::unique_ptr<World> world;
		};

		Geometry geometry;
		BuildSettings baseSettings{};
		std::vector<Profile> profiles;
		bool initialized{ false };
	};

	ProfileRegistry::ProfileRegistry() : impl_(std::make_unique<Impl>()) {}
	ProfileRegistry::~ProfileRegistry() = default;
	ProfileRegistry::ProfileRegistry(ProfileRegistry&&) noexcept = default;
	ProfileRegistry& ProfileRegistry::operator=(ProfileRegistry&&) noexcept = default;

	ProfileResolution ProfileRegistry::Initialize(const Geometry& geometry, const BuildSettings& settings)
	{
		Reset();
		impl_->geometry = geometry;
		impl_->baseSettings = settings;
		impl_->initialized = true;
		const ProfileResolution resolution = ResolveProfile(settings.agent);
		if (resolution.status != BuildStatus::Succeeded)
		{
			Reset();
		}
		return resolution;
	}

	ProfileResolution ProfileRegistry::ResolveProfile(const AgentSettings& agent)
	{
		if (!impl_->initialized || !agent.IsValid())
		{
			return { BuildStatus::InvalidSettings, {} };
		}
		for (std::uint32_t index = 0; index < impl_->profiles.size(); ++index)
		{
			if (AreAgentSettingsEqual(impl_->profiles[index].agent, agent))
			{
				return { BuildStatus::Succeeded, { index } };
			}
		}

		BuildSettings settings = impl_->baseSettings;
		settings.agent = agent;
		auto world = std::make_unique<World>();
		const BuildStatus status = world->Build(impl_->geometry, settings);
		if (status != BuildStatus::Succeeded ||
			impl_->profiles.size() >= std::numeric_limits<std::uint32_t>::max())
		{
			return { status == BuildStatus::Succeeded ? BuildStatus::BuildFailed : status, {} };
		}
		const auto index = static_cast<std::uint32_t>(impl_->profiles.size());
		impl_->profiles.push_back({ agent, std::move(world) });
		return { BuildStatus::Succeeded, { index } };
	}

	const World* ProfileRegistry::TryGetWorld(const ProfileHandle profile) const noexcept
	{
		return profile.IsValid() && profile.value < impl_->profiles.size()
			? impl_->profiles[profile.value].world.get() : nullptr;
	}

	std::size_t ProfileRegistry::GetProfileCount() const noexcept { return impl_->profiles.size(); }

	void ProfileRegistry::Reset() noexcept
	{
		impl_->profiles.clear();
		impl_->geometry = {};
		impl_->baseSettings = {};
		impl_->initialized = false;
	}
}