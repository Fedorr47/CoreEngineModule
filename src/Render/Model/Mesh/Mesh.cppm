module;

#include <cstdint>
#include <vector>
#include <string>
#include <span>

export module core:mesh;

import :rhi;

export namespace rendern
{

	struct VertexDesc
	{
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
	};

	constexpr std::uint32_t strideVDBytes = static_cast<std::uint32_t>(sizeof(VertexDesc));

	struct MeshCPU
	{
		std::vector<VertexDesc> vertices;
		std::vector<std::uint32_t> indices;
	};

	struct MeshRHI
	{
		rhi::BufferHandle vertexBuffer;
		rhi::BufferHandle indexBuffer;
		rhi::InputLayoutHandle layout;

		std::uint32_t vertexStrideBytes{ sizeof(VertexDesc) };
		std::uint32_t indexCount{ 0 };
		rhi::IndexType indexType{ rhi::IndexType::UINT32 };
	};

	inline rhi::InputLayoutHandle CreateVertexDescLayout(rhi::IRHIDevice& device, std::string_view name = "VertexDecs")
	{
		rhi::InputLayoutDesc desc{};
		desc.debugName = std::string(name);
		desc.strideBytes = strideVDBytes;
		desc.attributes = {
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Position, .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32_FLOAT, .offsetBytes = 0},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Normal,	.semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32_FLOAT, .offsetBytes = 12},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord, .semanticIndex = 0, .format = rhi::VertexFormat::R32G32_FLOAT,	  .offsetBytes = 24},
		};
		return device.CreateInputLayout(desc);
	}

	inline MeshRHI UploadMesh(rhi::IRHIDevice& device, const MeshCPU& cpu, std::string_view debugName = "Mesh")
	{
		MeshRHI outMeshRHI;
		outMeshRHI.vertexStrideBytes = strideVDBytes;
		outMeshRHI.indexCount = static_cast<std::uint32_t>(cpu.indices.size());
		
		outMeshRHI.layout = CreateVertexDescLayout(device, debugName);

		// Vertex buffer
		{
			rhi::BufferDesc vertexBuffer{};
			vertexBuffer.bindFlag = rhi::BufferBindFlag::VertexBuffer;
			vertexBuffer.usageFlag = rhi::BufferUsageFlag::Static;
			vertexBuffer.sizeInBytes = cpu.vertices.size() * sizeof(VertexDesc);
			vertexBuffer.debugName = std::string(debugName) + "_VB";

			outMeshRHI.vertexBuffer = device.CreateBuffer(vertexBuffer);
			if (!cpu.vertices.empty())
			{
				device.UpdateBuffer(outMeshRHI.vertexBuffer, std::as_bytes(std::span(cpu.vertices)));
			}
		}

		// Index Buffer
		{
			rhi::BufferDesc indexBuffer{};
			indexBuffer.bindFlag = rhi::BufferBindFlag::IndexBuffer;
			indexBuffer.usageFlag = rhi::BufferUsageFlag::Static;
			indexBuffer.sizeInBytes = cpu.indices.size() * sizeof(std::uint32_t);
			indexBuffer.debugName = std::string(debugName) + "_IB";

			outMeshRHI.indexBuffer = device.CreateBuffer(indexBuffer);
			if (!cpu.indices.empty())
			{
				device.UpdateBuffer(outMeshRHI.indexBuffer, std::as_bytes(std::span(cpu.indices)));
			}
		}

		return outMeshRHI;
	}

	inline void DestroyMesh(rhi::IRHIDevice& device, MeshRHI& mesh) noexcept
	{
		if (mesh.indexBuffer)
		{
			device.DestroyBuffer(mesh.indexBuffer);
		}
		if (mesh.vertexBuffer)
		{
			device.DestroyBuffer(mesh.vertexBuffer);
		}
		if (mesh.layout)
		{
			device.DestroyInputLayout(mesh.layout);
		}
		mesh = {};
	}
}