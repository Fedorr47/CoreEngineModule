module;

#if defined(_WIN32)
// Prevent Windows headers from defining the `min`/`max` macros which break `std::min`/`std::max`.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <stdexcept>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>

#include "d3dx12.h"
#endif

export module core:render_core_dx12;

import :resource_manager_core;
import :rhi;
import :rhi_dx12;

#if defined(_WIN32)
using Microsoft::WRL::ComPtr;
#endif

DXGI_FORMAT DxgiRGBA8(bool srgb)
{
	return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
}

export namespace rendern
{
#if defined(_WIN32)

	export class DX12TextureUploader final : public ITextureUploader
	{
	public:
		explicit DX12TextureUploader(rhi::IRHIDevice& device)
			: device_(device)
		{
		}

		std::optional<GPUTexture> CreateAndUpload(const TextureCPUData& cpuData, const TextureProperties& properties) override
		{
			// Need DX12 device
			auto* dxDev = dynamic_cast<rhi::DX12Device*>(&device_);
			if (!dxDev)
			{
				return std::nullopt;
			}

			ID3D12Device* d3d = dxDev->NativeDevice();
			ID3D12CommandQueue* queue = dxDev->NativeQueue();
			if (!d3d || !queue)
			{
				return std::nullopt;
			}

			// ---------------------- Cubemap ----------------------
			if (properties.dimension == TextureDimension::Cube)
			{
				const std::uint32_t width = cpuData.width ? cpuData.width : properties.width;
				const std::uint32_t height = cpuData.height ? cpuData.height : properties.height;

				if (width == 0 || height == 0)
				{
					return std::nullopt;
				}

				// Validate mip chains (expect RGBA8)
				if (cpuData.format != TextureFormat::RGBA || cpuData.channels != 4)
				{
					return std::nullopt;
				}

				const auto& face0 = cpuData.cubeMips[0];
				if (face0.empty())
				{
					return std::nullopt;
				}

				const UINT mipLevels = static_cast<UINT>(face0.size());
				for (int face = 0; face < 6; ++face)
				{
					const auto& fm = cpuData.cubeMips[static_cast<std::size_t>(face)];
					if (fm.size() != mipLevels)
					{
						return std::nullopt;
					}
					if (fm.empty() || fm[0].width != width || fm[0].height != height)
					{
						return std::nullopt;
					}
					for (UINT mip = 0; mip < mipLevels; ++mip)
					{
						const auto& ml = fm[mip];
						if (ml.width == 0 || ml.height == 0)
						{
							return std::nullopt;
						}
						const std::size_t expectedSize = static_cast<std::size_t>(ml.width) * static_cast<std::size_t>(ml.height) * 4u;
						if (ml.pixels.empty() || ml.pixels.size() != expectedSize)
						{
							return std::nullopt;
						}
					}
				}

				const DXGI_FORMAT fmt = DxgiRGBA8(properties.srgb);

				// Create default texture cube resource (Texture2DArray[6] with TEXTURECUBE SRV)
				D3D12_RESOURCE_DESC texDesc{};
				texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texDesc.Alignment = 0;
				texDesc.Width = width;
				texDesc.Height = height;
				texDesc.DepthOrArraySize = 6; // 6 faces
				texDesc.MipLevels = static_cast<UINT16>(mipLevels);
				texDesc.Format = fmt;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

				ComPtr<ID3D12Resource> texture;
				{
					D3D12_HEAP_PROPERTIES heapProps{};
					heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

					ThrowIfFailed(
						d3d->CreateCommittedResource(
							&heapProps,
							D3D12_HEAP_FLAG_NONE,
							&texDesc,
							D3D12_RESOURCE_STATE_COPY_DEST,
							nullptr,
							IID_PPV_ARGS(&texture)),
						"DX12TextureUploader: CreateCommittedResource(textureCube) failed");
				}

				// Create upload buffer
				const UINT numSubresources = mipLevels * 6u;
				UINT64 uploadBytes = 0;
				d3d->GetCopyableFootprints(&texDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &uploadBytes);

				ComPtr<ID3D12Resource> upload;
				{
					D3D12_HEAP_PROPERTIES uploadProps{};
					uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

					auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);

					ThrowIfFailed(
						d3d->CreateCommittedResource(
							&uploadProps,
							D3D12_HEAP_FLAG_NONE,
							&upDesc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							nullptr,
							IID_PPV_ARGS(&upload)),
						"DX12TextureUploader: CreateCommittedResource(upload) failed");
				}

				// Prepare subresources in (arraySlice-major) order:
				// subresource = mip + slice * mipLevels (plane 0)
				std::vector<D3D12_SUBRESOURCE_DATA> subs;
				subs.reserve(numSubresources);

				for (UINT slice = 0; slice < 6u; ++slice)
				{
					const auto& mips = cpuData.cubeMips[static_cast<std::size_t>(slice)];
					for (UINT mip = 0; mip < mipLevels; ++mip)
					{
						const auto& mipInst = mips[mip];
						D3D12_SUBRESOURCE_DATA subResData{};
						subResData.pData = mipInst.pixels.data();
						subResData.RowPitch = static_cast<LONG_PTR>(mipInst.width) * 4;
						subResData.SlicePitch = subResData.RowPitch * static_cast<LONG_PTR>(mipInst.height);
						subs.push_back(subResData);
					}
				}

				// Record copy commands into a temporary list
				ComPtr<ID3D12CommandAllocator> alloc;
				ThrowIfFailed(
					d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)),
					"DX12TextureUploader: CreateCommandAllocator failed");

				ComPtr<ID3D12GraphicsCommandList> list;
				ThrowIfFailed(
					d3d->CreateCommandList(
						0,
						D3D12_COMMAND_LIST_TYPE_DIRECT,
						alloc.Get(),
						nullptr,
						IID_PPV_ARGS(&list)),
					"DX12TextureUploader: CreateCommandList failed");

				UpdateSubresources(list.Get(), texture.Get(), upload.Get(), 0, 0, numSubresources, subs.data());

				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					texture.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				list->ResourceBarrier(1, &barrier);

				ThrowIfFailed(list->Close(), "DX12TextureUploader: Close cmdlist failed");

				ID3D12CommandList* lists[] = { list.Get() };
				queue->ExecuteCommandLists(1, lists);

				// Fence wait (so we can free upload immediately)
				ComPtr<ID3D12Fence> fence;
				ThrowIfFailed(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
					"DX12TextureUploader: CreateFence failed");

				HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				if (!eventHandle)
				{
					return std::nullopt;
				}

				const UINT64 fv = 1;
				ThrowIfFailed(queue->Signal(fence.Get(), fv), "DX12TextureUploader: Signal failed");
				if (fence->GetCompletedValue() < fv)
				{
					ThrowIfFailed(fence->SetEventOnCompletion(fv, eventHandle), "DX12TextureUploader: SetEventOnCompletion failed");
					WaitForSingleObject(eventHandle, INFINITE);
				}
				CloseHandle(eventHandle);

				// Register in DX12 RHI and allocate a TEXTURECUBE SRV.
				const rhi::TextureHandle handle = dxDev->RegisterSampledTextureCube(texture.Get(), fmt, mipLevels);
				if (!handle)
				{
					return std::nullopt;
				}

				return GPUTexture{ static_cast<unsigned int>(handle.id) };
			}

			// ---------------------- Tex2D ----------------------
			// Validate
					if (cpuData.format != TextureFormat::RGBA || cpuData.channels != 4)
					{
						return std::nullopt;
					}

					if (cpuData.mips.empty())
					{
						return std::nullopt;
					}

					const auto& mips = cpuData.mips;
					const std::uint32_t width = mips[0].width ? mips[0].width : (cpuData.width ? cpuData.width : properties.width);
					const std::uint32_t height = mips[0].height ? mips[0].height : (cpuData.height ? cpuData.height : properties.height);

					if (width == 0 || height == 0 || mips[0].width != width || mips[0].height != height)
					{
						return std::nullopt;
					}

					const UINT mipLevels = static_cast<UINT>(mips.size());
					for (UINT mip = 0; mip < mipLevels; ++mip)
					{
						const auto& ml = mips[mip];
						if (ml.width == 0 || ml.height == 0)
						{
							return std::nullopt;
						}
						const std::size_t expectedSize = static_cast<std::size_t>(ml.width) * static_cast<std::size_t>(ml.height) * 4u;
						if (ml.pixels.empty() || ml.pixels.size() != expectedSize)
						{
							return std::nullopt;
						}
					}

			const DXGI_FORMAT fmt = DxgiRGBA8(properties.srgb);

			// Create default texture with mip levels
			D3D12_RESOURCE_DESC texDesc{};
			texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texDesc.Alignment = 0;
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.DepthOrArraySize = 1;
			texDesc.MipLevels = static_cast<UINT16>(mipLevels);
			texDesc.Format = fmt;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ComPtr<ID3D12Resource> texture;
			{
				D3D12_HEAP_PROPERTIES heapProps{};
				heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

				ThrowIfFailed(
					d3d->CreateCommittedResource(
						&heapProps,
						D3D12_HEAP_FLAG_NONE,
						&texDesc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						IID_PPV_ARGS(&texture)),
					"DX12TextureUploader: CreateCommittedResource(texture) failed");
			}

			// Create upload buffer
			UINT64 uploadBytes = 0;
			d3d->GetCopyableFootprints(&texDesc, 0, mipLevels, 0, nullptr, nullptr, nullptr, &uploadBytes);

			ComPtr<ID3D12Resource> upload;
			{
				D3D12_HEAP_PROPERTIES uploadProps{};
				uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

				auto upDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBytes);

				ThrowIfFailed(
					d3d->CreateCommittedResource(
						&uploadProps,
						D3D12_HEAP_FLAG_NONE,
						&upDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&upload)),
					"DX12TextureUploader: CreateCommittedResource(upload) failed");
			}

			// Prepare subresources
			std::vector<D3D12_SUBRESOURCE_DATA> subs;
			subs.reserve(mipLevels);
			for (UINT i = 0; i < mipLevels; ++i)
			{
				const auto& mipInst = mips[i];
				D3D12_SUBRESOURCE_DATA subResData{};
				subResData.pData = mipInst.pixels.data();
				subResData.RowPitch = static_cast<LONG_PTR>(mipInst.width) * 4;
				subResData.SlicePitch = subResData.RowPitch * static_cast<LONG_PTR>(mipInst.height);
				subs.push_back(subResData);
			}

			// Record copy commands into a temporary list
			ComPtr<ID3D12CommandAllocator> alloc;
			ThrowIfFailed(
				d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)),
				"DX12TextureUploader: CreateCommandAllocator failed");

			ComPtr<ID3D12GraphicsCommandList> list;
			ThrowIfFailed(
				d3d->CreateCommandList(
					0,
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					alloc.Get(),
					nullptr,
					IID_PPV_ARGS(&list)),
				"DX12TextureUploader: CreateCommandList failed");

			UpdateSubresources(list.Get(), texture.Get(), upload.Get(), 0, 0, mipLevels, subs.data());

			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				texture.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			list->ResourceBarrier(1, &barrier);

			ThrowIfFailed(list->Close(), "DX12TextureUploader: Close cmdlist failed");

			ID3D12CommandList* lists[] = { list.Get() };
			queue->ExecuteCommandLists(1, lists);

			// Fence wait
			ComPtr<ID3D12Fence> fence;
			ThrowIfFailed(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
				"DX12TextureUploader: CreateFence failed");

			HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (!eventHandle)
			{
				return std::nullopt;
			}

			const UINT64 fv = 1;
			ThrowIfFailed(queue->Signal(fence.Get(), fv), "DX12TextureUploader: Signal failed");
			if (fence->GetCompletedValue() < fv)
			{
				ThrowIfFailed(fence->SetEventOnCompletion(fv, eventHandle), "DX12TextureUploader: SetEventOnCompletion failed");
				WaitForSingleObject(eventHandle, INFINITE);
			}
			CloseHandle(eventHandle);

			// Register inside DX12 RHI as a TextureHandle and create SRV in device's heap
			const rhi::TextureHandle handle = dxDev->RegisterSampledTexture(texture.Get(), fmt, mipLevels);
			if (!handle)
			{
				return std::nullopt;
			}

			return GPUTexture{ static_cast<unsigned int>(handle.id) };
		}

		void Destroy(GPUTexture texture) noexcept override
		{
			if (texture.id == 0)
			{
				return;
			}

			rhi::TextureHandle textureHandle{};
			textureHandle.id = static_cast<std::uint32_t>(texture.id);
			device_.DestroyTexture(textureHandle);
		}

	private:
		rhi::IRHIDevice& device_;
	};

#else // !_WIN32

	export class DX12TextureUploader final : public ITextureUploader
	{
	public:
		explicit DX12TextureUploader(rhi::IRHIDevice&) {}
		std::optional<GPUTexture> CreateAndUpload(const TextureCPUData&, const TextureProperties&) override { return std::nullopt; }
		void Destroy(GPUTexture) noexcept override {}
	};

#endif
} // namespace rendern