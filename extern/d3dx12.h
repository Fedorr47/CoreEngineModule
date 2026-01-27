#pragma once

// Minimal subset of d3dx12.h utilities used by this project.
// Covers: D3D12_DEFAULT + CD3D12_*_DESC + CD3DX12_RESOURCE_BARRIER::Transition + UpdateSubresources.

#include <d3d12.h>
#include <dxgi.h>
#include <cstdint>
#include <cstring>
#include <algorithm>

// -------------------------------------------
// Default tag
// -------------------------------------------
struct CD3DX12_DEFAULT {};
inline constexpr CD3DX12_DEFAULT D3D12_DEFAULT{};

// -------------------------------------------
// HEAP_PROPERTIES helper
// -------------------------------------------
struct CD3DX12_HEAP_PROPERTIES : public D3D12_HEAP_PROPERTIES
{
    explicit CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE type) noexcept
    {
        Type = type;
        CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        CreationNodeMask = 1;
        VisibleNodeMask = 1;
    }
};

// -------------------------------------------
// RESOURCE_DESC helper
// -------------------------------------------
struct CD3DX12_RESOURCE_DESC : public D3D12_RESOURCE_DESC
{
    static CD3DX12_RESOURCE_DESC Buffer(UINT64 bytes) noexcept
    {
        CD3DX12_RESOURCE_DESC d{};
        d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Alignment = 0;
        d.Width = bytes;
        d.Height = 1;
        d.DepthOrArraySize = 1;
        d.MipLevels = 1;
        d.Format = DXGI_FORMAT_UNKNOWN;
        d.SampleDesc.Count = 1;
        d.SampleDesc.Quality = 0;
        d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        d.Flags = D3D12_RESOURCE_FLAG_NONE;
        return d;
    }
};

// -------------------------------------------
// BLEND_DESC helper
// -------------------------------------------
struct CD3DX12_BLEND_DESC : public D3D12_BLEND_DESC
{
    CD3DX12_BLEND_DESC() noexcept = default;

    explicit CD3DX12_BLEND_DESC(CD3DX12_DEFAULT) noexcept
    {
        AlphaToCoverageEnable = FALSE;
        IndependentBlendEnable = FALSE;

        D3D12_RENDER_TARGET_BLEND_DESC rt{};
        rt.BlendEnable = FALSE;
        rt.LogicOpEnable = FALSE;
        rt.SrcBlend = D3D12_BLEND_ONE;
        rt.DestBlend = D3D12_BLEND_ZERO;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_ZERO;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rt.LogicOp = D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        for (auto& r : RenderTarget)
            r = rt;
    }
};

// -------------------------------------------
// RASTERIZER_DESC helper
// -------------------------------------------
struct CD3DX12_RASTERIZER_DESC : public D3D12_RASTERIZER_DESC
{
    CD3DX12_RASTERIZER_DESC() noexcept = default;

    explicit CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT) noexcept
    {
        FillMode = D3D12_FILL_MODE_SOLID;
        CullMode = D3D12_CULL_MODE_BACK;
        FrontCounterClockwise = FALSE;
        DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        DepthClipEnable = TRUE;
        MultisampleEnable = FALSE;
        AntialiasedLineEnable = FALSE;
        ForcedSampleCount = 0;
        ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }
};

// -------------------------------------------
// DEPTH_STENCIL_DESC helper
// -------------------------------------------
struct CD3DX12_DEPTH_STENCIL_DESC : public D3D12_DEPTH_STENCIL_DESC
{
    CD3DX12_DEPTH_STENCIL_DESC() noexcept = default;

    explicit CD3DX12_DEPTH_STENCIL_DESC(CD3DX12_DEFAULT) noexcept
    {
        DepthEnable = TRUE;
        DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        StencilEnable = FALSE;
        StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

        FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        BackFace = FrontFace;
    }
};

// -------------------------------------------
// RESOURCE_BARRIER helper
// -------------------------------------------
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
    static CD3DX12_RESOURCE_BARRIER Transition(
        ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
    {
        CD3DX12_RESOURCE_BARRIER result{};
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        result.Flags = flags;

        // !!! important: access the BASE data member, not the function name
        auto& tr = static_cast<D3D12_RESOURCE_BARRIER&>(result).Transition;
        tr.pResource = pResource;
        tr.StateBefore = stateBefore;
        tr.StateAfter = stateAfter;
        tr.Subresource = subresource;

        return result;
    }
};

// -------------------------------------------
// Compatibility aliases (code expects CD3D12_* names)
// -------------------------------------------
using CD3D12_BLEND_DESC = CD3DX12_BLEND_DESC;
using CD3D12_RASTERIZER_DESC = CD3DX12_RASTERIZER_DESC;
using CD3D12_DEPTH_STENCIL_DESC = CD3DX12_DEPTH_STENCIL_DESC;

// -------------------------------------------
// Minimal UpdateSubresources for Texture2D mip chain
// Works for your usage: UpdateSubresources(cmdList, dstTex, upload, 0, 0, mipLevels, subs.data())
// -------------------------------------------
inline UINT64 UpdateSubresources(
    ID3D12GraphicsCommandList* pCmdList,
    ID3D12Resource* pDestinationResource,
    ID3D12Resource* pIntermediate,
    UINT64 IntermediateOffset,
    UINT FirstSubresource,
    UINT NumSubresources,
    const D3D12_SUBRESOURCE_DATA* pSrcData)
{
    if (!pCmdList || !pDestinationResource || !pIntermediate || !pSrcData || NumSubresources == 0)
        return 0;

    // Destination desc (assume texture2D here)
    const D3D12_RESOURCE_DESC dstDesc = pDestinationResource->GetDesc();

    // We need the device to call GetCopyableFootprints
    ID3D12Device* device = nullptr;
    HRESULT hr = pDestinationResource->GetDevice(IID_PPV_ARGS(&device));
    if (FAILED(hr) || !device)
        return 0;

    // Allocate arrays on stack for "small" mip counts; fallback to heap if needed.
    // For safety in demo: always heap.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[NumSubresources]{};
    UINT* numRows = new UINT[NumSubresources]{};
    UINT64* rowSizeInBytes = new UINT64[NumSubresources]{};
    UINT64 requiredSize = 0;

    device->GetCopyableFootprints(&dstDesc, FirstSubresource, NumSubresources, IntermediateOffset,
        layouts, numRows, rowSizeInBytes, &requiredSize);

    // Map upload buffer and write each subresource respecting RowPitch
    std::uint8_t* mapped = nullptr;
    D3D12_RANGE readRange{ 0, 0 };
    hr = pIntermediate->Map(0, &readRange, reinterpret_cast<void**>(&mapped));
    if (FAILED(hr) || !mapped)
    {
        delete[] layouts;
        delete[] numRows;
        delete[] rowSizeInBytes;
        device->Release();
        return 0;
    }

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        const auto& src = pSrcData[i];
        const auto& layout = layouts[i];
        std::uint8_t* dst = mapped + layout.Offset;

        const std::uint8_t* srcBytes = reinterpret_cast<const std::uint8_t*>(src.pData);

        // Copy row by row
        for (UINT row = 0; row < numRows[i]; ++row)
        {
            std::memcpy(
                dst + row * layout.Footprint.RowPitch,
                srcBytes + row * src.RowPitch,
                static_cast<size_t>(rowSizeInBytes[i]));
        }
    }

    pIntermediate->Unmap(0, nullptr);

    // Issue CopyTextureRegion per subresource
    for (UINT i = 0; i < NumSubresources; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = pDestinationResource;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = FirstSubresource + i;

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = pIntermediate;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = layouts[i];

        pCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }

    delete[] layouts;
    delete[] numRows;
    delete[] rowSizeInBytes;
    device->Release();

    return requiredSize;
}
