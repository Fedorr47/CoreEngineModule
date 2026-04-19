#include "FakeRHI.h"

#include <algorithm>
#include <array>

//---------------------- Fake Swap chain  ----------------------//
FakeRHISwapChain::FakeRHISwapChain(
	rhi::SwapChainDesc swapChainDesc, 
	rhi::FrameBufferHandle backBuffer,
	rhi::TextureHandle depthTexture)
		: swapChainDesc_(swapChainDesc)
		, backBuffer_(backBuffer)
		, depthTexture_(depthTexture)
{
}

rhi::SwapChainDesc FakeRHISwapChain::GetDesc() const
{
	return swapChainDesc_;
}

rhi::FrameBufferHandle FakeRHISwapChain::GetCurrentBackBuffer() const
{
	return backBuffer_;
}

rhi::TextureHandle FakeRHISwapChain::GetDepthTexture() const
{
	return depthTexture_;
}

void FakeRHISwapChain::Present()
{
	++presentCount_;
}

void FakeRHISwapChain::Resize(rhi::Extent2D newExtent)
{
	swapChainDesc_.extent = newExtent;
	++resizeCount_;
	resizeHistory_.push_back(newExtent);
}

std::uint32_t FakeRHISwapChain::GetPresentCount() const noexcept
{
	return presentCount_;
}

std::uint32_t FakeRHISwapChain::GetResizeCount() const noexcept
{
	return resizeCount_;
}

const std::vector<rhi::Extent2D>& FakeRHISwapChain::GetResizeHistory() const noexcept
{
	return resizeHistory_;
}

//---------------------- Fake Device ----------------------//
rhi::Backend FakeRHIDevice::GetBackend() const noexcept
{
	return rhi::Backend::Null;
}

std::string_view FakeRHIDevice::GetName() const
{
	return "Fake RHI Device (test)";
}

rhi::TextureHandle FakeRHIDevice::CreateTexture2D(rhi::Extent2D extent, rhi::Format format)
{
	const auto handle = NextHandle<rhi::TextureHandle>();
	textureCreates_.push_back(TextureCreateEvent{
		.handle = handle, .extent = extent, .format = format, .isCubeMap = false});
	return handle;
}

rhi::TextureHandle FakeRHIDevice::CreateTextureCube(rhi::Extent2D extent, rhi::Format format)
{
	const auto handle = NextHandle<rhi::TextureHandle>();
	textureCreates_.push_back(TextureCreateEvent{
		.handle = handle, .extent = extent, .format = format, .isCubeMap = true});
	return handle;
}

void FakeRHIDevice::DestroyTexture(rhi::TextureHandle texture) noexcept
{
	if (texture)
	{
		destroyedTextures_.push_back(texture);
	}
}

rhi::FrameBufferHandle FakeRHIDevice::CreateFramebuffer(rhi::TextureHandle color, rhi::TextureHandle depth)
{
	const std::array<rhi::TextureHandle, 1> colorTextures{color};
	return RecordFrameBufferEvent("single", colorTextures, depth);
}

rhi::FrameBufferHandle FakeRHIDevice::CreateFramebufferMRT(std::span<const rhi::Handle<rhi::TextureTag>> colors,
	rhi::TextureHandle depth)
{
	return RecordFrameBufferEvent("mrt", colors, depth);
}

rhi::FrameBufferHandle FakeRHIDevice::CreateFramebufferCubeFace(rhi::TextureHandle colorCube, std::uint32_t faceIndex,
	rhi::TextureHandle depth)
{
	const std::array<rhi::TextureHandle, 1> colorTextures{colorCube};
	return RecordFrameBufferEvent("cube_face_" + std::to_string(faceIndex), colorTextures, depth);
}

rhi::FrameBufferHandle FakeRHIDevice::CreateFramebufferCubeFaceMip(rhi::TextureHandle colorCube,
	std::uint32_t faceIndex, std::uint32_t mipLevel, rhi::TextureHandle depth)
{
	const std::array<rhi::TextureHandle, 1> colorTextures{colorCube};
	return RecordFrameBufferEvent("cube_face_mip_" + std::to_string(faceIndex), colorTextures, depth);
}

rhi::FrameBufferHandle FakeRHIDevice::CreateFramebufferCube(rhi::TextureHandle colorCube, rhi::TextureHandle depthCube)
{
	const std::array<rhi::TextureHandle, 1> colorTextures{colorCube};
	return RecordFrameBufferEvent("cube", colorTextures, depthCube);
}

void FakeRHIDevice::DestroyFramebuffer(rhi::FrameBufferHandle framebuffer) noexcept
{
	if (framebuffer)
	{
		destroyedFrameBuffers_.push_back(framebuffer);
	}
}

rhi::BufferHandle FakeRHIDevice::CreateBuffer(const rhi::BufferDesc& desc)
{
	const auto handle = NextHandle<rhi::BufferHandle>();
	bufferCreates_.push_back(desc);
	bufferData_[handle.id] = {};
	return handle;
}

void FakeRHIDevice::UpdateBuffer(rhi::BufferHandle buffer, std::span<const std::byte> data, std::size_t offsetBytes)
{
	auto& storage = bufferData_[buffer.id];
	if (storage.size() < offsetBytes + data.size())
	{
		storage.resize(offsetBytes + data.size());
	}
	std::copy(data.begin(), data.end(), storage.begin() + static_cast<std::ptrdiff_t>(offsetBytes));
}

void FakeRHIDevice::DestroyBuffer(rhi::BufferHandle buffer) noexcept
{
	bufferData_.erase(buffer.id);
}

rhi::InputLayoutHandle FakeRHIDevice::CreateInputLayout(const rhi::InputLayoutDesc& desc)
{
	return NextHandle<rhi::InputLayoutHandle>();
}

void FakeRHIDevice::DestroyInputLayout(rhi::InputLayoutHandle inputLayout) noexcept
{
}

rhi::ShaderHandle FakeRHIDevice::CreateShader(rhi::ShaderStage stage, std::string_view debugName,
	std::string_view sourceOrBytecode)
{
	return NextHandle<rhi::ShaderHandle>();
}

void FakeRHIDevice::DestroyShader(rhi::ShaderHandle shader) noexcept
{
}

rhi::PipelineHandle FakeRHIDevice::CreatePipeline(std::string_view debugName, rhi::ShaderHandle vertexShader,
	rhi::ShaderHandle pixelShader, rhi::PrimitiveTopologyType topologyType)
{
	return NextHandle<rhi::PipelineHandle>();
}

void FakeRHIDevice::DestroyPipeline(rhi::PipelineHandle pipeline) noexcept
{
}

void FakeRHIDevice::SubmitCommandList(rhi::CommandList&& commandList)
{
	submittedCommandLists_.push_back(std::move(commandList));
}

rhi::TextureDescIndex FakeRHIDevice::AllocateTextureDesctiptor(rhi::TextureHandle texture)
{
	const auto index = nextDescriptorId_++;
	descriptors_[index] = texture;
	return index;
}

void FakeRHIDevice::UpdateTextureDescriptor(rhi::TextureDescIndex index, rhi::TextureHandle texture)
{
	descriptors_[index] = texture;
}

void FakeRHIDevice::FreeTextureDescriptor(rhi::TextureDescIndex index) noexcept
{
	descriptors_.erase(index);
}

rhi::FenceHandle FakeRHIDevice::CreateFence(bool signaled)
{
	rhi::FenceHandle fenceHandle;
	fenceHandle.id = nextFenceId_++;
	fences_[fenceHandle.id] = signaled;
	return fenceHandle;
}

void FakeRHIDevice::DestroyFence(rhi::FenceHandle fence) noexcept
{
	fences_.erase(fence.id);
}

void FakeRHIDevice::SignalFence(rhi::FenceHandle fence)
{
	fences_[fence.id] = true;
}

void FakeRHIDevice::WaitFence(rhi::FenceHandle fence)
{
	fences_[fence.id] = true;
}

bool FakeRHIDevice::IsFenceSignaled(rhi::FenceHandle fence)
{
	if (const auto it = fences_.find(fence.id); it != fences_.end())
	{
		return it->second;
	}
	return true;
}

const std::vector<TextureCreateEvent>& FakeRHIDevice::GetTextureCreates() const noexcept
{
	return textureCreates_;
}

const std::vector<FrameBufferCreateEvent>& FakeRHIDevice::GetFrameBufferCreates() const noexcept
{
	return frameBufferCreates_;
}

const std::vector<rhi::TextureHandle>& FakeRHIDevice::GetDestroyedTextures() const noexcept
{
	return destroyedTextures_;
}

const std::vector<rhi::FrameBufferHandle> FakeRHIDevice::GetDestroyedFrameBuffers() const noexcept
{
	return destroyedFrameBuffers_;
}

const std::vector<rhi::CommandList>& FakeRHIDevice::GetSubmittedCommandLists() const noexcept
{
	return submittedCommandLists_;
}

rhi::FrameBufferHandle FakeRHIDevice::RecordFrameBufferEvent(std::string kind,
	std::span<const rhi::TextureHandle> colors, rhi::TextureHandle depth)
{
	const auto handle = NextHandle<rhi::FrameBufferHandle>();
	FrameBufferCreateEvent event;
	event.handle = handle;
	event.colors.assign(colors.begin(), colors.end());
	event.depth = depth;
	event.kind = std::move(kind);
	frameBufferCreates_.push_back(std::move(event));
	return handle;
}
