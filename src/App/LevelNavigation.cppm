module;

#include <cstdint>
#include <limits>

export module core:level_navigation;

import :level;
import :navigation;
import :math_utils;
import std;

export namespace app::navigationRuntime
{
	enum class GeometryStatus
	{
		Ready,
		WaitingForMeshes,
		InvalidGeometry
	};

	struct GeometryResult
	{
		GeometryStatus status{ GeometryStatus::Ready };
		navigation::Geometry geometry;
		std::size_t sourceMeshCount{};
	};

	[[nodiscard]] float LinearDeterminant(const mathUtils::Mat4& matrix) noexcept
	{
		const mathUtils::Vec3 x{ matrix.columns[0].x, matrix.columns[0].y, matrix.columns[0].z };
		const mathUtils::Vec3 y{ matrix.columns[1].x, matrix.columns[1].y, matrix.columns[1].z };
		const mathUtils::Vec3 z{ matrix.columns[2].x, matrix.columns[2].y, matrix.columns[2].z };
		return mathUtils::Dot(x, mathUtils::Cross(y, z));
	}

	[[nodiscard]] GeometryResult BuildNavigationGeometry(
		std::span<const rendern::LevelStaticMeshSource> sources)
	{
		GeometryResult result{};
		std::size_t totalVertices{};
		std::size_t totalIndices{};
		bool waiting = false;

		// Validate every source before copying any geometry. Failure takes
		// precedence over loading so a broken level cannot remain pending forever.
		for (const rendern::LevelStaticMeshSource& source : sources)
		{
			if (!source.mesh)
			{
				result.status = GeometryStatus::InvalidGeometry;
				result.geometry = {};
				return result;
			}
			const ResourceState state = source.mesh->GetLoadState();
			if (state == ResourceState::Loading || state == ResourceState::Unloaded)
			{
				waiting = true;
				continue;
			}
			if (state != ResourceState::Loaded)
			{
				result.status = GeometryStatus::InvalidGeometry;
				return result;
			}
			const std::shared_ptr<const rendern::MeshCPU> cpu = source.mesh->GetCpuGeometry();
			if (!cpu || cpu->indices.empty() || cpu->indices.size() % 3 != 0
				|| cpu->vertices.size() > std::numeric_limits<std::uint32_t>::max() - totalVertices
				|| cpu->indices.size() > std::numeric_limits<std::size_t>::max() - totalIndices)
			{
				result.status = GeometryStatus::InvalidGeometry;
				return result;
			}
			for (const std::uint32_t index : cpu->indices)
			{
				if (index >= cpu->vertices.size())
				{
					result.status = GeometryStatus::InvalidGeometry;
					return result;
				}
			}
			totalVertices += cpu->vertices.size();
			totalIndices += cpu->indices.size();
		}
		if (waiting)
		{
			result.status = GeometryStatus::WaitingForMeshes;
			return result;
		}
		if (totalIndices == 0)
		{
			result.status = GeometryStatus::InvalidGeometry;
			return result;
		}

		result.geometry.vertices.reserve(totalVertices);
		result.geometry.indices.reserve(totalIndices);
		for (const rendern::LevelStaticMeshSource& source : sources)
		{
			const std::shared_ptr<const rendern::MeshCPU> cpu = source.mesh->GetCpuGeometry();

			const auto baseVertex = static_cast<std::uint32_t>(result.geometry.vertices.size());
			for (const rendern::VertexDesc& vertex : cpu->vertices)
			{
				result.geometry.vertices.push_back(mathUtils::TransformPoint(
					source.world, { vertex.px, vertex.py, vertex.pz }));
			}
			const bool mirrored = LinearDeterminant(source.world) < 0.0f;
			for (std::size_t triangle = 0; triangle < cpu->indices.size(); triangle += 3)
			{
				const std::uint32_t a = cpu->indices[triangle];
				const std::uint32_t b = cpu->indices[triangle + 1];
				const std::uint32_t c = cpu->indices[triangle + 2];
				if (mirrored)
				{
					result.geometry.indices.insert(result.geometry.indices.end(),
						{ baseVertex + a, baseVertex + c, baseVertex + b });
				}
				else
				{
					result.geometry.indices.insert(result.geometry.indices.end(),
						{ baseVertex + a, baseVertex + b, baseVertex + c });
				}
			}
			++result.sourceMeshCount;
		}
		return result;
	}

	[[nodiscard]] GeometryResult BuildLevelNavigationGeometry(const rendern::LevelInstance& level)
	{
		return BuildNavigationGeometry(level.GetStaticMeshSources());
	}
}