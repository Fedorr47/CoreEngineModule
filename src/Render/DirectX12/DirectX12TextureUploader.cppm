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

		~DX12TextureUploader() override
		{
			AbortUploadBatch_();
			if (batchEvent_)
			{
				CloseHandle(batchEvent_);
				batchEvent_ = nullptr;
			}
		}

		void BeginUploadBatch() override
		{
			auto* dxDev = dynamic_cast<rhi::DX12Device*>(&device_);
			if (!dxDev)
			{
				return;
			}

			ID3D12Device* d3d = dxDev->NativeDevice();
			if (!d3d)
			{
				return;
			}

			++batchDepth_;
			if (batchDepth_ > 1u)
			{
				return;
			}

			try
			{
				EnsureBatchObjects_(d3d);
				ThrowIfFailed(batchAllocator_->Reset(), "DX12TextureUploader: batch allocator reset failed");
				ThrowIfFailed(batchList_->Reset(batchAllocator_.Get(), nullptr), "DX12TextureUploader: batch cmdlist reset failed");
				batchPendingUploads_.clear();
				batchHasCommands_ = false;
			}
			catch (...)
			{
				batchDepth_ = 0u;
				batchPendingUploads_.clear();
				batchHasCommands_ = false;
				throw;
			}
		}

		void EndUploadBatch() override
		{
			auto* dxDev = dynamic_cast<rhi::DX12Device*>(&device_);
			if (!dxDev)
			{
				return;
			}

			if (batchDepth_ == 0u)
			{
				return;
			}

			--batchDepth_;
			if (batchDepth_ != 0u)
			{
				return;
			}

			ID3D12CommandQueue* queue = dxDev->NativeQueue();
			if (!queue)
			{
				AbortUploadBatch_();
				return;
			}
			SubmitAndWaitBatch_(queue);
		}

		std::optional<GPUTexture> CreateAndUpload(const TextureCPUData& cpuData, const TextureProperties& properties) override
		{
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

			const bool ownsBatch = (batchDepth_ == 0u);
			if (ownsBatch)
			{
				BeginUploadBatch();
			}

			std::optional<GPUTexture> uploadedTexture{};
			try
			{
				uploadedTexture = CreateAndRecordUpload_(*dxDev, d3d, cpuData, properties);
				if (ownsBatch)
				{
					try
					{
						EndUploadBatch();
					}
					catch (...)
					{
						if (uploadedTexture && uploadedTexture->id != 0)
						{
							Destroy(*uploadedTexture);
							uploadedTexture.reset();
						}
						throw;
					}
				}
				return uploadedTexture;
			}
			catch (...)
			{
				if (ownsBatch)
				{
					AbortUploadBatch_();
				}
				throw;
			}
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
		std::optional<GPUTexture> CreateAndRecordUpload_(
			rhi::DX12Device& dxDev,
			ID3D12Device* d3d,
			const TextureCPUData& cpuData,
			const TextureProperties& properties)
		{
			if (properties.dimension == TextureDimension::Cube)
			{
				return RecordCubeUpload_(dxDev, d3d, cpuData, properties);
			}
			return RecordTexture2DUpload_(dxDev, d3d, cpuData, properties);
		}

		void EnsureBatchObjects_(ID3D12Device* d3d)
		{
			if (!batchFence_)
			{
				ThrowIfFailed(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&batchFence_)),
					"DX12TextureUploader: CreateFence failed");
			}

			if (!batchEvent_)
			{
				batchEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				if (!batchEvent_)
				{
					throw std::runtime_error("DX12TextureUploader: CreateEvent failed");
				}
			}

			if (!batchAllocator_)
			{
				ThrowIfFailed(
					d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&batchAllocator_)),
					"DX12TextureUploader: CreateCommandAllocator failed");
			}

			if (!batchList_)
			{
				ThrowIfFailed(
					d3d->CreateCommandList(
						0,
						D3D12_COMMAND_LIST_TYPE_DIRECT,
						batchAllocator_.Get(),
						nullptr,
						IID_PPV_ARGS(&batchList_)),
					"DX12TextureUploader: CreateCommandList failed");
				ThrowIfFailed(batchList_->Close(), "DX12TextureUploader: Close initial batch cmdlist failed");
			}
		}

		void SubmitAndWaitBatch_(ID3D12CommandQueue* queue)
		{
			if (!batchList_)
			{
				batchPendingUploads_.clear();
				batchHasCommands_ = false;
				return;
			}

			try
			{
				ThrowIfFailed(batchList_->Close(), "DX12TextureUploader: Close batch cmdlist failed");
				if (batchHasCommands_)
				{
					ID3D12CommandList* lists[] = { batchList_.Get() };
					queue->ExecuteCommandLists(1, lists);

					const UINT64 fenceValue = ++batchFenceValue_;
					ThrowIfFailed(queue->Signal(batchFence_.Get(), fenceValue), "DX12TextureUploader: Signal failed");
					if (batchFence_->GetCompletedValue() < fenceValue)
					{
						ThrowIfFailed(batchFence_->SetEventOnCompletion(fenceValue, batchEvent_),
							"DX12TextureUploader: SetEventOnCompletion failed");
						WaitForSingleObject(batchEvent_, INFINITE);
					}
				}

				batchPendingUploads_.clear();
				batchHasCommands_ = false;
			}
			catch (...)
			{
				AbortUploadBatch_();
				throw;
			}
		}

		void AbortUploadBatch_() noexcept
		{
			batchDepth_ = 0u;
			batchPendingUploads_.clear();
			batchHasCommands_ = false;
			batchList_.Reset();
			batchAllocator_.Reset();
			batchFence_.Reset();
		}

		std::optional<GPUTexture> RecordCubeUpload_(
			rhi::DX12Device& dxDev,
			ID3D12Device* d3d,
			const TextureCPUData& cpuData,
			const TextureProperties& properties)
		{
			ID3D12GraphicsCommandList* list = batchList_.Get();
			if (!list)
			{
				return std::nullopt;
			}

			const std::uint32_t width = cpuData.width ? cpuData.width : properties.width;
			const std::uint32_t height = cpuData.height ? cpuData.height : properties.height;

			if (width == 0 || height == 0)
			{
				return std::nullopt;
			}

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
			ComPtr<ID3D12Resource> texture;
			ComPtr<ID3D12Resource> upload;
			rhi::TextureHandle registeredHandle{};

			try
			{
				D3D12_RESOURCE_DESC texDesc{};
				texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texDesc.Alignment = 0;
				texDesc.Width = width;
				texDesc.Height = height;
				texDesc.DepthOrArraySize = 6;
				texDesc.MipLevels = static_cast<UINT16>(mipLevels);
				texDesc.Format = fmt;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

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

				registeredHandle = dxDev.RegisterSampledTextureCube(texture.Get(), fmt, mipLevels);
				if (!registeredHandle)
				{
					return std::nullopt;
				}

				const UINT numSubresources = mipLevels * 6u;
				UINT64 uploadBytes = 0;
				d3d->GetCopyableFootprints(&texDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &uploadBytes);

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
						"DX12TextureUploader: CreateCommittedResource(uploadCube) failed");
				}

				std::vector<D3D12_SUBRESOURCE_DATA> subs{};
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

				UpdateSubresources(list, texture.Get(), upload.Get(), 0, 0, numSubresources, subs.data());
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					texture.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				list->ResourceBarrier(1, &barrier);

				batchPendingUploads_.push_back(upload);
				batchHasCommands_ = true;
				return GPUTexture{ static_cast<unsigned int>(registeredHandle.id) };
			}
			catch (...)
			{
				if (registeredHandle)
				{
					dxDev.DestroyTexture(registeredHandle);
				}
				throw;
			}
		}

		std::optional<GPUTexture> RecordTexture2DUpload_(
			rhi::DX12Device& dxDev,
			ID3D12Device* d3d,
			const TextureCPUData& cpuData,
			const TextureProperties& properties)
		{
			ID3D12GraphicsCommandList* list = batchList_.Get();
			if (!list)
			{
				return std::nullopt;
			}

			if (cpuData.format != TextureFormat::RGBA || cpuData.channels != 4)
			{
				return std::nullopt;
			}

			if (cpuData.mips.empty())
			{
				return std::nullopt;
			}

			const auto& mips = cpuData.mips;
			const std::uint32_t baseWidth =
				(mips[0].width != 0u) ? mips[0].width : ((cpuData.width != 0u) ? cpuData.width : properties.width);
			const std::uint32_t baseHeight =
				(mips[0].height != 0u) ? mips[0].height : ((cpuData.height != 0u) ? cpuData.height : properties.height);

			if (baseWidth == 0 || baseHeight == 0)
			{
				return std::nullopt;
			}

			const UINT mipLevels = static_cast<UINT>(mips.size());
			for (UINT mip = 0; mip < mipLevels; ++mip)
			{
				const auto& ml = mips[mip];
				const std::uint32_t mw = (ml.width != 0u) ? ml.width : std::max(1u, baseWidth >> mip);
				const std::uint32_t mh = (ml.height != 0u) ? ml.height : std::max(1u, baseHeight >> mip);
				if (mw == 0u || mh == 0u)
				{
					return std::nullopt;
				}
				const std::size_t expectedSize = static_cast<std::size_t>(mw) * static_cast<std::size_t>(mh) * 4u;
				if (ml.pixels.empty() || ml.pixels.size() != expectedSize)
				{
					return std::nullopt;
				}
			}

			const DXGI_FORMAT fmt = DxgiRGBA8(properties.srgb);
			ComPtr<ID3D12Resource> texture;
			ComPtr<ID3D12Resource> upload;
			rhi::TextureHandle registeredHandle{};

			try
			{
				D3D12_RESOURCE_DESC texDesc{};
				texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				texDesc.Alignment = 0;
				texDesc.Width = baseWidth;
				texDesc.Height = baseHeight;
				texDesc.DepthOrArraySize = 1;
				texDesc.MipLevels = static_cast<UINT16>(mipLevels);
				texDesc.Format = fmt;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

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
						"DX12TextureUploader: CreateCommittedResource(texture2D) failed");
				}

				registeredHandle = dxDev.RegisterSampledTexture(texture.Get(), fmt, mipLevels);
				if (!registeredHandle)
				{
					return std::nullopt;
				}

				UINT64 uploadBytes = 0;
				d3d->GetCopyableFootprints(&texDesc, 0, mipLevels, 0, nullptr, nullptr, nullptr, &uploadBytes);

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
						"DX12TextureUploader: CreateCommittedResource(upload2D) failed");
				}

				std::vector<D3D12_SUBRESOURCE_DATA> subs{};
				subs.reserve(mipLevels);
				for (UINT mip = 0; mip < mipLevels; ++mip)
				{
					const auto& mipInst = mips[mip];
					const std::uint32_t mw = (mipInst.width != 0u) ? mipInst.width : std::max(1u, baseWidth >> mip);
					const std::uint32_t mh = (mipInst.height != 0u) ? mipInst.height : std::max(1u, baseHeight >> mip);
					D3D12_SUBRESOURCE_DATA subResData{};
					subResData.pData = mipInst.pixels.data();
					subResData.RowPitch = static_cast<LONG_PTR>(mw) * 4;
					subResData.SlicePitch = subResData.RowPitch * static_cast<LONG_PTR>(mh);
					subs.push_back(subResData);
				}

				UpdateSubresources(list, texture.Get(), upload.Get(), 0, 0, mipLevels, subs.data());
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					texture.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				list->ResourceBarrier(1, &barrier);

				batchPendingUploads_.push_back(upload);
				batchHasCommands_ = true;
				return GPUTexture{ static_cast<unsigned int>(registeredHandle.id) };
			}
			catch (...)
			{
				if (registeredHandle)
				{
					dxDev.DestroyTexture(registeredHandle);
				}
				throw;
			}
		}

		rhi::IRHIDevice& device_;
		ComPtr<ID3D12CommandAllocator> batchAllocator_{};
		ComPtr<ID3D12GraphicsCommandList> batchList_{};
		ComPtr<ID3D12Fence> batchFence_{};
		HANDLE batchEvent_{ nullptr };
		UINT64 batchFenceValue_{ 0 };
		std::vector<ComPtr<ID3D12Resource>> batchPendingUploads_{};
		std::uint32_t batchDepth_{ 0 };
		bool batchHasCommands_{ false };
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