module;

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

export module core:assimp_scene_loader;

import :math_utils;
import :file_system;
import :scene;

export namespace rendern
{
    struct ImportedMaterialTextureRef
    {
        std::string path;
        bool embedded{ false };
        std::optional<std::uint32_t> embeddedIndex;
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
    };

    struct ImportedEmbeddedTexture
    {
        std::uint32_t index{ 0 };
        std::string suggestedExtension;
    };

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

    struct ImportedModelScene
    {
        std::vector<ImportedSubmeshInfo> submeshes;
        std::vector<ImportedMaterialInfo> materials;
        std::vector<ImportedSceneNode> nodes;
    };

    inline mathUtils::Mat4 AiToMat4(const aiMatrix4x4& m)
    {
        mathUtils::Mat4 out{ 1.0f };
        out(0, 0) = m.a1; out(0, 1) = m.a2; out(0, 2) = m.a3; out(0, 3) = m.a4;
        out(1, 0) = m.b1; out(1, 1) = m.b2; out(1, 2) = m.b3; out(1, 3) = m.b4;
        out(2, 0) = m.c1; out(2, 1) = m.c2; out(2, 2) = m.c3; out(2, 3) = m.c4;
        out(3, 0) = m.d1; out(3, 1) = m.d2; out(3, 2) = m.d3; out(3, 3) = m.d4;
        return out;
    }

    inline std::optional<ImportedMaterialTextureRef> ReadTextureRef(
        aiMaterial* mat,
        aiTextureType type)
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

        ImportedMaterialTextureRef out{};
        out.path = tex.C_Str();
        out.embedded = !out.path.empty() && out.path[0] == '*';

        if (out.embedded)
        {
            try
            {
                out.embeddedIndex = static_cast<std::uint32_t>(std::stoul(out.path.substr(1)));
            }
            catch (...)
            {
                out.embeddedIndex.reset();
            }
        }

        return out;
    }

    inline std::string NormalizeExt(std::string ext)
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

    inline void WriteBytesToFile(const std::filesystem::path& path, const void* data, std::size_t size)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f)
        {
            throw std::runtime_error("Failed to write embedded texture: " + path.string());
        }
        f.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    inline void WriteUncompressedAiTextureAsTga(const std::filesystem::path& path, const aiTexture* tex)
    {
        if (!tex || tex->mHeight == 0)
        {
            throw std::runtime_error("Invalid raw aiTexture for TGA export");
        }

        const std::uint16_t width = static_cast<std::uint16_t>(tex->mWidth);
        const std::uint16_t height = static_cast<std::uint16_t>(tex->mHeight);

        std::vector<unsigned char> bytes;
        bytes.resize(18u + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);

        bytes[2] = 2; // uncompressed true-color
        bytes[12] = static_cast<unsigned char>(width & 0xFF);
        bytes[13] = static_cast<unsigned char>((width >> 8) & 0xFF);
        bytes[14] = static_cast<unsigned char>(height & 0xFF);
        bytes[15] = static_cast<unsigned char>((height >> 8) & 0xFF);
        bytes[16] = 32;
        bytes[17] = 0x20; // top-left

        unsigned char* dst = bytes.data() + 18;
        for (std::size_t i = 0; i < static_cast<std::size_t>(width) * static_cast<std::size_t>(height); ++i)
        {
            const aiTexel& s = tex->pcData[i];
            dst[i * 4 + 0] = s.b;
            dst[i * 4 + 1] = s.g;
            dst[i * 4 + 2] = s.r;
            dst[i * 4 + 3] = s.a;
        }

        WriteBytesToFile(path, bytes.data(), bytes.size());
    }

    inline std::string ExportEmbeddedTextureToImportedAsset(
        const aiScene* scene,
        const std::filesystem::path& modelAbsPath,
        std::uint32_t embeddedIndex)
    {
        namespace fs = std::filesystem;

        if (!scene || embeddedIndex >= scene->mNumTextures)
        {
            throw std::runtime_error("Embedded texture index out of range");
        }

        const aiTexture* tex = scene->mTextures[embeddedIndex];
        if (!tex)
        {
            throw std::runtime_error("Embedded texture is null");
        }

        const fs::path assetRoot = corefs::FindAssetRoot();
        const std::string stem = modelAbsPath.stem().string();
        const fs::path outDir = assetRoot / "imported" / stem;

        std::string ext = NormalizeExt(tex->achFormatHint);
        if (tex->mHeight == 0)
        {
            if (ext == "bin")
            {
                ext = "png";
            }
            const fs::path outPath = outDir / ("embedded_" + std::to_string(embeddedIndex) + "." + ext);
            WriteBytesToFile(outPath, tex->pcData, static_cast<std::size_t>(tex->mWidth));
            return fs::relative(outPath, assetRoot).generic_string();
        }

        ext = "tga";
        const fs::path outPath = outDir / ("embedded_" + std::to_string(embeddedIndex) + "." + ext);
        WriteUncompressedAiTextureAsTga(outPath, tex);
        return fs::relative(outPath, assetRoot).generic_string();
    }

    inline ImportedModelScene LoadAssimpScene(std::filesystem::path pathIn, bool flipUVs = true)
    {
        namespace fs = std::filesystem;
        fs::path path = pathIn;
        if (!path.is_absolute())
        {
            path = corefs::ResolveAsset(path);
        }

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
        out.submeshes.reserve(scene->mNumMeshes);
        for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            const aiMesh* mesh = scene->mMeshes[mi];
            ImportedSubmeshInfo sm{};
            sm.submeshIndex = mi;
            sm.materialIndex = mesh ? mesh->mMaterialIndex : 0u;
            if (mesh && mesh->mName.length > 0)
            {
                sm.name = mesh->mName.C_Str();
            }
            else
            {
                sm.name = "Submesh_" + std::to_string(mi);
            }
            out.submeshes.push_back(std::move(sm));
        }

        out.materials.reserve(scene->mNumMaterials);
        for (unsigned mi = 0; mi < scene->mNumMaterials; ++mi)
        {
            aiMaterial* mat = scene->mMaterials[mi];

            ImportedMaterialInfo info{};
            aiString name;
            if (mat && mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length > 0)
            {
                info.name = name.C_Str();
            }
            else
            {
                info.name = "Material_" + std::to_string(mi);
            }

            // Base color / diffuse
            info.baseColor = ReadTextureRef(mat, aiTextureType_BASE_COLOR);
            if (!info.baseColor)
            {
                info.baseColor = ReadTextureRef(mat, aiTextureType_DIFFUSE);
            }

            // Normal
            info.normal = ReadTextureRef(mat, aiTextureType_NORMAL_CAMERA);
            if (!info.normal)
            {
                info.normal = ReadTextureRef(mat, aiTextureType_NORMALS);
            }
            if (!info.normal)
            {
                info.normal = ReadTextureRef(mat, aiTextureType_HEIGHT);
            }

            // PBR-ish slots
            info.metallic = ReadTextureRef(mat, aiTextureType_METALNESS);
            info.roughness = ReadTextureRef(mat, aiTextureType_DIFFUSE_ROUGHNESS);
            info.ao = ReadTextureRef(mat, aiTextureType_AMBIENT_OCCLUSION);
            if (!info.ao)
            {
                info.ao = ReadTextureRef(mat, aiTextureType_LIGHTMAP);
            }
            info.emissive = ReadTextureRef(mat, aiTextureType_EMISSIVE);
            if (!info.emissive)
            {
                info.emissive = ReadTextureRef(mat, aiTextureType_EMISSION_COLOR);
            }

            const auto normalizeTextureRef = [&](std::optional<ImportedMaterialTextureRef>& ref)
                {
                    if (!ref || !ref->embedded || !ref->embeddedIndex)
                    {
                        return;
                    }

                    ref->path = ExportEmbeddedTextureToImportedAsset(scene, path, *ref->embeddedIndex);
                    ref->embedded = false;
                    ref->embeddedIndex.reset();
                };

            normalizeTextureRef(info.baseColor);
            normalizeTextureRef(info.normal);
            normalizeTextureRef(info.metallic);
            normalizeTextureRef(info.roughness);
            normalizeTextureRef(info.ao);
            normalizeTextureRef(info.emissive);

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
        std::vector<mathUtils::Mat4> stackParentWorlds;
        stackNodes.push_back(scene->mRootNode);
        stackParents.push_back(-1);
        stackParentWorlds.push_back(mathUtils::Mat4(1.0f));

        while (!stackNodes.empty())
        {
            const aiNode* node = stackNodes.back();
            const int parent = stackParents.back();
            const mathUtils::Mat4 parentWorld = stackParentWorlds.back();
            stackNodes.pop_back();
            stackParents.pop_back();
            stackParentWorlds.pop_back();

            ImportedSceneNode dst{};
            dst.name = (node->mName.length > 0) ? std::string(node->mName.C_Str()) : std::string("Node_") + std::to_string(out.nodes.size());
            dst.parent = parent;
            dst.localTransform = convertTransform(AiToMat4(node->mTransformation));
            dst.submeshes.reserve(node->mNumMeshes);
            for (unsigned i = 0; i < node->mNumMeshes; ++i)
            {
                dst.submeshes.push_back(node->mMeshes[i]);
            }
            const int thisIndex = static_cast<int>(out.nodes.size());
            out.nodes.push_back(std::move(dst));

            for (int ci = static_cast<int>(node->mNumChildren) - 1; ci >= 0; --ci)
            {
                stackNodes.push_back(node->mChildren[ci]);
                stackParents.push_back(thisIndex);
                stackParentWorlds.push_back(parentWorld * AiToMat4(node->mTransformation));
            }
        }

        return out;
    }
}