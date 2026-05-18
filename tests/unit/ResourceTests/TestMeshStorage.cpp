#include <gtest/gtest.h>

#include "FakeMeshIO.h"

namespace
{
    rendern::MeshProperties MakeProperties()
    {
        rendern::MeshProperties properties{};
        properties.filePath = "fake.mesh";
        properties.debugName = "FakeMesh";
        return properties;
    }

    class MeshStorageTest : public ::testing::Test
    {
    protected:
        ResourceManager manager{};
        FakeRHIDevice device{};
        FakeMeshLoader loader{};
        FakeMeshUploader uploader{};
        FakeJobSystem jobSystem{};
        FakeRenderQueue renderQueue{};

        rendern::MeshIO MakeIO()
        {
            return MakeMeshIO(device, jobSystem, renderQueue, loader, uploader);
        }

        void DrainAsyncPipeline()
        {
            auto io = MakeIO();
            jobSystem.Drain();
            manager.ProcessUploads<rendern::MeshResource>(io, 64, 64);
            renderQueue.Drain();
        }

        void ClearStorage()
        {
            auto io = MakeIO();
            manager.Clear<rendern::MeshResource>();
            manager.ProcessUploads<rendern::MeshResource>(io, 64, 64);
            renderQueue.Drain();
            uploader.destroyedMeshes.clear();
            uploader.uploadedCpu.clear();
            uploader.uploadCount = 0;
            loader.callCount = 0;
        }

        void SetUp() override { ClearStorage(); }
        void TearDown() override { ClearStorage(); }
    };
}

TEST_F(MeshStorageTest, LoadAsyncSuccessfulMeshTransitionsToLoaded)
{
	auto io = MakeIO();
	auto mesh = manager.LoadAsync<rendern::MeshResource>("mesh_ok", io, MakeProperties());
	ASSERT_TRUE(mesh);
	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_ok"), ResourceState::Loading);

	DrainAsyncPipeline();

	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_ok"), ResourceState::Loaded);
	EXPECT_EQ(loader.callCount, 1u);
	EXPECT_EQ(uploader.uploadCount, 1u);
	EXPECT_EQ(uploader.uploadedCpu.size(), 1u);
	EXPECT_EQ(uploader.uploadedCpu.front().indices.size(), 3u);
	EXPECT_NE(mesh->GetResource().vertexBuffer.id, 0u);
	EXPECT_NE(mesh->GetResource().indexBuffer.id, 0u);
}

TEST_F(MeshStorageTest, DecodeFailureTransitionsToFailedWithoutUpload)
{
	loader.succeed = false;
	auto io = MakeIO();
	auto mesh = manager.LoadAsync<rendern::MeshResource>("mesh_decode_fail", io, MakeProperties());
	ASSERT_TRUE(mesh);

	DrainAsyncPipeline();

	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_decode_fail"), ResourceState::Failed);
	EXPECT_EQ(loader.callCount, 1u);
	EXPECT_EQ(uploader.uploadCount, 0u);
	EXPECT_TRUE(uploader.destroyedMeshes.empty());
}

TEST_F(MeshStorageTest, UploadFailureTransitionsToFailedAndDoesNotLeak)
{
	uploader.succeed = false;
	auto io = MakeIO();
	auto mesh = manager.LoadAsync<rendern::MeshResource>("mesh_upload_fail", io, MakeProperties());
	ASSERT_TRUE(mesh);

	DrainAsyncPipeline();

	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_upload_fail"), ResourceState::Failed);
	EXPECT_EQ(uploader.uploadCount, 1u);
	EXPECT_EQ(mesh->GetResource().vertexBuffer.id, 0u);
	EXPECT_EQ(mesh->GetResource().indexBuffer.id, 0u);
	EXPECT_TRUE(uploader.destroyedMeshes.empty());
}

TEST_F(MeshStorageTest, RetryAfterFailureRestartsAndLoadsSuccessfully)
{
	auto io = MakeIO();
	loader.succeed = false;
	auto first = manager.LoadAsync<rendern::MeshResource>("mesh_retry", io, MakeProperties());
	ASSERT_TRUE(first);
	DrainAsyncPipeline();
	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_retry"), ResourceState::Failed);

	loader.succeed = true;
	auto second = manager.LoadAsync<rendern::MeshResource>("mesh_retry", io, MakeProperties());
	ASSERT_TRUE(second);
	EXPECT_EQ(first.get(), second.get());
	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_retry"), ResourceState::Loading);

	DrainAsyncPipeline();
	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_retry"), ResourceState::Loaded);
	EXPECT_EQ(uploader.uploadCount, 1u);
}

TEST_F(MeshStorageTest, DuplicateInFlightRequestReusesSingleLoadOperation)
{
	auto io = MakeIO();
	auto first = manager.LoadAsync<rendern::MeshResource>("mesh_dup", io, MakeProperties());
	auto second = manager.LoadAsync<rendern::MeshResource>("mesh_dup", io, MakeProperties());
	ASSERT_TRUE(first);
	ASSERT_TRUE(second);
	EXPECT_EQ(first.get(), second.get());
	EXPECT_EQ(jobSystem.jobs.size(), 1u);

	DrainAsyncPipeline();
	EXPECT_EQ(loader.callCount, 1u);
	EXPECT_EQ(uploader.uploadCount, 1u);
}

TEST_F(MeshStorageTest, ClearLoadedMeshesDestroysUploadedResources)
{
	auto io = MakeIO();
	auto a = manager.LoadAsync<rendern::MeshResource>("mesh_a", io, MakeProperties());
	auto b = manager.LoadAsync<rendern::MeshResource>("mesh_b", io, MakeProperties());
	ASSERT_TRUE(a);
	ASSERT_TRUE(b);
	DrainAsyncPipeline();
	ASSERT_EQ(manager.GetState<rendern::MeshResource>("mesh_a"), ResourceState::Loaded);
	ASSERT_EQ(manager.GetState<rendern::MeshResource>("mesh_b"), ResourceState::Loaded);

	manager.Clear<rendern::MeshResource>();
	manager.ProcessUploads<rendern::MeshResource>(io, 64, 64);
	renderQueue.Drain();

	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_a"), ResourceState::Unknown);
	EXPECT_EQ(manager.GetState<rendern::MeshResource>("mesh_b"), ResourceState::Unknown);
	EXPECT_EQ(uploader.destroyedMeshes.size(), 2u);
}