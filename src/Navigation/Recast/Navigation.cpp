module;

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
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
			return std::isfinite(settings.agent.radius) &&
				settings.agent.radius > 0.0f &&
				std::isfinite(settings.agent.height) &&
				settings.agent.height > 0.0f &&
				std::isfinite(settings.agent.maximumStepHeight) &&
				settings.agent.maximumStepHeight >= 0.0f &&
				std::isfinite(settings.agent.maximumSlopeAngleDegrees) &&
				settings.agent.maximumSlopeAngleDegrees >= 0.0f &&
				settings.agent.maximumSlopeAngleDegrees < 90.0f &&
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
}