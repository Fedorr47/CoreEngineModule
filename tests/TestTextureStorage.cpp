#include <gtest/gtest.h>

#include "FakeTextureIO.h"

static TextureProperties MakeProperties(TextureFormat fmt = TextureFormat::RGBA)
{
	TextureProperties properties{};
	properties.format = fmt;
	properties.filePath = "dummy.png";
	properties.srgb = true;
	properties.generateMips = true;
	return properties;
}

TEST(TextureStorage, LoadAndCreateEntrySucceeds)
{
	ResourceManager ResManager;

	FakeTextureDecoder decoder{};
	FakeTextureUploader uploader{};
	FakeJobSystem jobSystem{};
	FakeRenderQueue renderQueue{};
	auto io = MakeIO(decoder, uploader, jobSystem, renderQueue);

	auto texture = ResManager.Load<TextureResource>("tex1", io, MakeProperties(TextureFormat::RGBA));
	ASSERT_TRUE(texture);

	auto& storage = ResManager.GetStorage<TextureResource>();
	EXPECT_EQ(storage.GetState("tex1"), ResourceState::Loading);

	EXPECT_TRUE(storage.ProcessUploads(io));
	EXPECT_EQ(storage.GetState("tex1"), ResourceState::Loaded);

	EXPECT_NE(texture->GetResource().id, 0u);
	EXPECT_EQ(uploader.createdIds.size(), 1u);
}