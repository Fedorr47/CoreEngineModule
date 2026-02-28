module;

#include <cstdint>
#include <cstring>

export module core:debug_draw_renderer_dx12;

import std;

import :rhi;
import :render_core;
import :math_utils;
import :file_system;
import :debug_draw;

export namespace rendern::debugDraw
{
	class DebugDrawRendererDX12
	{
	public:
		DebugDrawRendererDX12(rhi::IRHIDevice& device, ShaderLibrary& shaderLibrary, PSOCache& psoCache)
			: device_(device)
			, shaderLibrary_(shaderLibrary)
			, psoCache_(psoCache)
		{
		}

		void Upload(const DebugDrawList& list)
		{
			EnsureResources();

			lastDepthVertexCount_ = static_cast<std::uint32_t>(list.DepthVertexCount());
			lastOverlayVertexCount_ = static_cast<std::uint32_t>(list.OverlayVertexCount());
			lastVertexCount_ = lastDepthVertexCount_ + lastOverlayVertexCount_;
			if (lastVertexCount_ == 0)
			{
				return;
			}

			EnsureVertexBufferCapacity(lastVertexCount_);

			uploadScratch_.clear();
			uploadScratch_.reserve(list.lineVertices.size() + list.overlayLineVertices.size());
			uploadScratch_.insert(uploadScratch_.end(), list.lineVertices.begin(), list.lineVertices.end());
			uploadScratch_.insert(uploadScratch_.end(), list.overlayLineVertices.begin(), list.overlayLineVertices.end());

			const std::span<const DebugVertex> verts{ uploadScratch_.data(), uploadScratch_.size() };
			device_.UpdateBuffer(vertexBuffer_, std::as_bytes(verts), 0);
		}

		void Draw(rhi::CommandList& cmd, const mathUtils::Mat4& viewProj, bool depthTest)
		{
			if (lastVertexCount_ == 0)
			{
				return;
			}

			EnsureResources();

			cmd.BindPipeline(psoLines_);
			cmd.BindInputLayout(inputLayout_);
			cmd.BindVertexBuffer(0, vertexBuffer_, static_cast<std::uint32_t>(sizeof(DebugVertex)), 0);
			cmd.SetPrimitiveTopology(rhi::PrimitiveTopology::LineList);

			struct alignas(16) Constants
			{
				std::array<float, 16> uViewProj{};
			};

			Constants c{};
			const mathUtils::Mat4 vpT = mathUtils::Transpose(viewProj);
			std::memcpy(c.uViewProj.data(), mathUtils::ValuePtr(vpT), sizeof(float) * 16);
			cmd.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

			if (lastDepthVertexCount_ > 0)
			{
				rhi::GraphicsState depthState{};
				depthState.depth.testEnable = depthTest;
				depthState.depth.writeEnable = false;
				depthState.depth.depthCompareOp = rhi::CompareOp::LessEqual;
				depthState.rasterizer.cullMode = rhi::CullMode::None;
				depthState.blend.enable = false;

				cmd.SetState(depthState);
				cmd.Draw(lastDepthVertexCount_, 0);
			}

			if (lastOverlayVertexCount_ > 0)
			{
				rhi::GraphicsState overlayState{};
				overlayState.depth.testEnable = false;
				overlayState.depth.writeEnable = false;
				overlayState.depth.depthCompareOp = rhi::CompareOp::Always;
				overlayState.rasterizer.cullMode = rhi::CullMode::None;
				overlayState.blend.enable = false;

				cmd.SetState(overlayState);
				cmd.Draw(lastOverlayVertexCount_, lastDepthVertexCount_);
			}
		}

		void Shutdown()
		{
			if (vertexBuffer_)
			{
				device_.DestroyBuffer(vertexBuffer_);
				vertexBuffer_ = {};
			}
			vbCapacityVertices_ = 0;
			lastVertexCount_ = 0;
			lastDepthVertexCount_ = 0;
			lastOverlayVertexCount_ = 0;
			uploadScratch_.clear();

			if (inputLayout_)
			{
				device_.DestroyInputLayout(inputLayout_);
				inputLayout_ = {};
			}

			psoLines_ = {};
			initialized_ = false;
		}

	private:
		void EnsureResources()
		{
			if (initialized_)
			{
				return;
			}

			rhi::InputLayoutDesc il{};
			il.debugName = "DebugLinesInputLayout";
			il.strideBytes = static_cast<std::uint32_t>(sizeof(DebugVertex));
			il.attributes = {
				rhi::VertexAttributeDesc{
					.semantic = rhi::VertexSemantic::Position,
					.semanticIndex = 0,
					.format = rhi::VertexFormat::R32G32B32_FLOAT,
					.inputSlot = 0,
					.offsetBytes = 0,
					.normalized = false
				},
				rhi::VertexAttributeDesc{
					.semantic = rhi::VertexSemantic::Color,
					.semanticIndex = 0,
					.format = rhi::VertexFormat::R8G8B8A8_UNORM,
					.inputSlot = 0,
					.offsetBytes = 12,
					.normalized = true
				},
			};
			inputLayout_ = device_.CreateInputLayout(il);

			const std::filesystem::path shaderPath = corefs::ResolveAsset("shaders\\DebugLines_dx12.hlsl");

			rendern::ShaderKey vsKey{};
			vsKey.stage = rhi::ShaderStage::Vertex;
			vsKey.name = "VS_DebugLines";
			vsKey.filePath = shaderPath.string();
			rhi::ShaderHandle vs = shaderLibrary_.GetOrCreateShader(vsKey);

			rendern::ShaderKey psKey{};
			psKey.stage = rhi::ShaderStage::Pixel;
			psKey.name = "PS_DebugLines";
			psKey.filePath = shaderPath.string();
			rhi::ShaderHandle ps = shaderLibrary_.GetOrCreateShader(psKey);

			psoLines_ = psoCache_.GetOrCreate("PSO_DebugLines", vs, ps, rhi::PrimitiveTopologyType::Line);

			EnsureVertexBufferCapacity(4096);

			initialized_ = true;
		}

		void EnsureVertexBufferCapacity(std::uint32_t vertexCount)
		{
			if (vertexBuffer_.id != 0 && vbCapacityVertices_ >= vertexCount)
			{
				return;
			}

			std::uint32_t newCap = std::max<std::uint32_t>(vertexCount, 4096u);
			if (vbCapacityVertices_ != 0)
			{
				newCap = std::max<std::uint32_t>(newCap, vbCapacityVertices_ * 2u);
			}

			vbCapacityVertices_ = newCap;

			if (vertexBuffer_.id != 0)
			{
				device_.DestroyBuffer(vertexBuffer_);
				vertexBuffer_ = {};
			}

			rhi::BufferDesc vbDesc{};
			vbDesc.bindFlag = rhi::BufferBindFlag::VertexBuffer;
			vbDesc.usageFlag = rhi::BufferUsageFlag::Dynamic;
			vbDesc.sizeInBytes = static_cast<std::size_t>(vbCapacityVertices_) * sizeof(DebugVertex);
			vbDesc.debugName = "DebugLinesVB";
			vertexBuffer_ = device_.CreateBuffer(vbDesc);
		}

	private:
		rhi::IRHIDevice& device_;
		ShaderLibrary& shaderLibrary_;
		PSOCache& psoCache_;

		rhi::InputLayoutHandle inputLayout_{};
		rhi::PipelineHandle psoLines_{};
		rhi::BufferHandle vertexBuffer_{};
		std::vector<DebugVertex> uploadScratch_{};
		std::uint32_t vbCapacityVertices_{ 0 };
		std::uint32_t lastVertexCount_{ 0 };
		std::uint32_t lastDepthVertexCount_{ 0 };
		std::uint32_t lastOverlayVertexCount_{ 0 };
		bool initialized_{ false };
	};
}