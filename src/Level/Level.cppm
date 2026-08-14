module;

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <array>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <cassert>
#include <type_traits>

export module core:level;
import :scene; 
import :level_ecs; 
import :asset_manager; 
import :resource_manager; 
import :render_bindless; 
import :file_system; 
import :math_utils;
import :string_utils;
import :json_utils;
import :assimp_scene_loader;
import :assimp_loader;
import :animator;
import :animation_controller;
import :animation_clip;
import :physics_types;

// ------------------------------------------------------------
// LevelAsset / LevelInstance

#include "LevelImpl/Level_JsonSupport.inl"

export namespace rendern
{

#include "LevelImpl/Level_AssetTypes.inl"

	class LevelInstance
	{
	public:
		LevelInstance() = default;


#include "LevelImpl/Level_LevelInstance_RuntimeBindings.inl"

#include "LevelImpl/Level_LevelInstance_Editing.inl"

	private:
		friend LevelInstance InstantiateLevel(Scene& scene, AssetManager& assets, BindlessTable& bindless, const LevelAsset& asset, const mathUtils::Mat4& root);


#include "LevelImpl/Level_LevelInstance_PrivateHelpers.inl"

#include "LevelImpl/Level_LevelInstance_State.inl"
	};

	[[nodiscard]] LevelAsset LoadLevelAssetFromJson(std::string_view levelRelativePath);
	[[nodiscard]] LevelInstance InstantiateLevel(Scene& scene, AssetManager& assets, BindlessTable& bindless, const LevelAsset& asset, const mathUtils::Mat4& root);
	void SaveLevelAssetToJson(std::string_view levelRelativeOrAbsPath, const LevelAsset& level);
	[[nodiscard]] AnimationProfileAsset LoadAnimationProfileAssetFromJson(std::string_view path, std::string_view id);
	void SaveAnimationProfileAssetToJson(std::string_view path, const AnimationProfileAsset& profile);
}

namespace rendern
{
#include "../Animation/Serialization/Level_LoadJson_Animation.inl"

#include "LevelImpl/Level_LoadJson_AssetSections.inl"

#include "LevelImpl/Level_LoadJson_SceneSections.inl"

#include "LevelImpl/Level_LoadJson.inl"

#include "LevelImpl/Level_InstantiateRuntime.inl"
#include "../Animation/Serialization/Level_SaveJson_Animation.inl"

#include "LevelImpl/Level_SaveJson_Support.inl"

#include "LevelImpl/Level_SaveJson_AssetSections.inl"

#include "LevelImpl/Level_SaveJson_SceneSections.inl"

#include "LevelImpl/Level_SaveJson.inl"
#include "../Animation/Serialization/AnimationProfileJson.inl"
}

