module;

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <unordered_map>
#include <unordered_set>

export module core:assimp_scene_loader;

import :math_utils;
import :file_system;
import :scene;

#include "AssimpImportShared.inl"

export namespace rendern
{
    struct ImportedSubmeshInfo
    {
        std::uint32_t submeshIndex{ 0 };
        std::uint32_t materialIndex{ 0 };
        std::string name;
    };

    struct ImportedSceneNode
    {
        std::string name;
        int parent{ -1 };
        Transform localTransform{};
        std::vector<std::uint32_t> submeshes;
    };

    struct ImportedMaterialTextureRef
    {
        // For external textures:
        //   - scan-only mode: resolved normalized path (asset-relative if possible, otherwise absolute)
        //   - materialize mode: asset-relative path inside assets/imported/...
        //
        // For embedded textures:
        //   - scan-only mode: "*N"
        //   - materialize mode: exported asset-relative path inside assets/imported/...
        std::string path;
		bool embedded{ false }; // whether this texture was embedded in the model file (and thus needs export)
    };

    struct ImportedMaterialInfo
    {
        std::string name;
        std::optional<ImportedMaterialTextureRef> baseColor;
        std::optional<ImportedMaterialTextureRef> normal;
        std::optional<ImportedMaterialTextureRef> metallic;
        std::optional<ImportedMaterialTextureRef> roughness;
        std::optional<ImportedMaterialTextureRef> ao;
        std::optional<ImportedMaterialTextureRef> emissive;
        std::optional<ImportedMaterialTextureRef> specular;
        std::optional<ImportedMaterialTextureRef> gloss;
        std::optional<ImportedMaterialTextureRef> height;
    };

    struct ImportedModelScene
    {
        std::vector<ImportedSubmeshInfo> submeshes;
        std::vector<ImportedMaterialInfo> materials;
        std::vector<ImportedSceneNode> nodes;
    };

        struct ImportedTextureWriteTracker
        {
            std::unordered_set<std::string> writtenPaths;
            std::unordered_map<std::string, std::size_t> writeAttempts;
        };

        std::string MakeWriteDiagnostic(const std::filesystem::path& path, std::size_t attempt)
        {
            namespace fs = std::filesystem;
            std::error_code ec;
            const bool exists = fs::exists(path, ec) && !ec;
            std::string diag = " | attempt=" + std::to_string(attempt);
            diag += " | exists=" + std::string(exists ? "true" : "false");
            if (exists)
            {
                ec.clear();
                const auto fileSize = fs::file_size(path, ec);
                diag += " | file_size=";
                diag += ec ? std::string("<error:") + ec.message() + ">" : std::to_string(fileSize);
            }
            return diag;
        }

        void CopyFileIfDifferent(const std::filesystem::path& src, const std::filesystem::path& dst, ImportedTextureWriteTracker* tracker = nullptr)
        {
            namespace fs = std::filesystem;
            const std::string key = rendern::assimp_detail::NormalizeLexically(dst).generic_string();
            std::size_t attempt = 1;
            if (tracker)
            {
                attempt = ++tracker->writeAttempts[key];
                if (tracker->writtenPaths.contains(key))
                {
                    return;
                }
            }

            std::error_code ec;
            fs::create_directories(dst.parent_path(), ec);

            ec.clear();
            const bool dstExists = fs::exists(dst, ec) && !ec;
            if (dstExists)
            {
                std::error_code srcEc;
                const auto srcSize = fs::file_size(src, srcEc);
                std::error_code dstEc;
                const auto dstSize = fs::file_size(dst, dstEc);
                if (!srcEc && !dstEc && srcSize == dstSize)
                {
                    if (tracker)
                    {
                        tracker->writtenPaths.insert(key);
                    }
                    return;
                }
            }

            ec.clear();
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
            if (ec)
            {
                throw std::runtime_error(
                    "Failed to copy imported texture from " + src.string() + " to " + dst.string() + ": " + ec.message() +
                    MakeWriteDiagnostic(dst, attempt));
            }
            if (tracker)
            {
                tracker->writtenPaths.insert(key);
            }
        }

        void WriteBytesToFile(const std::filesystem::path& path, const void* data, std::size_t size, ImportedTextureWriteTracker* tracker = nullptr)
        {
            const std::string key = rendern::assimp_detail::NormalizeLexically(path).generic_string();
            std::size_t attempt = 1;
            if (tracker)
            {
                attempt = ++tracker->writeAttempts[key];
                if (tracker->writtenPaths.contains(key))
                {
                    return;
                }
            }

            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);

            ec.clear();
            const bool exists = std::filesystem::exists(path, ec) && !ec;
            if (exists)
            {
                ec.clear();
                const auto fileSize = std::filesystem::file_size(path, ec);
                if (!ec && fileSize == size)
                {
                    if (tracker)
                    {
                        tracker->writtenPaths.insert(key);
                    }
                    return;
                }
            }

            std::ofstream f(path, std::ios::binary | std::ios::trunc);
            if (!f)
            {
                throw std::runtime_error("Failed to open imported texture for write: " + path.string() + MakeWriteDiagnostic(path, attempt));
            }
            f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
            if (!f)
            {
                throw std::runtime_error("Failed to write imported texture: " + path.string() + MakeWriteDiagnostic(path, attempt));
            }
            if (tracker)
            {
                tracker->writtenPaths.insert(key);
            }
        }

        void WriteUncompressedAiTextureAsTga(const std::filesystem::path& path, const aiTexture* tex, ImportedTextureWriteTracker* tracker = nullptr)
        {
            if (!tex || tex->mHeight == 0)
            {
                throw std::runtime_error("Invalid raw aiTexture for TGA export");
            }

            const std::uint16_t width = static_cast<std::uint16_t>(tex->mWidth);
            const std::uint16_t height = static_cast<std::uint16_t>(tex->mHeight);
            std::vector<unsigned char> bytes;
            bytes.resize(18u + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
            bytes[2] = 2;
            bytes[12] = static_cast<unsigned char>(width & 0xFF);
            bytes[13] = static_cast<unsigned char>((width >> 8) & 0xFF);
            bytes[14] = static_cast<unsigned char>(height & 0xFF);
            bytes[15] = static_cast<unsigned char>((height >> 8) & 0xFF);
            bytes[16] = 32;
            bytes[17] = 0x20;

            unsigned char* dst = bytes.data() + 18;
            for (std::size_t i = 0; i < static_cast<std::size_t>(width) * static_cast<std::size_t>(height); ++i)
            {
                const aiTexel& s = tex->pcData[i];
                dst[i * 4 + 0] = s.b;
                dst[i * 4 + 1] = s.g;
                dst[i * 4 + 2] = s.r;
                dst[i * 4 + 3] = s.a;
            }
            WriteBytesToFile(path, bytes.data(), bytes.size(), tracker);
        }

        std::optional<ImportedMaterialTextureRef> ResolveExternalTextureRefNoCopy(
            const std::filesystem::path& modelAbsPath,
            std::string_view rawTexturePath)
        {
            if (auto resolved = rendern::assimp_detail::TryResolveTexturePath(modelAbsPath, rawTexturePath))
            {
                ImportedMaterialTextureRef out{};
                out.path = rendern::assimp_detail::MakeAssetRelativePath(*resolved);
                out.embedded = false;
                return out;
            }
            return std::nullopt;
        }

        std::optional<std::string> CopyExternalTextureToImportedFolder(
            const std::filesystem::path& modelAbsPath,
            std::string_view rawTexturePath,
            std::string_view slotName,
            ImportedTextureWriteTracker* tracker = nullptr)
        {
            const auto resolved = rendern::assimp_detail::TryResolveTexturePath(modelAbsPath, rawTexturePath);
            if (!resolved)
            {
                return std::nullopt;
            }

            const std::filesystem::path dstDir = rendern::assimp_detail::MakeImportedDirForModel(modelAbsPath);

            const std::string sourceTag =
                rendern::assimp_detail::Hex64(rendern::assimp_detail::HashStringStable(resolved->generic_string()));

            const std::string filename = rendern::assimp_detail::MakeImportedTextureFilename(
                modelAbsPath,
                slotName,
                sourceTag,
                resolved->extension().string());

            const std::filesystem::path dst = dstDir / filename;
            CopyFileIfDifferent(*resolved, dst, tracker);
            return rendern::assimp_detail::MakeAssetRelativePath(dst);
        }

        std::string ExportEmbeddedTextureToImportedFolder(
            const aiScene* scene,
            const std::filesystem::path& modelAbsPath,
            std::uint32_t embeddedIndex,
            std::string_view slotName,
            ImportedTextureWriteTracker* tracker = nullptr)
        {
            if (!scene || embeddedIndex >= scene->mNumTextures)
            {
                throw std::runtime_error("Embedded texture index out of range");
            }

            const aiTexture* tex = scene->mTextures[embeddedIndex];
            if (!tex)
            {
                throw std::runtime_error("Embedded texture is null");
            }

            const std::filesystem::path outDir = rendern::assimp_detail::MakeImportedDirForModel(modelAbsPath);
            std::filesystem::path outPath;

            if (tex->mHeight == 0)
            {
                std::string ext = rendern::assimp_detail::NormalizeExt(tex->achFormatHint);
                if (ext == "bin")
                {
                    ext = "png";
                }

                const std::string filename = rendern::assimp_detail::MakeImportedTextureFilename(
                    modelAbsPath,
                    slotName,
                    "embedded_" + std::to_string(embeddedIndex),
                    ext);

                outPath = outDir / filename;
                WriteBytesToFile(outPath, tex->pcData, static_cast<std::size_t>(tex->mWidth), tracker);
            }
            else
            {
                const std::string filename = rendern::assimp_detail::MakeImportedTextureFilename(
                    modelAbsPath,
                    slotName,
                    "embedded_" + std::to_string(embeddedIndex),
                    "tga");

                outPath = outDir / filename;
                WriteUncompressedAiTextureAsTga(outPath, tex, tracker);
            }

            return rendern::assimp_detail::MakeAssetRelativePath(outPath);
        }

        std::optional<ImportedMaterialTextureRef> ReadAndNormalizeTextureRef(
            const aiScene* scene,
            const std::filesystem::path& modelAbsPath,
            aiMaterial* mat,
            aiTextureType type,
            std::string_view slotName,
            bool materializeTextures,
            ImportedTextureWriteTracker* tracker = nullptr)
        {
            if (!mat)
            {
                return std::nullopt;
            }

            aiString tex;
            if (mat->GetTexture(type, 0, &tex) != AI_SUCCESS || tex.length == 0)
            {
                return std::nullopt;
            }

            const std::string raw = tex.C_Str();

            // Explicit embedded reference: "*0", "*1", ...
            if (!raw.empty() && raw[0] == '*')
            {
                try
                {
                    const std::uint32_t idx =
                        static_cast<std::uint32_t>(std::stoul(raw.substr(1)));

                    ImportedMaterialTextureRef out{};
                    out.embedded = true;

                    if (materializeTextures)
                    {
                        out.path = ExportEmbeddedTextureToImportedFolder(scene, modelAbsPath, idx, slotName, tracker);
                    }
                    else
                    {
                        out.path = raw;
                    }

                    return out;
                }
                catch (...)
                {
                    return std::nullopt;
                }
            }

            // External texture path
            if (materializeTextures)
            {
                if (auto copied = CopyExternalTextureToImportedFolder(modelAbsPath, raw, slotName, tracker))
                {
                    ImportedMaterialTextureRef out{};
                    out.path = *copied;
                    out.embedded = false;
                    return out;
                }
            }
            else
            {
                if (auto resolved = ResolveExternalTextureRefNoCopy(modelAbsPath, raw))
                {
                    return resolved;
                }
            }

            // Fallback: maybe importer exposed a weird external-looking string,
            // but the texture is actually embedded in the file.
            if (auto embeddedIdx = rendern::assimp_detail::TryFindEmbeddedTextureIndex(scene, raw))
            {
                ImportedMaterialTextureRef out{};
                out.embedded = true;

                if (materializeTextures)
                {
                    out.path = ExportEmbeddedTextureToImportedFolder(
                        scene,
                        modelAbsPath,
                        *embeddedIdx,
                        slotName,
                        tracker);
                }
                else
                {
                    out.path = "*" + std::to_string(*embeddedIdx);
                }

                return out;
            }

            return std::nullopt;
        }
        
    inline ImportedModelScene LoadAssimpScene(
        std::filesystem::path pathIn,
        bool flipUVs = true,
        bool importSkeletonNodes = false,
        bool materializeTextures = false)
    {
        namespace fs = std::filesystem;
        fs::path path = pathIn;
        if (!path.is_absolute())
        {
            path = corefs::ResolveAsset(path);
        }
        path = rendern::assimp_detail::NormalizeLexically(path);

        unsigned flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals;
        if (flipUVs)
        {
            flags |= aiProcess_FlipUVs;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(), flags);
        if (!scene || !scene->mRootNode)
        {
            const char* err = importer.GetErrorString();
            std::string msg = "Assimp failed to load scene: " + path.string();
            if (err && *err)
            {
                msg += " | ";
                msg += err;
            }
            throw std::runtime_error(msg);
        }

        ImportedModelScene out{};
        ImportedTextureWriteTracker textureWriteTracker{};
        out.submeshes.reserve(scene->mNumMeshes);
        for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            const aiMesh* mesh = scene->mMeshes[mi];
            ImportedSubmeshInfo sm{};
            sm.submeshIndex = mi;
            sm.materialIndex = mesh ? mesh->mMaterialIndex : 0u;
            sm.name = (mesh && mesh->mName.length > 0) ? std::string(mesh->mName.C_Str()) : ("Submesh_" + std::to_string(mi));
            out.submeshes.push_back(std::move(sm));
        }

        out.materials.reserve(scene->mNumMaterials);
        for (unsigned mi = 0; mi < scene->mNumMaterials; ++mi)
        {
            aiMaterial* mat = scene->mMaterials[mi];
            ImportedMaterialInfo info{};
            aiString name;
            info.name = (mat && mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0)
                ? std::string(name.C_Str())
                : ("Material_" + std::to_string(mi));

            info.baseColor = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_BASE_COLOR, "albedo", materializeTextures, &textureWriteTracker);
            if (!info.baseColor) info.baseColor = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_DIFFUSE, "albedo", materializeTextures, &textureWriteTracker);
            info.normal = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_NORMAL_CAMERA, "normal", materializeTextures, &textureWriteTracker);
            if (!info.normal) info.normal = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_NORMALS, "normal", materializeTextures, &textureWriteTracker);
            if (!info.normal) info.normal = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_HEIGHT, "normal", materializeTextures, &textureWriteTracker);
            info.metallic = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_METALNESS, "metallic", materializeTextures, &textureWriteTracker);
            info.roughness = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", materializeTextures, &textureWriteTracker);
            info.ao = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_AMBIENT_OCCLUSION, "ao", materializeTextures, &textureWriteTracker);
            if (!info.ao) info.ao = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_LIGHTMAP, "ao", materializeTextures, &textureWriteTracker);
            info.emissive = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_EMISSIVE, "emissive", materializeTextures, &textureWriteTracker);
            if (!info.emissive) info.emissive = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_EMISSION_COLOR, "emissive", materializeTextures, &textureWriteTracker);
            info.specular = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_SPECULAR, "specular", materializeTextures, &textureWriteTracker);
            info.gloss = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_SHININESS, "gloss", materializeTextures, &textureWriteTracker);
            info.height = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_DISPLACEMENT, "height", materializeTextures, &textureWriteTracker);
            if (!info.height) info.height = ReadAndNormalizeTextureRef(scene, path, mat, aiTextureType_HEIGHT, "height", materializeTextures, &textureWriteTracker);
            out.materials.push_back(std::move(info));
        }

        const auto convertTransform = [](const mathUtils::Mat4& m)
            {
                Transform t{};
                t.useMatrix = true;
                t.matrix = m;
                return t;
            };

        std::vector<int> stackParents;
        std::vector<const aiNode*> stackNodes;
        stackNodes.push_back(scene->mRootNode);
        stackParents.push_back(-1);

        while (!stackNodes.empty())
        {
            const aiNode* node = stackNodes.back();
            const int parent = stackParents.back();
            stackNodes.pop_back();
            stackParents.pop_back();

            const bool hasMeshes = node->mNumMeshes > 0;
            const bool shouldKeepNode = importSkeletonNodes || hasMeshes || parent < 0;

            int thisIndex = parent;
            if (shouldKeepNode)
            {
                ImportedSceneNode dst{};
                dst.name = (node->mName.length > 0) ? std::string(node->mName.C_Str()) : std::string("Node_") + std::to_string(out.nodes.size());
                dst.parent = parent;
                dst.localTransform = convertTransform(rendern::assimp_detail::AiToMat4(node->mTransformation));
                dst.submeshes.reserve(node->mNumMeshes);
                for (unsigned i = 0; i < node->mNumMeshes; ++i)
                {
                    dst.submeshes.push_back(node->mMeshes[i]);
                }
                thisIndex = static_cast<int>(out.nodes.size());
                out.nodes.push_back(std::move(dst));
            }

            for (int ci = static_cast<int>(node->mNumChildren) - 1; ci >= 0; --ci)
            {
                stackNodes.push_back(node->mChildren[ci]);
                stackParents.push_back(thisIndex);
            }
        }

        return out;
    }
}