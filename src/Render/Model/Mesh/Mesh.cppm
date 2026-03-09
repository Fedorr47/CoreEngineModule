module;

#include <cstdint>
#include <vector>
#include <string>
#include <span>
#include <cmath>

export module core:mesh;

import :rhi;
import :math_utils;

export namespace rendern
{

	struct VertexDesc
	{
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
		float tx, ty, tz, tw;
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
		// Base (per-vertex) layout: POSITION/NORMAL/TEXCOORD0/TANGENT
		rhi::InputLayoutHandle layout;
		// Instanced layout: base slot0 + model matrix (4x float4) in slot1 (DX12 only)
		rhi::InputLayoutHandle layoutInstanced;

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
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Tangent,  .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32A32_FLOAT, .offsetBytes = 32},
		};
		return device.CreateInputLayout(desc);
	}

	inline rhi::InputLayoutHandle CreateVertexDescLayoutInstanced(rhi::IRHIDevice& device, std::string_view name = "VertexDescInstanced")
	{
		rhi::InputLayoutDesc desc{};
		desc.debugName = std::string(name);
		desc.strideBytes = strideVDBytes; // slot0 stride
		desc.attributes = {
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Position,.semanticIndex = 0,.format = rhi::VertexFormat::R32G32B32_FLOAT,.inputSlot = 0,.offsetBytes = 0},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Normal,  .semanticIndex = 0,.format = rhi::VertexFormat::R32G32B32_FLOAT,.inputSlot = 0,.offsetBytes = 12},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,.semanticIndex = 0,.format = rhi::VertexFormat::R32G32_FLOAT,    .inputSlot = 0,.offsetBytes = 24},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Tangent, .semanticIndex = 0,.format = rhi::VertexFormat::R32G32B32A32_FLOAT,.inputSlot = 0,.offsetBytes = 32},

		// Instance matrix columns in slot1: TEXCOORD1..4
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,.semanticIndex = 1,.format = rhi::VertexFormat::R32G32B32A32_FLOAT,.inputSlot = 1,.offsetBytes = 0},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,.semanticIndex = 2,.format = rhi::VertexFormat::R32G32B32A32_FLOAT,.inputSlot = 1,.offsetBytes = 16},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,.semanticIndex = 3,.format = rhi::VertexFormat::R32G32B32A32_FLOAT,.inputSlot = 1,.offsetBytes = 32},
		rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,.semanticIndex = 4,.format = rhi::VertexFormat::R32G32B32A32_FLOAT,.inputSlot = 1,.offsetBytes = 48},
		};
		return device.CreateInputLayout(desc);
	}


	inline mathUtils::Vec3 BuildFallbackTangent(const mathUtils::Vec3& normal) noexcept
	{
		const mathUtils::Vec3 axis = (std::abs(normal.z) < 0.999f)
			? mathUtils::Vec3(0.0f, 0.0f, 1.0f)
			: mathUtils::Vec3(0.0f, 1.0f, 0.0f);
		return mathUtils::Normalize(mathUtils::Cross(axis, normal));
	}

	inline void ComputeTangents(MeshCPU& cpu)
	{
		if (cpu.vertices.empty())
		{
			return;
		}

		std::vector<mathUtils::Vec3> tan1(cpu.vertices.size(), mathUtils::Vec3(0.0f, 0.0f, 0.0f));
		std::vector<mathUtils::Vec3> tan2(cpu.vertices.size(), mathUtils::Vec3(0.0f, 0.0f, 0.0f));

		for (std::size_t i = 0; i + 2 < cpu.indices.size(); i += 3)
		{
			const std::uint32_t i0 = cpu.indices[i + 0];
			const std::uint32_t i1 = cpu.indices[i + 1];
			const std::uint32_t i2 = cpu.indices[i + 2];
			if (i0 >= cpu.vertices.size() || i1 >= cpu.vertices.size() || i2 >= cpu.vertices.size())
			{
				continue;
			}

			const VertexDesc& v0 = cpu.vertices[i0];
			const VertexDesc& v1 = cpu.vertices[i1];
			const VertexDesc& v2 = cpu.vertices[i2];

			const mathUtils::Vec3 p0(v0.px, v0.py, v0.pz);
			const mathUtils::Vec3 p1(v1.px, v1.py, v1.pz);
			const mathUtils::Vec3 p2(v2.px, v2.py, v2.pz);

			const float x1 = p1.x - p0.x;
			const float x2 = p2.x - p0.x;
			const float y1 = p1.y - p0.y;
			const float y2 = p2.y - p0.y;
			const float z1 = p1.z - p0.z;
			const float z2 = p2.z - p0.z;

			const float s1 = v1.u - v0.u;
			const float s2 = v2.u - v0.u;
			const float t1 = v1.v - v0.v;
			const float t2 = v2.v - v0.v;

			const float det = s1 * t2 - s2 * t1;
			if (std::abs(det) < 1e-8f)
			{
				continue;
			}

			const float invDet = 1.0f / det;
			const mathUtils::Vec3 sdir(
				(t2 * x1 - t1 * x2) * invDet,
				(t2 * y1 - t1 * y2) * invDet,
				(t2 * z1 - t1 * z2) * invDet);
			const mathUtils::Vec3 tdir(
				(s1 * x2 - s2 * x1) * invDet,
				(s1 * y2 - s2 * y1) * invDet,
				(s1 * z2 - s2 * z1) * invDet);

			tan1[i0] = tan1[i0] + sdir;
			tan1[i1] = tan1[i1] + sdir;
			tan1[i2] = tan1[i2] + sdir;
			tan2[i0] = tan2[i0] + tdir;
			tan2[i1] = tan2[i1] + tdir;
			tan2[i2] = tan2[i2] + tdir;
		}

		for (std::size_t i = 0; i < cpu.vertices.size(); ++i)
		{
			VertexDesc& v = cpu.vertices[i];
			const mathUtils::Vec3 n = mathUtils::Normalize(mathUtils::Vec3(v.nx, v.ny, v.nz));

			mathUtils::Vec3 t = tan1[i] - n * mathUtils::Dot(n, tan1[i]);
			if (mathUtils::Length(t) < 1e-6f)
			{
				t = BuildFallbackTangent(n);
			}
			else
			{
				t = mathUtils::Normalize(t);
			}

			const float handedness =
				(mathUtils::Dot(mathUtils::Cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

			v.tx = t.x;
			v.ty = t.y;
			v.tz = t.z;
			v.tw = handedness;
		}
	}


	inline MeshRHI UploadMesh(rhi::IRHIDevice& device, const MeshCPU& cpu, std::string_view debugName = "Mesh")
	{
		MeshRHI outMeshRHI;
		outMeshRHI.vertexStrideBytes = strideVDBytes;
		outMeshRHI.indexCount = static_cast<std::uint32_t>(cpu.indices.size());

		outMeshRHI.layout = CreateVertexDescLayout(device, debugName);
		if (device.GetBackend() == rhi::Backend::DirectX12)
		{
			outMeshRHI.layoutInstanced = CreateVertexDescLayoutInstanced(device, std::string(debugName) + "_Instanced");
		}
		else
		{
			outMeshRHI.layoutInstanced = outMeshRHI.layout;
		}

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
		if (mesh.layoutInstanced && mesh.layoutInstanced.id != mesh.layout.id)
		{
			device.DestroyInputLayout(mesh.layoutInstanced);
		}
		if (mesh.layout)
		{
			device.DestroyInputLayout(mesh.layout);
		}
		mesh = {};
	}

	MeshCPU MakeSkyboxCubeCPU()
	{
		using rendern::VertexDesc;
		rendern::MeshCPU cpu{};

		cpu.vertices = {
			// px,py,pz,  nx,ny,nz,   u,v, tx,ty,tz,tw
			VertexDesc{-1,-1,-1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{ 1,-1,-1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{ 1, 1,-1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{-1, 1,-1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{-1,-1, 1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{ 1,-1, 1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{ 1, 1, 1, 0,0,0, 0,0, 1,0,0,1},
			VertexDesc{-1, 1, 1, 0,0,0, 0,0, 1,0,0,1},
		};

		cpu.indices = {
			// -Z
			0,1,2,  2,3,0,
			// +Z
			4,6,5,  6,4,7,
			// -X
			4,0,3,  3,7,4,
			// +X
			1,5,6,  6,2,1,
			// -Y
			4,5,1,  1,0,4,
			// +Y
			3,2,6,  6,7,3
		};

		return cpu;
	}
}