#include <gtest/gtest.h>

#include "TestSupport/TestFixtureLoader.h"

import core;

using namespace rendern;

template <typename Callable>
void ExpectRuntimeErrorContains(Callable&& callable, std::string_view expectedFragment)
{
    try
    {
        std::forward<Callable>(callable)();
        FAIL() << "Expected std::runtime_error to be thrown";
    }
    catch (const std::runtime_error& ex)
    {
        const std::string message = ex.what();
        EXPECT_NE(message.find(expectedFragment), std::string::npos) << "Actual message: " << message;
    }
}

TEST(AssimpImportContracts, StaticMeshImportRejectsEmptyMeshFixture)
{
    const auto fixture = ResolveFixturePath("assimp/empty_mesh.obj");

    ExpectRuntimeErrorContains(
    [&]
    {
        LoadAssimp(fixture, true, std::nullopt, true);
    },
    "Assimp failed to load mesh");
}

TEST(AssimpImportContracts, StaticMeshImportRejectsOutOfRangeSubmeshIndex)
{
    const auto fixture = ResolveFixturePath("assimp/single_triangle.obj");

    ExpectRuntimeErrorContains(
   [&]
    {
        LoadAssimp(fixture, true, 99u, true);
    },"Assimp submesh index out of range");
}

TEST(AssimpImportContracts, SkinnedImportRejectsMeshWithoutBones)
{
    const auto fixture = ResolveFixturePath("assimp/single_triangle.obj");

    ExpectRuntimeErrorContains(
    [&]
    {
       LoadAssimpSkinned(fixture, true, std::nullopt);
    },"requires meshes with bones");
}

TEST(AssimpImportContracts, AnimationImportRejectsIncompatibleSkeleton)
{
    const auto fixture = std::filesystem::path("animations/idle.fbx");

    Skeleton invalidSkeleton{};
    invalidSkeleton.rootBoneIndex = 0;

    ExpectRuntimeErrorContains(
    [&]
    {
        LoadAssimpAnimationClips(fixture, invalidSkeleton, true);
    },"no compatible bone channels");
}