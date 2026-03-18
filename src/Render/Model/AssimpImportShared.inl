namespace rendern::assimp_detail
{
    [[nodiscard]] inline mathUtils::Mat4 AiToMat4(const aiMatrix4x4& m)
    {
        mathUtils::Mat4 out{ 1.0f };
        out(0, 0) = m.a1; out(0, 1) = m.a2; out(0, 2) = m.a3; out(0, 3) = m.a4;
        out(1, 0) = m.b1; out(1, 1) = m.b2; out(1, 2) = m.b3; out(1, 3) = m.b4;
        out(2, 0) = m.c1; out(2, 1) = m.c2; out(2, 2) = m.c3; out(2, 3) = m.c4;
        out(3, 0) = m.d1; out(3, 1) = m.d2; out(3, 2) = m.d3; out(3, 3) = m.d4;
        return out;
    }

    [[nodiscard]] inline mathUtils::Vec3 AiToVec3(const aiVector3D& v) noexcept
    {
        return mathUtils::Vec3(v.x, v.y, v.z);
    }

    [[nodiscard]] inline mathUtils::Vec4 AiToQuatVec4(const aiQuaternion& q) noexcept
    {
        return mathUtils::Vec4(q.x, q.y, q.z, q.w);
    }

    [[nodiscard]] inline std::filesystem::path NormalizeLexically(const std::filesystem::path& p)
    {
        std::error_code ec;
        const auto weak = std::filesystem::weakly_canonical(p, ec);
        if (!ec)
        {
            return weak;
        }
        return p.lexically_normal();
    }

    [[nodiscard]] inline std::string NormalizeNameForCompare(std::string_view s)
    {
        std::filesystem::path p{ std::string(s) };
        std::string out = p.filename().string();
        if (out.empty())
        {
            out = std::string(s);
        }

        for (char& c : out)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return out;
    }

    [[nodiscard]] inline std::string NormalizeExt(std::string ext)
    {
        if (!ext.empty() && ext[0] == '.')
        {
            ext.erase(ext.begin());
        }
        for (char& c : ext)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (ext.empty())
        {
            ext = "bin";
        }
        return ext;
    }

    [[nodiscard]] inline std::uint64_t HashStringStable(std::string_view s)
    {
        std::uint64_t h = 14695981039346656037ull;
        for (const unsigned char c : s)
        {
            h ^= static_cast<std::uint64_t>(c);
            h *= 1099511628211ull;
        }
        return h;
    }

    [[nodiscard]] inline std::string Hex64(const std::uint64_t value)
    {
        char buf[17]{};
        std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(value));
        return std::string(buf);
    }

    [[nodiscard]] inline std::string SanitizeFilename(std::string s)
    {
        for (char& c : s)
        {
            const bool ok =
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '-' || c == '.';
            if (!ok)
            {
                c = '_';
            }
        }
        if (s.empty())
        {
            s = "asset";
        }
        return s;
    }

    [[nodiscard]] inline std::string MakeAssetRelativePath(const std::filesystem::path& absolutePath)
    {
        namespace fs = std::filesystem;
        const fs::path assetRoot = NormalizeLexically(corefs::FindAssetRoot());
        const fs::path absNorm = NormalizeLexically(absolutePath);

        std::error_code ec;
        fs::path rel = fs::relative(absNorm, assetRoot, ec);
        if (!ec && !rel.empty())
        {
            return rel.generic_string();
        }

        return absNorm.generic_string();
    }

    [[nodiscard]] inline std::optional<std::filesystem::path> TryResolveTexturePath(
        const std::filesystem::path& modelAbsPath,
        std::string_view rawTexturePath)
    {
        namespace fs = std::filesystem;
        if (rawTexturePath.empty())
        {
            return std::nullopt;
        }

        fs::path raw{ std::string(rawTexturePath) };

        std::vector<fs::path> candidates;
        if (raw.is_absolute())
        {
            candidates.push_back(raw);
        }
        else
        {
            const fs::path modelDir = modelAbsPath.parent_path();
            const fs::path assetRoot = corefs::FindAssetRoot();
            const std::string modelStem = modelAbsPath.stem().string();

            candidates.push_back(modelDir / raw);
            candidates.push_back(assetRoot / raw);

            if (raw.has_filename())
            {
                const fs::path filenameOnly = raw.filename();

                candidates.push_back(assetRoot / "textures" / filenameOnly);
                candidates.push_back(modelDir / filenameOnly);
                candidates.push_back(modelDir / (modelStem + ".fbm") / filenameOnly);
                candidates.push_back(assetRoot / "models" / (modelStem + ".fbm") / filenameOnly);
            }
        }

        for (const fs::path& c : candidates)
        {
            std::error_code ec;
            if (fs::exists(c, ec) && !ec)
            {
                return NormalizeLexically(c);
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] inline std::optional<std::uint32_t> TryFindEmbeddedTextureIndex(
        const aiScene* scene,
        std::string_view rawTexturePath)
    {
        if (!scene || scene->mNumTextures == 0)
        {
            return std::nullopt;
        }

        const std::string wanted = NormalizeNameForCompare(rawTexturePath);
        if (!wanted.empty())
        {
            for (std::uint32_t i = 0; i < scene->mNumTextures; ++i)
            {
                const aiTexture* tex = scene->mTextures[i];
                if (!tex)
                {
                    continue;
                }

                const std::string texName = NormalizeNameForCompare(tex->mFilename.C_Str());
                if (!texName.empty() && texName == wanted)
                {
                    return i;
                }
            }
        }

        if (scene->mNumTextures == 1)
        {
            return 0u;
        }

        return std::nullopt;
    }

    [[nodiscard]] inline std::string MakeModelImportKey(const std::filesystem::path& modelAbsPath)
    {
        namespace fs = std::filesystem;
        const fs::path absNorm = NormalizeLexically(modelAbsPath);
        const fs::path assetRoot = NormalizeLexically(corefs::FindAssetRoot());

        std::error_code ec;
        fs::path rel = fs::relative(absNorm, assetRoot, ec);
        const std::string stableId = (!ec && !rel.empty())
            ? rel.generic_string()
            : absNorm.generic_string();

        return SanitizeFilename(modelAbsPath.stem().string()) + "_" + Hex64(HashStringStable(stableId));
    }

    [[nodiscard]] inline std::string MakeImportedTextureFilename(
        const std::filesystem::path& modelAbsPath,
        std::string_view slotName,
        std::string_view sourceTag,
        std::string_view extension)
    {
        const std::string ext = NormalizeExt(std::string(extension));
        return SanitizeFilename(
            modelAbsPath.stem().string() + "_" +
            std::string(slotName) + "_" +
            std::string(sourceTag) + "." +
            ext);
    }

    [[nodiscard]] inline std::filesystem::path MakeImportedDirForModel(const std::filesystem::path& modelAbsPath)
    {
        return corefs::FindAssetRoot() / "imported" / MakeModelImportKey(modelAbsPath);
    }
}
