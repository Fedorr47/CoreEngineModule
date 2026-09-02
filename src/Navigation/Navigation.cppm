module;

#include <cstdint>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

export module core:navigation;

import :math_utils;

export namespace navigation
{
	// Triangle soup is expressed in CoreEngine world space (right-handed, Y-up).
	struct Geometry
	{
		std::vector<mathUtils::Vec3> vertices;
		std::vector<std::uint32_t> indices;
	};

	struct AgentSettings
	{
		float radius{ 0.4f };
		float height{ 1.8f };
		float maximumStepHeight{ 0.4f };
		float maximumSlopeAngleDegrees{ 45.0f };
		
		[[nodiscard]] bool IsValid() const noexcept;
	};

	struct BuildSettings
	{
		AgentSettings agent{};
		float cellSize{ 0.2f };
		float cellHeight{ 0.1f };
		float regionMinSize{ 2.0f };
		float regionMergeSize{ 8.0f };
		float edgeMaxLength{ 12.0f };
		float edgeMaxError{ 1.3f };
		int verticesPerPolygon{ 6 };
		float detailSampleDistance{ 6.0f };
		float detailSampleMaxError{ 1.0f };
	};

	enum class BuildStatus
	{
		Succeeded,
		InvalidGeometry,
		InvalidSettings,
		BuildFailed
	};

	enum class QueryStatus
	{
		Succeeded,
		NotInitialized,
		InvalidRequest,
		StartNotOnNavMesh,
		GoalNotOnNavMesh,
		NoPath,
		BufferTooSmall
	};

	struct ProjectionResult
	{
		QueryStatus status{ QueryStatus::NotInitialized };
		mathUtils::Vec3 position{};
	};

	struct PathRequest
	{
		mathUtils::Vec3 start{};
		mathUtils::Vec3 goal{};
		mathUtils::Vec3 searchExtents{ 2.0f, 4.0f, 2.0f };
	};

	struct PathResult
	{
		QueryStatus status{ QueryStatus::NotInitialized };
		std::vector<mathUtils::Vec3> points;
	};
	
	struct DebugLine
	{
		mathUtils::Vec3 start{};
		mathUtils::Vec3 end{};
		std::uint32_t rgba{};
	};
	
	struct DebugTriangle
	{
		mathUtils::Vec3 a{};
		mathUtils::Vec3 b{};
		mathUtils::Vec3 c{};
		std::uint32_t rgbaA{};
		std::uint32_t rgbaB{};
		std::uint32_t rgbaC{};
	};

	struct DebugGeometry
	{
		std::vector<DebugLine> lines;
		std::vector<DebugTriangle> triangles;
	};

	// Appends a line strip for a supplied path; Navigation does not retain debug paths.
	void AppendPathDebugGeometry(
		const PathResult& path,
		DebugGeometry& geometry,
		std::uint32_t rgba = 0xff00ffffu);

	class World final
	{
	public:
		World();
		~World();

		World(World&&) noexcept;
		World& operator=(World&&) noexcept;

		World(const World&) = delete;
		World& operator=(const World&) = delete;

		[[nodiscard]] BuildStatus Build(const Geometry&, const BuildSettings& = {});
		void Reset() noexcept;
		[[nodiscard]] bool IsInitialized() const noexcept;
		[[nodiscard]] ProjectionResult ProjectPoint(
			const mathUtils::Vec3&,
			const mathUtils::Vec3& searchExtents) const;
		[[nodiscard]] PathResult FindPath(const PathRequest&) const;
		[[nodiscard]] DebugGeometry BuildDebugGeometry() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};
	
	// Handles are valid only while the owning registry retains its current contents.
	struct ProfileHandle
	{
		std::uint32_t value{ std::numeric_limits<std::uint32_t>::max() };

		[[nodiscard]] constexpr bool IsValid() const noexcept
		{
			return value != std::numeric_limits<std::uint32_t>::max();
		}
		[[nodiscard]] constexpr bool operator==(const ProfileHandle&) const noexcept = default;
	};

	struct ProfileResolution
	{
		BuildStatus status{ BuildStatus::BuildFailed };
		ProfileHandle profile{};
	};

	// Owns the shared source geometry and one Recast bake per distinct agent configuration.
	class ProfileRegistry final
	{
	public:
		ProfileRegistry();
		~ProfileRegistry();
		ProfileRegistry(ProfileRegistry&&) noexcept;
		ProfileRegistry& operator=(ProfileRegistry&&) noexcept;
		ProfileRegistry(const ProfileRegistry&) = delete;
		ProfileRegistry& operator=(const ProfileRegistry&) = delete;

		[[nodiscard]] ProfileResolution Initialize(const Geometry&, const BuildSettings& = {});
		[[nodiscard]] ProfileResolution ResolveProfile(const AgentSettings&);
		[[nodiscard]] const World* TryGetWorld(ProfileHandle) const noexcept;
		[[nodiscard]] std::size_t GetProfileCount() const noexcept;
		void Reset() noexcept;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};
}