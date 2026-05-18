#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <stdexcept>

#include "FakeTextureIO.h"
#include "FakeRHI.h"

import core;

struct FakeMeshLoader
{
	bool succeed{ true };
	std::uint32_t callCount{ 0 };
	rendern::MeshCPU nextMesh{};
	std::string errorMessage{ "Fake mesh decode failure" };

	std::optional<rendern::MeshCPU> Decode(const rendern::MeshProperties&, std::string_view)
	{
		++callCount;
		if (!succeed)
		{
			throw std::runtime_error(errorMessage);
		}
		if (nextMesh.vertices.empty())
		{
			nextMesh.vertices = {
				rendern::VertexDesc{0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f},
				rendern::VertexDesc{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f},
				rendern::VertexDesc{0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f},
			};
			nextMesh.indices = { 0u, 1u, 2u };
		}
		return nextMesh;
	}
};

struct FakeMeshUploader
{
	bool succeed{ true };
	std::uint32_t uploadCount{ 0 };
	std::uint32_t nextHandleId{ 1000 };

	std::vector<rendern::MeshCPU> uploadedCpu{};
	std::vector<rendern::MeshRHI> destroyedMeshes{};

	// Keep mesh storage tests focused on ResourceManager state transitions.
	// Do not call production UploadMesh here: that would couple these tests to
	// FakeRHIDevice behavior and overlap with RenderGraph/RHI tests.
	rendern::MeshRHI Upload(const rendern::MeshCPU& cpu, std::string_view)
	{
		++uploadCount;

		if (!succeed)
		{
			throw std::runtime_error("Fake mesh upload failure");
		}

		uploadedCpu.push_back(cpu);

		rendern::MeshRHI mesh{};
		mesh.vertexBuffer.id = nextHandleId++;
		mesh.indexBuffer.id = nextHandleId++;
		mesh.layout.id = nextHandleId++;
		mesh.layoutInstanced = mesh.layout;
		mesh.vertexStrideBytes = rendern::strideVDBytes;
		mesh.indexCount = static_cast<std::uint32_t>(cpu.indices.size());
		mesh.indexType = rhi::IndexType::UINT32;

		return mesh;
	}

	void Destroy(rendern::MeshRHI& mesh)
	{
		if (mesh.vertexBuffer.id != 0 || mesh.indexBuffer.id != 0)
		{
			destroyedMeshes.push_back(mesh);
		}

		mesh = {};
	}
};

inline rendern::MeshIO MakeMeshIO(
	FakeRHIDevice& device,
	FakeJobSystem& jobSystem,
	FakeRenderQueue& renderQueue,
	FakeMeshLoader& loader,
	FakeMeshUploader& uploader)
{
	rendern::MeshIO io{ device, jobSystem, renderQueue };
	io.loadMesh = [&loader](const rendern::MeshProperties& properties, std::string_view id)
	{
		return loader.Decode(properties, id);
	};
	io.uploadMesh = [&uploader](const rendern::MeshCPU& cpu, std::string_view debugName)
	{
		return uploader.Upload(cpu, debugName);
	};

	io.destroyMesh = [&uploader](rendern::MeshRHI& mesh)
	{
		uploader.Destroy(mesh);
	};
	return io;
}