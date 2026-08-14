AnimationProfileAsset LoadAnimationProfileAssetFromJson(std::string_view path, std::string_view id)
{
    return LoadExternalAnimationProfileAssetFromJson_(path, std::string(id));
}

void SaveAnimationProfileAssetToJson(std::string_view path, const AnimationProfileAsset& profile)
{
    if (profile.id.empty())
    {
        throw std::runtime_error("Animation profile JSON: profile id must not be empty");
    }

    // Validate and produce the complete payload before opening the destination. This
    // preserves an existing asset when authored data cannot be serialized safely.
    std::ostringstream stream;
    stream << "{\n  \"motions\": {";
    bool first = true;
    for (const std::string& motion : SortedStringKeys(profile.motions))
    {
        const AnimationClipRef& binding = profile.motions.at(motion);
        if (motion.empty())
        {
            throw std::runtime_error("Animation profile JSON: motion id must not be empty");
        }
        if (binding.sourceAssetId.empty())
        {
            throw std::runtime_error("Animation profile JSON: motion '" + motion + "'.sourceAssetId is required");
        }
        stream << (first ? "\n" : ",\n") << "    ";
        first = false;
        WriteJsonEscaped(stream, motion);
        stream << ": {\"sourceAssetId\": ";
        WriteJsonEscaped(stream, binding.sourceAssetId);
        stream << ", \"clip\": ";
        WriteJsonEscaped(stream, binding.clipName);
        stream << "}";
    }
    if (!first) stream << "\n  ";
    stream << "}\n}\n";
    const std::string text = stream.str();

    const std::filesystem::path absolutePath = corefs::ResolveAsset(std::filesystem::path(std::string(path)));
    std::filesystem::create_directories(absolutePath.parent_path());
    std::ofstream file(absolutePath, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Animation profile JSON: failed to open for write: " + absolutePath.string());
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.flush();
    if (!file) throw std::runtime_error("Animation profile JSON: failed to write: " + absolutePath.string());
}