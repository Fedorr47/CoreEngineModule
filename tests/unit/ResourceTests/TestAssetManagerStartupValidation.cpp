#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "FakeMeshIO.h"
#include "FakeTextureIO.h"
#include "TestSupport/TestTempPath.h"

using namespace test;

TEST(AssetManagerStartupValidation, AcceptsExistingAssetRootDirectory)
{
	const ScopedTempPath tempRoot(MakeUniqueTempPath("dir_ok"));
	ASSERT_TRUE(std::filesystem::create_directories(tempRoot.Path()));

	FakeTextureDecoder decoder{};
	FakeTextureUploader uploader{};
	FakeJobSystem jobSystem{};
	FakeRenderQueue renderQueue{};
	TextureIO textureIO = MakeIO(decoder, uploader, jobSystem, renderQueue);

	FakeRHIDevice device{};
	FakeMeshLoader meshLoader{};
	FakeMeshUploader meshUploader{};
	rendern::MeshIO meshIO = MakeMeshIO(device, jobSystem, renderQueue, meshLoader, meshUploader);

	EXPECT_NO_THROW({
		[[maybe_unused]] AssetManager manager(textureIO, meshIO, tempRoot.Path());
	});
}

TEST(AssetManagerStartupValidation, RejectsMissingAssetRootDirectory)
{
	const ScopedTempPath missingRoot(MakeUniqueTempPath("missing_dir"));

	FakeTextureDecoder decoder{};
	FakeTextureUploader uploader{};
	FakeJobSystem jobSystem{};
	FakeRenderQueue renderQueue{};
	TextureIO textureIO = MakeIO(decoder, uploader, jobSystem, renderQueue);

	FakeRHIDevice device{};
	FakeMeshLoader meshLoader{};
	FakeMeshUploader meshUploader{};
	rendern::MeshIO meshIO = MakeMeshIO(device, jobSystem, renderQueue, meshLoader, meshUploader);

	try
	{
		[[maybe_unused]] AssetManager manager(textureIO, meshIO, missingRoot.Path());
		FAIL() << "Expected std::invalid_argument for missing asset root";
	}
	catch (const std::invalid_argument& ex)
	{
		const std::string message = ex.what();
		EXPECT_NE(message.find("asset root"), std::string::npos);
		EXPECT_NE(message.find("does not exist"), std::string::npos);
		EXPECT_NE(message.find(missingRoot.Path().string()), std::string::npos);
	}
}

TEST(AssetManagerStartupValidation, RejectsRegularFileAssetRoot)
{
	const ScopedTempPath tempFilePath(MakeUniqueTempPath("file_root"));
	{
		std::ofstream out(tempFilePath.Path());
		ASSERT_TRUE(out.good());
		out << "not_a_directory";
	}
	ASSERT_TRUE(std::filesystem::exists(tempFilePath.Path()));
	ASSERT_TRUE(std::filesystem::is_regular_file(tempFilePath.Path()));

	FakeTextureDecoder decoder{};
	FakeTextureUploader uploader{};
	FakeJobSystem jobSystem{};
	FakeRenderQueue renderQueue{};
	TextureIO textureIO = MakeIO(decoder, uploader, jobSystem, renderQueue);

	FakeRHIDevice device{};
	FakeMeshLoader meshLoader{};
	FakeMeshUploader meshUploader{};
	rendern::MeshIO meshIO = MakeMeshIO(device, jobSystem, renderQueue, meshLoader, meshUploader);

	try
	{
		[[maybe_unused]] AssetManager manager(textureIO, meshIO, tempFilePath.Path());
		FAIL() << "Expected std::invalid_argument for regular-file asset root";
	}
	catch (const std::invalid_argument& ex)
	{
		const std::string message = ex.what();
		EXPECT_NE(message.find("asset root"), std::string::npos);
		EXPECT_NE(message.find("not a directory"), std::string::npos);
		EXPECT_NE(message.find(tempFilePath.Path().string()), std::string::npos);
	}
}