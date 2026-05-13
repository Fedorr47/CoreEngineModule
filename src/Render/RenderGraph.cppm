module;

#include <cstdint>
#include <string>
#include <optional>
#include <span>
#include <functional>
#include <vector>
#include <chrono>
#include <algorithm>

export module core:render_graph;

import :rhi;

export namespace renderGraph
{
	enum class ResourceUsage : std::uint8_t
	{
		Unknown,
		RenderTarget,
		DepthStencil,
		Sampled,
		Storage
	};


	enum class TextureType : std::uint8_t
	{
		Tex2D,
		Cube
	};
struct RGTextureDesc
	{
		rhi::Extent2D extent{ 0, 0 };
		rhi::Format format{ rhi::Format::Unknown };
		// If set, this texture is provided by the caller and will not be created/destroyed by the render graph.
		rhi::TextureHandle externalTexture{};
		ResourceUsage usage{ ResourceUsage::Unknown };
		TextureType type{ TextureType::Tex2D };
		std::string debugName;
	};

	struct RGTexture
	{
		std::uint32_t id{};
	};

	using RGTextureHandle = RGTexture;

	struct PassAttachments
	{
		bool useSwapChainBackbuffer{ false };
		// If false, depth-stencil is not bound in BeginPass.
		bool bindDepthStencil{ true };

		std::vector<RGTexture> colors;
		std::optional<RGTexture> depth;


		// If set, color attachment is treated as a cubemap and this selects the face [0..5].
		std::optional<std::uint32_t> colorCubeFace;
		// Optional mip level for cubemap color attachment. Used for reflection-probe prefilter passes.
		std::optional<std::uint32_t> colorCubeMip;
		// If true, color cubemap is rendered as all faces (array layers) in a single pass (e.g. DX12 view-instancing).
		bool colorCubeAllFaces{ false };
		rhi::ClearDesc clearDesc{};
	};

	class RenderGraphResources
	{
	public:
		RenderGraphResources(std::span<const rhi::TextureHandle> textures)
			: textures_(textures)
		{
		}

		rhi::TextureHandle GetTexture(const RGTexture& texture) const
		{
			if (texture.id >= textures_.size())
			{
				return {};
			}
			return textures_[texture.id];
		}

	private:
		std::span<const rhi::TextureHandle> textures_;
	};

	struct PassContext
	{
		rhi::IRHIDevice& device;
		rhi::IRHISwapChain& swapChain;
		rhi::CommandList& commandList;
		const RenderGraphResources& resources;
		rhi::Extent2D passExtent;
	};

	using PassCallback = std::function<void(PassContext&)>;

	struct PassNode
	{
		std::string name;
		PassAttachments attachments;
		PassCallback execute;
	};
	
	struct PassCpuTiming
	{
		std::string name;
		double cpuMs{ 0.0 };
	};

	struct ExecuteCpuTimings
	{
		double createTexturesMs{ 0.0 };
		double createFramebuffersMs{ 0.0 };
		double executePassesMs{ 0.0 };
		double destroyFramebuffersMs{ 0.0 };
		double destroyTexturesMs{ 0.0 };
		double submitCommandListMs{ 0.0 };
		double totalMs{ 0.0 };
		std::vector<PassCpuTiming> passTimings{};
	};
	
	struct ExecuteOptions
	{
		bool enableCpuTiming{ false };
	};

	class RenderGraph
	{
	public:
		RGTexture CreateTexture(RGTextureDesc desc)
		{
			const std::uint32_t id = static_cast<std::uint32_t>(textures_.size());
			textures_.emplace_back(std::move(desc));
			return RGTexture{ .id = id };
		}

		RGTexture ImportTexture(rhi::TextureHandle externalTexture, RGTextureDesc desc)
		{
			desc.externalTexture = externalTexture;
			return CreateTexture(std::move(desc));
		}

		void AddPass(std::string_view name, PassAttachments attachments, PassCallback callback)
		{
			passes_.emplace_back(PassNode{.name = std::string(name), .attachments = std::move(attachments), .execute = std::move(callback) });
		}

		void AddSwapChainPass(std::string_view name, rhi::ClearDesc clearDesc, PassCallback callback, bool bindDepthStencil = true)
		{
			PassAttachments attachments{};
			attachments.useSwapChainBackbuffer = true;
			attachments.bindDepthStencil = bindDepthStencil;
			attachments.clearDesc = clearDesc;
			passes_.emplace_back(PassNode{ .name = std::string(name), .attachments = std::move(attachments), .execute = std::move(callback) });
		}

		void Reset()
		{
			passes_.clear();
			textures_.clear();
		}

		void Execute(rhi::IRHIDevice& device, rhi::IRHISwapChain& swapChain, ExecuteOptions options = {})
		{
			lastExecuteCpuTimings_ = {};
			const auto totalStart = std::chrono::steady_clock::now();
			std::vector<rhi::TextureHandle> allocatedTextures;
			allocatedTextures.reserve(textures_.size());
			std::vector<std::uint8_t> owned;
			owned.reserve(textures_.size());
			const auto createTexturesStart = std::chrono::steady_clock::now();
			for (const auto& texDesc : textures_)
			{
				if (texDesc.externalTexture)
				{
					allocatedTextures.push_back(texDesc.externalTexture);
					owned.push_back(0);
					continue;
				}

				rhi::TextureHandle texture = (texDesc.type == TextureType::Cube)
					? device.CreateTextureCube(texDesc.extent, texDesc.format)
					: device.CreateTexture2D(texDesc.extent, texDesc.format);
				allocatedTextures.push_back(texture);
				owned.push_back(1);
			}
			const auto createTexturesEnd = std::chrono::steady_clock::now();
			if (options.enableCpuTiming)
			{
				lastExecuteCpuTimings_.createTexturesMs = std::chrono::duration<double, std::milli>(createTexturesEnd - createTexturesStart).count();
			}

			RenderGraphResources resources(allocatedTextures);
			rhi::CommandList commandList;

			std::vector<rhi::FrameBufferHandle> transientFramebuffers;
			transientFramebuffers.reserve(passes_.size());

			const auto executePassesStart = std::chrono::steady_clock::now();
			for (auto& pass : passes_)
			{
				const auto passStart = std::chrono::steady_clock::now();
				rhi::FrameBufferHandle frameBuffer{};
				rhi::Extent2D passExtent{ 0, 0 };

				if (pass.attachments.useSwapChainBackbuffer)
				{
					frameBuffer = swapChain.GetCurrentBackBuffer();
					passExtent = swapChain.GetDesc().extent;
				}
				else
				{
					std::vector<rhi::TextureHandle> colors;
					colors.reserve(pass.attachments.colors.size());

					for (const auto& c : pass.attachments.colors)
					{
						colors.push_back(resources.GetTexture(c));
					}

					const auto depth = pass.attachments.depth ? resources.GetTexture(*pass.attachments.depth) : rhi::TextureHandle();

					if (!pass.attachments.colors.empty())
					{
						passExtent = textures_[pass.attachments.colors.front().id].extent;
					}
					else if (pass.attachments.depth)
					{
						passExtent = textures_[pass.attachments.depth->id].extent;
					}

					// Cubemap rendering is only supported for a single color attachment.
					const auto createFramebufferStart = std::chrono::steady_clock::now();
					if (pass.attachments.colorCubeAllFaces && colors.size() == 1 && colors[0].id != 0)
					{
						frameBuffer = device.CreateFramebufferCube(colors[0], depth);
					}
					else if (pass.attachments.colorCubeFace && colors.size() == 1 && colors[0].id != 0)
					{
						if (pass.attachments.colorCubeMip)
						{
							frameBuffer = device.CreateFramebufferCubeFaceMip(colors[0], *pass.attachments.colorCubeFace, *pass.attachments.colorCubeMip, depth);
						}
						else
						{
							frameBuffer = device.CreateFramebufferCubeFace(colors[0], *pass.attachments.colorCubeFace, depth);
						}
					}
					else
					{
						frameBuffer = device.CreateFramebufferMRT(colors, depth);
					}
					transientFramebuffers.push_back(frameBuffer);
					if (options.enableCpuTiming)
					{
						const auto createFramebufferEnd = std::chrono::steady_clock::now();
						lastExecuteCpuTimings_.createFramebuffersMs += std::chrono::duration<double, std::milli>(createFramebufferEnd - createFramebufferStart).count();
					}
				}

				rhi::BeginPassDesc begin{};
				begin.frameBuffer = frameBuffer;
				begin.extent = passExtent;
				begin.clearDesc = pass.attachments.clearDesc;
				begin.swapChain = pass.attachments.useSwapChainBackbuffer ? &swapChain : nullptr;
				begin.bindDepthStencil = pass.attachments.bindDepthStencil;

				commandList.BeginPass(begin);

				PassContext ctx{ device, swapChain, commandList, resources, passExtent };
				pass.execute(ctx);

				commandList.EndPass();
				if (options.enableCpuTiming)
				{
					const auto passEnd = std::chrono::steady_clock::now();
					lastExecuteCpuTimings_.passTimings.push_back(PassCpuTiming{
						.name = pass.name,
						.cpuMs = std::chrono::duration<double, std::milli>(passEnd - passStart).count()
					});
				}
			}
			const auto executePassesEnd = std::chrono::steady_clock::now();
			if (options.enableCpuTiming)
			{
				lastExecuteCpuTimings_.executePassesMs = std::chrono::duration<double, std::milli>(executePassesEnd - executePassesStart).count();
			}

			const auto submitStart = std::chrono::steady_clock::now();
			device.SubmitCommandList(std::move(commandList));
			const auto submitEnd = std::chrono::steady_clock::now();
			if (options.enableCpuTiming)
			{
				lastExecuteCpuTimings_.submitCommandListMs = std::chrono::duration<double, std::milli>(submitEnd - submitStart).count();
			}

			const auto destroyFramebuffersStart = std::chrono::steady_clock::now();

			for (auto frameBuffer : transientFramebuffers)
			{
				device.DestroyFramebuffer(frameBuffer);
			}
			const auto destroyFramebuffersEnd = std::chrono::steady_clock::now();
			if (options.enableCpuTiming)
			{
				lastExecuteCpuTimings_.destroyFramebuffersMs = std::chrono::duration<double, std::milli>(destroyFramebuffersEnd - destroyFramebuffersStart).count();
			}
			const auto destroyTexturesStart = std::chrono::steady_clock::now();
			for (std::size_t i = 0; i < allocatedTextures.size(); ++i)
			{
				if (owned[i] != 0)
				{
					device.DestroyTexture(allocatedTextures[i]);
				}
			}
			const auto destroyTexturesEnd = std::chrono::steady_clock::now();
			if (options.enableCpuTiming)
			{
				lastExecuteCpuTimings_.destroyTexturesMs = std::chrono::duration<double, std::milli>(destroyTexturesEnd - destroyTexturesStart).count();
				lastExecuteCpuTimings_.totalMs = std::chrono::duration<double, std::milli>(destroyTexturesEnd - totalStart).count();
				std::sort(lastExecuteCpuTimings_.passTimings.begin(), lastExecuteCpuTimings_.passTimings.end(), [](const PassCpuTiming& a, const PassCpuTiming& b) { return a.cpuMs > b.cpuMs; });
			}
		}
		
		const ExecuteCpuTimings& GetLastExecuteCpuTimings() const noexcept { return lastExecuteCpuTimings_; }
	private:
		std::vector<PassNode> passes_;
		std::vector<RGTextureDesc> textures_;
		ExecuteCpuTimings lastExecuteCpuTimings_{};
	};
}
