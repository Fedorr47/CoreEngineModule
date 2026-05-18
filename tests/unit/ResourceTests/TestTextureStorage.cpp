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

TEST_F(TextureStorageTest, DuplicateInFlightRequestReusesSingleLoadOperation)
{
	auto io = MakeTextureIO();
	auto first = manager.LoadAsync<TextureResource>("tex_dup", io, MakeProperties(TextureFormat::RGBA));
	auto second = manager.LoadAsync<TextureResource>("tex_dup", io, MakeProperties(TextureFormat::RGB));

	ASSERT_TRUE(first);
	ASSERT_TRUE(second);
	
	// Regression guard: duplicate requests for the same texture key must not enqueue
	// another async load while the first request is still in-flight.
	EXPECT_EQ(first.get(), second.get());
	EXPECT_EQ(jobSystem.jobs.size(), 1u);
	EXPECT_EQ(manager.GetState<TextureResource>("tex_dup"), ResourceState::Loading);

	DrainAsyncPipeline();

	// The first request owns the load parameters; later duplicate requests only attach
	// to the existing in-flight entry and must not overwrite pending properties.
	EXPECT_EQ(manager.GetState<TextureResource>("tex_dup"), ResourceState::Loaded);
	EXPECT_EQ(first->GetProperties().format, TextureFormat::RGBA);
	ASSERT_EQ(uploader.createdIds.size(), 1u);
}

TEST_F(TextureStorageTest, RetryAfterUploadFailureRestartsAndLoadsSuccessfully)
{
	auto io = MakeTextureIO();
	uploader.succeed = false;

	auto first = manager.LoadAsync<TextureResource>("tex_upload_retry", io, MakeProperties());
	ASSERT_TRUE(first);
	DrainAsyncPipeline();
	
	// First attempt reaches Failed through the upload stage, not through decode.
	EXPECT_EQ(manager.GetState<TextureResource>("tex_upload_retry"), ResourceState::Failed);

	uploader.succeed = true;
	auto second = manager.LoadAsync<TextureResource>("tex_upload_retry", io, MakeProperties(TextureFormat::RGB));
	ASSERT_TRUE(second);
	
	// Retry contract: failed entries are reusable placeholders, but a new load attempt
	// must be scheduled and may use the new request properties.
	EXPECT_EQ(first.get(), second.get());
	EXPECT_EQ(manager.GetState<TextureResource>("tex_upload_retry"), ResourceState::Loading);

	DrainAsyncPipeline();

	// Successful retry must replace the failed state with Loaded and publish the
	// properties from the retry request.
	EXPECT_EQ(manager.GetState<TextureResource>("tex_upload_retry"), ResourceState::Loaded);
	EXPECT_EQ(second->GetProperties().format, TextureFormat::RGB);
	ASSERT_EQ(uploader.createdIds.size(), 1u);
}

TEST_F(TextureStorageTest, ClearResetsLoadedAndFailedState)
{
	auto io = MakeTextureIO();
	auto loaded = manager.LoadAsync<TextureResource>("tex_loaded", io, MakeProperties());
	ASSERT_TRUE(loaded);
	DrainAsyncPipeline();
	ASSERT_EQ(manager.GetState<TextureResource>("tex_loaded"), ResourceState::Loaded);
	const std::uint32_t loadedId = loaded->GetResource().id;

	decoder.succeed = false;
	auto failed = manager.LoadAsync<TextureResource>("tex_failed", io, MakeProperties());
	ASSERT_TRUE(failed);
	
	// Decode failure is enough to move the entry into Failed; no upload processing is expected.
	jobSystem.Drain();
	ASSERT_EQ(manager.GetState<TextureResource>("tex_failed"), ResourceState::Failed);

	manager.Clear<TextureResource>();
	
	// Process any queued upload/render work after Clear to catch stale callbacks that
	// could accidentally recreate cleared entries or leak/double-destroy resources.
	manager.ProcessUploads<TextureResource>(io, 64, 64);
	renderQueue.Drain();

	EXPECT_EQ(manager.GetState<TextureResource>("tex_loaded"), ResourceState::Unknown);
	EXPECT_EQ(manager.GetState<TextureResource>("tex_failed"), ResourceState::Unknown);
	
	// Only the successfully uploaded GPU resource should be destroyed; the failed
	// decode path never created an upload resource.
	ASSERT_EQ(uploader.destroyedIds.size(), 1u);
	EXPECT_EQ(uploader.destroyedIds.front(), loadedId);
}

TEST_F(TextureStorageTest, ClearWhileRequestPendingDropsInFlightState)
{
	auto io = MakeTextureIO();
	auto pending = manager.LoadAsync<TextureResource>("tex_pending", io, MakeProperties());
	ASSERT_TRUE(pending);
	ASSERT_EQ(manager.GetState<TextureResource>("tex_pending"), ResourceState::Loading);

	// Clear must immediately remove the visible in-flight state from storage.
	manager.Clear<TextureResource>();
	EXPECT_EQ(manager.GetState<TextureResource>("tex_pending"), ResourceState::Unknown);

	// Drain stale async work after Clear. Cleared in-flight requests must be ignored:
	// they should not recreate the entry and should not upload a GPU resource.
	DrainAsyncPipeline();
	EXPECT_EQ(manager.GetState<TextureResource>("tex_pending"), ResourceState::Unknown);
	EXPECT_TRUE(uploader.createdIds.empty());
}