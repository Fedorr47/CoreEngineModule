module;

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module core:skinned_mesh;

import :rhi;
import :math_utils;
import :skeleton;

export namespace rendern
{
	inline constexpr std::uint32_t kMaxSkinWeightsPerVertex = 4;

	struct SkinnedVertexDesc
	{
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
		float tx, ty, tz, tw;
		std::uint16_t boneIndex0{ 0 };
		std::uint16_t boneIndex1{ 0 };
		std::uint16_t boneIndex2{ 0 };
		std::uint16_t boneIndex3{ 0 };
		float boneWeight0{ 1.0f };
		float boneWeight1{ 0.0f };
		float boneWeight2{ 0.0f };
		float boneWeight3{ 0.0f };
	};

	constexpr std::uint32_t strideSkinnedVDBytes = static_cast<std::uint32_t>(sizeof(SkinnedVertexDesc));

	struct SkinnedSubmesh
	{
		std::string name;
		std::uint32_t firstIndex{ 0 };
		std::uint32_t indexCount{ 0 };
		std::uint32_t materialIndex{ 0 };
	};

	struct SkinnedBounds
	{
		mathUtils::Vec3 aabbMin{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 aabbMax{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 sphereCenter{ 0.0f, 0.0f, 0.0f };
		float sphereRadius{ 0.0f };
	};

	struct PerClipBounds
	{
		std::string clipName;
		SkinnedBounds bounds{};
	};

	struct SkinnedMeshBounds
	{
		SkinnedBounds bindPoseBounds{};
		SkinnedBounds maxAnimatedBounds{};
		std::vector<PerClipBounds> perClipBounds;
	};

	struct SkinnedMeshCPU
	{
		std::vector<SkinnedVertexDesc> vertices;
		std::vector<std::uint32_t> indices;
		std::vector<SkinnedSubmesh> submeshes;
		Skeleton skeleton{};
		SkinnedMeshBounds bounds{};
	};

	struct SkinnedMeshRHI
	{
		rhi::BufferHandle vertexBuffer;
		rhi::BufferHandle indexBuffer;
		rhi::InputLayoutHandle layout;

		std::uint32_t vertexStrideBytes{ sizeof(SkinnedVertexDesc) };
		std::uint32_t indexCount{ 0 };
		rhi::IndexType indexType{ rhi::IndexType::UINT32 };
	};

	inline rhi::InputLayoutHandle CreateSkinnedVertexDescLayout(rhi::IRHIDevice& device, std::string_view name = "SkinnedVertexDesc")
	{
		rhi::InputLayoutDesc desc{};
		desc.debugName = std::string(name);
		desc.strideBytes = strideSkinnedVDBytes;
		desc.attributes = {
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Position,    .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32_FLOAT,    .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, px))},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Normal,      .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32_FLOAT,    .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, nx))},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord,    .semanticIndex = 0, .format = rhi::VertexFormat::R32G32_FLOAT,       .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, u))},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Tangent,     .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32A32_FLOAT, .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, tx))},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::BoneIndices, .semanticIndex = 0, .format = rhi::VertexFormat::R16G16B16A16_UINT,  .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, boneIndex0))},
			rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::BoneWeights, .semanticIndex = 0, .format = rhi::VertexFormat::R32G32B32A32_FLOAT, .offsetBytes = static_cast<std::uint32_t>(offsetof(SkinnedVertexDesc, boneWeight0))},
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

	inline void NormalizeBoneWeights(SkinnedVertexDesc& vertex) noexcept
	{
		std::array<float, kMaxSkinWeightsPerVertex> weights = {
			std::max(vertex.boneWeight0, 0.0f),
			std::max(vertex.boneWeight1, 0.0f),
			std::max(vertex.boneWeight2, 0.0f),
			std::max(vertex.boneWeight3, 0.0f)
		};

		const float sum = weights[0] + weights[1] + weights[2] + weights[3];
		if (sum > 1e-8f)
		{
			const float invSum = 1.0f / sum;
			for (float& w : weights)
			{
				w *= invSum;
			}
		}
		else
		{
			weights = { 1.0f, 0.0f, 0.0f, 0.0f };
			vertex.boneIndex0 = 0;
			vertex.boneIndex1 = 0;
			vertex.boneIndex2 = 0;
			vertex.boneIndex3 = 0;
		}

		vertex.boneWeight0 = weights[0];
		vertex.boneWeight1 = weights[1];
		vertex.boneWeight2 = weights[2];
		vertex.boneWeight3 = weights[3];
	}

	inline void NormalizeBoneWeights(SkinnedMeshCPU& cpu) noexcept
	{
		for (auto& v : cpu.vertices)
		{
			NormalizeBoneWeights(v);
		}
	}

	[[nodiscard]] inline SkinnedBounds ComputeBindPoseBounds(const SkinnedMeshCPU& cpu) noexcept
	{
		SkinnedBounds b{};
		if (cpu.vertices.empty())
		{
			return b;
		}

		const auto& v0 = cpu.vertices.front();
		float minX = v0.px, minY = v0.py, minZ = v0.pz;
		float maxX = v0.px, maxY = v0.py, maxZ = v0.pz;
		for (const auto& v : cpu.vertices)
		{
			minX = std::min(minX, v.px); minY = std::min(minY, v.py); minZ = std::min(minZ, v.pz);
			maxX = std::max(maxX, v.px); maxY = std::max(maxY, v.py); maxZ = std::max(maxZ, v.pz);
		}

		b.aabbMin = mathUtils::Vec3(minX, minY, minZ);
		b.aabbMax = mathUtils::Vec3(maxX, maxY, maxZ);
		b.sphereCenter = (b.aabbMin + b.aabbMax) * 0.5f;
		const mathUtils::Vec3 ext = b.aabbMax - b.sphereCenter;
		b.sphereRadius = mathUtils::Length(ext);
		return b;
	}

	inline void RefreshBindPoseBounds(SkinnedMeshCPU& cpu) noexcept
	{
		cpu.bounds.bindPoseBounds = ComputeBindPoseBounds(cpu);
		if (cpu.bounds.maxAnimatedBounds.sphereRadius <= 0.0f)
		{
			cpu.bounds.maxAnimatedBounds = cpu.bounds.bindPoseBounds;
		}
	}

	inline SkinnedMeshRHI UploadSkinnedMesh(rhi::IRHIDevice& device, const SkinnedMeshCPU& cpu, std::string_view debugName = "SkinnedMesh")
	{
		SkinnedMeshRHI outMeshRHI{};
		outMeshRHI.vertexStrideBytes = strideSkinnedVDBytes;
		outMeshRHI.indexCount = static_cast<std::uint32_t>(cpu.indices.size());
		outMeshRHI.layout = CreateSkinnedVertexDescLayout(device, debugName);

		{
			rhi::BufferDesc vertexBuffer{};
			vertexBuffer.bindFlag = rhi::BufferBindFlag::VertexBuffer;
			vertexBuffer.usageFlag = rhi::BufferUsageFlag::Static;
			vertexBuffer.sizeInBytes = cpu.vertices.size() * sizeof(SkinnedVertexDesc);
			vertexBuffer.debugName = std::string(debugName) + "_VB";

			outMeshRHI.vertexBuffer = device.CreateBuffer(vertexBuffer);
			if (!cpu.vertices.empty())
			{
				device.UpdateBuffer(outMeshRHI.vertexBuffer, std::as_bytes(std::span(cpu.vertices)));
			}
		}

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

	void ComputeTangents(SkinnedMeshCPU& cpu)
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

			const SkinnedVertexDesc& v0 = cpu.vertices[i0];
			const SkinnedVertexDesc& v1 = cpu.vertices[i1];
			const SkinnedVertexDesc& v2 = cpu.vertices[i2];

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
			SkinnedVertexDesc& v = cpu.vertices[i];
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

	inline void DestroySkinnedMesh(rhi::IRHIDevice& device, SkinnedMeshRHI& mesh) noexcept
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