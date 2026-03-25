module;

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

export module core:skeleton;

import :math_utils;

export namespace rendern
{
	struct SkeletonBone
	{
		std::string name;
		int parentIndex{ -1 };
		mathUtils::Mat4 inverseBindMatrix{ 1.0f };
		mathUtils::Mat4 bindLocalTransform{ 1.0f };
	};

	struct Skeleton
	{
		std::vector<SkeletonBone> bones;
		std::uint32_t rootBoneIndex{ 0 };
	};

	inline void RebuildBoneNameLookup(Skeleton& skeleton)
	{
		// No-op for now.
		//
		// We originally cached a std::unordered_map<std::string, uint32_t> here,
		// but MSVC modules currently trips over std::string equality during the
		// map's comparator instantiation in this exported module. A linear search is
		// perfectly acceptable for MVP skeletal animation and keeps the module
		// compileable. We keep the function so later patches can restore an
		// acceleration structure without touching call sites.
	}

	[[nodiscard]] inline std::optional<std::uint32_t> FindBoneIndex(const Skeleton& skeleton, std::string_view boneName)
	{
		for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(skeleton.bones.size()); ++i)
		{
			const std::string& candidate = skeleton.bones[i].name;
			if (candidate.size() == boneName.size()
				&& std::char_traits<char>::compare(candidate.data(), boneName.data(), boneName.size()) == 0)
			{
				return i;
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] inline bool IsValidSkeleton(const Skeleton& skeleton) noexcept
	{
		if (skeleton.bones.empty())
		{
			return false;
		}
		if (skeleton.rootBoneIndex >= skeleton.bones.size())
		{
			return false;
		}
		for (std::size_t i = 0; i < skeleton.bones.size(); ++i)
		{
			const int parentIndex = skeleton.bones[i].parentIndex;
			if (parentIndex >= static_cast<int>(skeleton.bones.size()))
			{
				return false;
			}
			if (parentIndex == static_cast<int>(i))
			{
				return false;
			}
		}
		return true;
	}
}