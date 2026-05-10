LevelAsset LoadLevelAssetFromJson(std::string_view levelRelativePath)
{
	const std::filesystem::path absPath = corefs::ResolveAsset(std::filesystem::path(std::string(levelRelativePath)));
	const std::string text = FILE_UTILS::ReadAllText(absPath);

	JsonParser parser(text);
	JsonValue root = parser.Parse();
	if (!root.IsObject())
	{
		throw std::runtime_error("Level JSON: root must be object");
	}
	const JsonObject& jsonObject = root.AsObject();

	LevelAsset out;
	out.name = GetStringOpt(jsonObject, "name", "Level");
	out.sourcePath = std::string(levelRelativePath);

	ParseMeshSection_(out, jsonObject);
	ParseModelSection_(out, jsonObject);
	ParseTextureSection_(out, jsonObject);
	ParseAnimationSection_(out, jsonObject);
	ParseExternalAnimationControllerAssetSection_(out, jsonObject);
	ParseAnimationControllerSection_(out, jsonObject);
	ParseSkinnedMeshSection_(out, jsonObject);
	ParseMaterialSection_(out, jsonObject);
	ParseCameraSection_(out, jsonObject);
	ParseLightSection_(out, jsonObject);
	ParseParticleEmitterSection_(out, jsonObject);
	ParseSkyboxSection_(out, jsonObject);
	ParseNodeSection_(out, jsonObject);

	return out;
}
