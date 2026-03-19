#include <gtest/gtest.h>

#include "FakeTextureIO.h"

namespace
{
	TextureProperties MakeProperties(const TextureFormat fmt = TextureFormat::RGBA)
	{
		TextureProperties properties{};
		properties.format = fmt;
		properties.filePath = "dummy.png";
		properties.srgb = true;
		properties.generateMips = true;
		return properties;
	}

	class TextureStorageTest : public ::testing::Test
	{
	protected:
		ResourceManager manager{};
		FakeTextureDecoder decoder{};
		FakeTextureUploader uploader{};
		FakeJobSystem jobSystem{};
		FakeRenderQueue renderQueue{};

		TextureIO MakeTextureIO()
		{
			return MakeIO(decoder, uploader, jobSystem, renderQueue);
		}

		void SetUp() override
		{
			ClearStorage();
		}

		void TearDown() override
		{
			ClearStorage();
		}

		void DrainAsyncPipeline()
		{
			auto io = MakeTextureIO();
			jobSystem.Drain();
			manager.ProcessUploads<TextureResource>(io, 64, 64);
			renderQueue.Drain();
		}

		void ClearStorage()
		{
			auto io = MakeTextureIO();
			manager.Clear<TextureResource>();
			jobSystem.Drain();
			manager.ProcessUploads<TextureResource>(io, 64, 64);
			renderQueue.Drain();
			uploader.createdIds.clear();
			uploader.destroyedIds.clear();
		}
	};
}

TEST_F(TextureStorageTest, LoadAndCreateEntrySucceeds)
{
	auto io = MakeTextureIO();
	auto texture = manager.LoadAsync<TextureResource>("tex1", io, MakeProperties(TextureFormat::RGBA));
	ASSERT_TRUE(texture);

	auto& storage = manager.GetStorage<TextureResource>();
	EXPECT_EQ(storage.GetState("tex1"), ResourceState::Loading);
	EXPECT_EQ(storage.GetStreamingStats().loadingEntries, 1u);

	DrainAsyncPipeline();

	EXPECT_EQ(storage.GetState("tex1"), ResourceState::Loaded);
	EXPECT_EQ(storage.GetStreamingStats().loadedEntries, 1u);
	EXPECT_NE(texture->GetResource().id, 0u);
	ASSERT_EQ(uploader.createdIds.size(), 1u);
}

TEST_F(TextureStorageTest, DecodeFailureSetsFailedState)
{
	decoder.succeed = false;
	auto io = MakeTextureIO();

	auto texture = manager.LoadAsync<TextureResource>("tex_fail", io, MakeProperties());
	ASSERT_TRUE(texture);

	jobSystem.Drain();

	auto& storage = manager.GetStorage<TextureResource>();
	EXPECT_EQ(storage.GetState("tex_fail"), ResourceState::Failed);
	EXPECT_FALSE(storage.GetError("tex_fail").empty());
	EXPECT_TRUE(uploader.createdIds.empty());
}

TEST_F(TextureStorageTest, FailedLoadCanBeRestartedAndLoaded)
{
	auto io = MakeTextureIO();
	decoder.succeed = false;

	auto first = manager.LoadAsync<TextureResource>("tex_restart", io, MakeProperties());
	ASSERT_TRUE(first);
	jobSystem.Drain();
	EXPECT_EQ(manager.GetState<TextureResource>("tex_restart"), ResourceState::Failed);

	decoder.succeed = true;
	auto second = manager.LoadAsync<TextureResource>("tex_restart", io, MakeProperties(TextureFormat::RGB));
	EXPECT_EQ(first.get(), second.get());
	EXPECT_EQ(manager.GetState<TextureResource>("tex_restart"), ResourceState::Loading);

	DrainAsyncPipeline();

	EXPECT_EQ(manager.GetState<TextureResource>("tex_restart"), ResourceState::Loaded);
	EXPECT_EQ(second->GetProperties().format, TextureFormat::RGB);
	EXPECT_NE(second->GetResource().id, 0u);
}

TEST_F(TextureStorageTest, UnloadUnusedQueuesDestroyAndRemovesEntry)
{
	auto io = MakeTextureIO();
	auto texture = manager.LoadAsync<TextureResource>("tex_unused", io, MakeProperties());
	ASSERT_TRUE(texture);
	DrainAsyncPipeline();

	const std::uint32_t createdId = texture->GetResource().id;
	ASSERT_NE(createdId, 0u);

	texture.reset();
	manager.UnloadUnused<TextureResource>();
	manager.ProcessUploads<TextureResource>(io, 64, 64);
	renderQueue.Drain();

	EXPECT_EQ(manager.GetState<TextureResource>("tex_unused"), ResourceState::Unknown);
	ASSERT_EQ(uploader.destroyedIds.size(), 1u);
	EXPECT_EQ(uploader.destroyedIds.front(), createdId);
}
