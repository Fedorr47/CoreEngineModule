module;

#include <filesystem>

export module core:file_system;

export import :file_utils;

export namespace corefs
{
	using namespace FILE_UTILS;
	
	fs::path FindAssetRoot()
	{
		static fs::path cached;
		if (!cached.empty())
		{
			return cached;
		}

		fs::path curretnPath = fs::current_path();
		for (int i = 0; i < 10; ++i)
		{
			fs::path candidate = curretnPath / "assets";
			if (fs::exists(candidate) && fs::is_directory(candidate))
			{
				cached = candidate;
				return cached;
			}
			if (!curretnPath.has_parent_path())
			{
				break;
			}
			curretnPath = curretnPath.parent_path();
		}

		cached = fs::path("assets");
		return cached;
	}

	fs::path ResolveAsset(const fs::path& relative)
	{
		if (relative.is_absolute())
		{
			return relative;
		}
		return FindAssetRoot() / relative;
	}
}