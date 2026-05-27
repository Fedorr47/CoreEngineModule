#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <optional>

import core;

//---------------------- Fake Swap chain  ----------------------//
struct FakeRHISwapChain : rhi::IRHISwapChain
{
	explicit FakeRHISwapChain(
		rhi::SwapChainDesc swapChainDesc,
		rhi::FrameBufferHandle backBuffer = rhi::FrameBufferHandle{ 9001 },
		rhi::TextureHandle depthTexture = rhi::TextureHandle{ 9002 }
	);
	
	rhi::SwapChainDesc GetDesc() const override;
	rhi::FrameBufferHandle GetCurrentBackBuffer() const override;
	rhi::TextureHandle GetDepthTexture() const override;
	void Present() override;
	void Resize(rhi::Extent2D newExtent) override;
	
	std::uint32_t GetPresentCount() const noexcept;
	std::uint32_t GetResizeCount() const noexcept;
	const std::vector<rhi::Extent2D>& GetResizeHistory() const noexcept;
	
private:
	rhi::SwapChainDesc swapChainDesc_{};
	rhi::FrameBufferHandle backBuffer_{};
	rhi::TextureHandle depthTexture_{};
	std::uint32_t presentCount_{0};
	std::uint32_t resizeCount_{0};
	std::vector<rhi::Extent2D> resizeHistory_{};
};

struct TextureCreateEvent
{
	rhi::TextureHandle handle{};
	rhi::Extent2D extent{};
	rhi::Format format{rhi::Format::Unknown};
	bool isCubeMap{false};
};

struct FrameBufferCreateEvent
{
	rhi::FrameBufferHandle handle{};
	std::vector<rhi::TextureHandle> colors{};
	rhi::TextureHandle depth{};
	std::string kind{};
	std::optional<std::uint32_t> cubeFace{};
	std::optional<std::uint32_t> cubeMip{};
	bool cubeAllFaces{false};
};

//---------------------- Fake Device ----------------------//
struct FakeRHIDevice : rhi::IRHIDevice
{
	rhi::Backend GetBackend() const noexcept override;
	std::string_view GetName() const override;
	
	rhi::TextureHandle CreateTexture2D(rhi::Extent2D extent, rhi::Format format) override;
	rhi::TextureHandle CreateTextureCube(rhi::Extent2D extent, rhi::Format format) override;
	void DestroyTexture(rhi::TextureHandle texture) noexcept override;
	
	rhi::FrameBufferHandle CreateFramebuffer(rhi::TextureHandle color, rhi::TextureHandle depth) override;
	rhi::FrameBufferHandle CreateFramebufferMRT(::std::span<const rhi::Handle<rhi::TextureTag>> colors, rhi::TextureHandle depth) override;
	rhi::FrameBufferHandle CreateFramebufferCubeFace(rhi::TextureHandle colorCube, std::uint32_t faceIndex, rhi::TextureHandle depth) override;
	rhi::FrameBufferHandle CreateFramebufferCubeFaceMip(rhi::TextureHandle colorCube, std::uint32_t faceIndex, std::uint32_t mipLevel, rhi::TextureHandle depth) override;
	rhi::FrameBufferHandle CreateFramebufferCube(rhi::TextureHandle colorCube, rhi::TextureHandle depthCube) override;
	void DestroyFramebuffer(rhi::FrameBufferHandle framebuffer) noexcept override;
	
	rhi::BufferHandle CreateBuffer(const rhi::BufferDesc& desc) override;
	void UpdateBuffer(rhi::BufferHandle buffer, ::std::span<const std::byte> data, std::size_t offsetBytes) override;
	void DestroyBuffer(rhi::BufferHandle buffer) noexcept override;
	
	rhi::InputLayoutHandle CreateInputLayout(const rhi::InputLayoutDesc& desc) override;
	void DestroyInputLayout(rhi::InputLayoutHandle inputLayout) noexcept override;
	
	rhi::ShaderHandle CreateShader(rhi::ShaderStage stage, std::string_view debugName, std::string_view sourceOrBytecode) override;
	void DestroyShader(rhi::ShaderHandle shader) noexcept override;
	
	rhi::PipelineHandle CreatePipeline(
		std::string_view debugName, 
		rhi::ShaderHandle vertexShader, 
		rhi::ShaderHandle pixelShader, 
		rhi::PrimitiveTopologyType topologyType) override;
	void DestroyPipeline(rhi::PipelineHandle pipeline) noexcept override;
	
	void SubmitCommandList(rhi::CommandList&& commandList) override;
	
	rhi::TextureDescIndex AllocateTextureDesctiptor(rhi::TextureHandle texture) override;
	void UpdateTextureDescriptor(rhi::TextureDescIndex index, rhi::TextureHandle texture) override;
	void FreeTextureDescriptor(rhi::TextureDescIndex index) noexcept override;
	
	rhi::FenceHandle CreateFence(bool signaled) override;
	void DestroyFence(rhi::FenceHandle fence) noexcept override;
	void SignalFence(rhi::FenceHandle fence) override;
	void WaitFence(rhi::FenceHandle fence) override;
	bool IsFenceSignaled(rhi::FenceHandle fence) override;
	
	const std::vector<TextureCreateEvent>& GetTextureCreates() const noexcept;
	const std::vector<FrameBufferCreateEvent>& GetFrameBufferCreates() const noexcept;
	const std::vector<rhi::TextureHandle> & GetDestroyedTextures() const noexcept;
	const std::vector<rhi::FrameBufferHandle> GetDestroyedFrameBuffers() const noexcept;
	const std::vector<rhi::CommandList>& GetSubmittedCommandLists() const noexcept;
	
private:
	template <typename HandleType>
	HandleType NextHandle()
	{
		HandleType handle{};
		handle.id = ++nextObjectId_;
		return handle;
	}
	
	rhi::FrameBufferHandle RecordFrameBufferEvent(
		std::string kind,
		std::span<const rhi::TextureHandle> colors,
		rhi::TextureHandle depth);
	
	std::uint32_t nextObjectId_{1};
	rhi::TextureDescIndex nextDescriptorId_{1};
	std::uint32_t nextFenceId_{1};
	
	std::vector<TextureCreateEvent> textureCreates_{};
	std::vector<FrameBufferCreateEvent> frameBufferCreates_{};
	std::vector<rhi::TextureHandle> destroyedTextures_{};
	std::vector<rhi::FrameBufferHandle> destroyedFrameBuffers_{};
	std::vector<rhi::CommandList> submittedCommandLists_{};
	
	std::vector<rhi::BufferDesc> bufferCreates_{};
	std::unordered_map<std::uint32_t, std::vector<std::byte>> bufferData_{};
	std::unordered_map<rhi::TextureDescIndex, rhi::TextureHandle> descriptors_{};
	std::unordered_map<std::uint32_t, bool> fences_{};
};


