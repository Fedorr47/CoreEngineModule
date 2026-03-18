void SaveLevelAssetToJson(std::string_view levelRelativeOrAbsPath, const LevelAsset& level)
{
	namespace fs = std::filesystem;

	const AliveNodeRemap_ remap = BuildAliveNodeRemap_(level);

	const fs::path absPath = corefs::ResolveAsset(fs::path(std::string(levelRelativeOrAbsPath)));
	fs::create_directories(absPath.parent_path());

	std::ofstream file(absPath, std::ios::binary | std::ios::trunc);
	if (!file)
	{
		throw std::runtime_error("Level JSON: failed to open for write: " + absPath.string());
	}

	std::ostringstream ss;
	ss.setf(std::ios::fixed);
	ss << std::setprecision(6);

	ss << "{\n";
	ss << "  \"name\": ";
	WriteJsonEscaped(ss, level.name);
	ss << ",\n";

	WriteMeshesSection_(ss, level);
	WriteModelsSection_(ss, level);
	WriteSkinnedMeshesSection_(ss, level);
	WriteAnimationsSection_(ss, level);
	WriteAnimationControllerAssetsSection_(ss, level);
	WriteAnimationControllersSection_(ss, level);
	WriteTexturesSection_(ss, level);
	WriteMaterialsSection_(ss, level);
	WriteCameraSection_(ss, level);
	WriteLightsSection_(ss, level);
	WriteParticleEmittersSection_(ss, level);
	WriteSkyboxSection_(ss, level);
	WriteNodesSection_(ss, level, remap);

	ss << "}\n";

	const std::string outText = ss.str();
	file.write(outText.data(), static_cast<std::streamsize>(outText.size()));
	file.flush();
	if (!file)
	{
		throw std::runtime_error("Level JSON: failed to write: " + absPath.string());
	}
}
