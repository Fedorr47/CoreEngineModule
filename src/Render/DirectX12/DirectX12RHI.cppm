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
    #include <d3dcompiler.h>
    #include <wrl/client.h>

    #include "d3dx12.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <array>
#include <stdexcept>
#include <algorithm>

export module core:rhi_dx12;

import :rhi;
import :dx12_core;

#if defined(_WIN32)
using Microsoft::WRL::ComPtr;
#endif

inline void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(msg);
    }
}

std::uint32_t AlignUp(std::uint32_t v, std::uint32_t a)
{
    return (v + (a - 1)) & ~(a - 1);
}

DXGI_FORMAT ToDXGIFormat(rhi::Format format)
{
    switch (format)
    {
    case rhi::Format::RGBA8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case rhi::Format::BGRA8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case rhi::Format::D32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;
    case rhi::Format::D24_UNORM_S8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT ToDXGIVertexFormat(rhi::VertexFormat format)
{
    switch (format)
    {
    case rhi::VertexFormat::R32G32B32_FLOAT:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case rhi::VertexFormat::R32G32_FLOAT:
        return DXGI_FORMAT_R32G32_FLOAT;
    case rhi::VertexFormat::R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case rhi::VertexFormat::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_COMPARISON_FUNC ToD3DCompare(rhi::CompareOp compareOp)
{
    switch (compareOp)
    {
    case rhi::CompareOp::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    case rhi::CompareOp::Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case rhi::CompareOp::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case rhi::CompareOp::LessEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case rhi::CompareOp::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case rhi::CompareOp::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case rhi::CompareOp::GreaterEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case rhi::CompareOp::Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    default:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }
}

D3D12_CULL_MODE ToD3DCull(rhi::CullMode cullMode)
{
    switch (cullMode)
    {
    case rhi::CullMode::None:
        return D3D12_CULL_MODE_NONE;
    case rhi::CullMode::Front:
        return D3D12_CULL_MODE_FRONT;
    case rhi::CullMode::Back:
        return D3D12_CULL_MODE_BACK;
    default:
        return D3D12_CULL_MODE_BACK;
    }
}

bool IsDepthFormat(rhi::Format format)
{
    return format == rhi::Format::D32_FLOAT || format == rhi::Format::D24_UNORM_S8_UINT;
}

const char* SemanticName(rhi::VertexSemantic semantic)
{
    switch (semantic)
    {
    case rhi::VertexSemantic::Position:
        return "POSITION";
    case rhi::VertexSemantic::Normal:
        return "NORMAL";
    case rhi::VertexSemantic::TexCoord:
        return "TEXCOORD";
    case rhi::VertexSemantic::Color:
        return "COLOR";
    case rhi::VertexSemantic::Tangent:
        return "TANGENT";
    default:
        return "TEXCOORD";
    }
}

std::uint32_t IndexSizeBytes(rhi::IndexType indexType)
{
    return (indexType == rhi::IndexType::UINT16) ? 2u : 4u;
}

export namespace rhi
{
    class DX12Device;

    struct DX12SwapChainDesc
    {
        SwapChainDesc base{};
        HWND hwnd{ nullptr };
        std::uint32_t bufferCount{ 2 };
    };

    class DX12SwapChain final : public IRHISwapChain
    {
    public:
        DX12SwapChain(DX12Device& device, DX12SwapChainDesc desc);
        ~DX12SwapChain() override = default;

        SwapChainDesc GetDesc() const override;
        FrameBufferHandle GetCurrentBackBuffer() const override;
        void Present() override;

        ID3D12Resource* CurrentBackBuffer() const { return backBuffers_[frameIndex_].Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentRTV() const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            h.ptr += static_cast<SIZE_T>(frameIndex_) * rtvInc_;
            return h;
        }

        ID3D12Resource* DepthBuffer() const { return depth_.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE DSV() const { return dsv_; }

        DXGI_FORMAT BackBufferFormat() const { return bbFormat_; }
        DXGI_FORMAT DepthFormat() const { return depthFormat_; }

        void EnsureSizeUpToDate();

    private:
        DX12Device& device_;
        DX12SwapChainDesc chainSwapDesc_;

        ComPtr<IDXGISwapChain4> swapChain_;
        ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        UINT rtvInc_{ 0 };

        std::vector<ComPtr<ID3D12Resource>> backBuffers_;
        UINT frameIndex_{ 0 };
        DXGI_FORMAT bbFormat_{ DXGI_FORMAT_B8G8R8A8_UNORM };

        ComPtr<ID3D12Resource> depth_;
        ComPtr<ID3D12DescriptorHeap> dsvHeap_;
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_{};
        DXGI_FORMAT depthFormat_{ DXGI_FORMAT_D32_FLOAT };
    };

#if defined(_WIN32)
    class DX12Device final : public IRHIDevice
    {
    public:
        DX12Device()
        {
            core_.Init();

            // Command allocator/list
            ThrowIfFailed(core_.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc_)),
                "DX12: CreateCommandAllocator failed");
            ThrowIfFailed(core_.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc_.Get(), nullptr, IID_PPV_ARGS(&cmdList_)),
                "DX12: CreateCommandList failed");
            cmdList_->Close();

            // Fence
            ThrowIfFailed(core_.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
                "DX12: CreateFence failed");
            fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (!fenceEvent_) throw std::runtime_error("DX12: CreateEvent failed");

            // SRV heap (shader visible)
            {
                D3D12_DESCRIPTOR_HEAP_DESC hd{};
                hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                hd.NumDescriptors = 1024;
                hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                ThrowIfFailed(core_.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srvHeap_)),
                    "DX12: Create SRV heap failed");
                srvInc_ = core_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                // null SRV in slot 0
                D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap_->GetCPUDescriptorHandleForHeapStart();
                D3D12_SHADER_RESOURCE_VIEW_DESC nullSrv{};
                nullSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                nullSrv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                nullSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                nullSrv.Texture2D.MipLevels = 1;
                core_.device->CreateShaderResourceView(nullptr, &nullSrv, cpu);

                nextSrvIndex_ = 1;
            }

            // Constant buffer (upload) – 64KB
            {
                const UINT constBufSize = 64u * 1024u;
                D3D12_HEAP_PROPERTIES hp{};
                hp.Type = D3D12_HEAP_TYPE_UPLOAD;

                D3D12_RESOURCE_DESC rd{};
                rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                rd.Width = constBufSize;
                rd.Height = 1;
                rd.DepthOrArraySize = 1;
                rd.MipLevels = 1;
                rd.Format = DXGI_FORMAT_UNKNOWN;
                rd.SampleDesc.Count = 1;
                rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

                ThrowIfFailed(core_.device->CreateCommittedResource(
                    &hp, D3D12_HEAP_FLAG_NONE, &rd,
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                    IID_PPV_ARGS(&constantBufferUpload_)),
                    "DX12: Create constant upload buffer failed");

                ThrowIfFailed(constantBufferUpload_->Map(0, nullptr, reinterpret_cast<void**>(&constantBufferMapped_)),
                    "DX12: Map constant upload buffer failed");
            }

            CreateRootSignature();
        }

        ~DX12Device() override
        {
            if (constantBufferUpload_)
            {
                constantBufferUpload_->Unmap(0, nullptr);
            }
            constantBufferMapped_ = nullptr;

            if (fenceEvent_)
            {
                CloseHandle(fenceEvent_);
                fenceEvent_ = nullptr;
            }
        }

        void ReplaceSampledTextureResource(rhi::TextureHandle h, ID3D12Resource* newRes, DXGI_FORMAT fmt, UINT mipLevels)
        {
            auto it = textures_.find(h.id);
            if (it == textures_.end())
                throw std::runtime_error("DX12: ReplaceSampledTextureResource: texture handle not found");

            it->second.resource.Reset();
            it->second.resource.Attach(newRes); // takes ownership (AddRef already implied by Attach contract)
            it->second.hasSRV = false;

            AllocateSRV(it->second, fmt, mipLevels);
        } /// DX12Device

        TextureHandle RegisterSampledTexture(ID3D12Resource* res, DXGI_FORMAT fmt, UINT mipLevels)
        {
            if (!res) return {};

            TextureHandle h{ ++nextTexId_ };
            TextureEntry e{};

            // Fill extent from resource desc
            const D3D12_RESOURCE_DESC rd = res->GetDesc();
            e.extent = Extent2D{ (std::uint32_t)rd.Width, (std::uint32_t)rd.Height };
            e.format = rhi::Format::RGBA8_UNORM; // internal book-keeping only (engine side)

            // Take ownership (AddRef)
            e.resource = res;

            // Allocate SRV in our shader-visible heap
            AllocateSRV(e, fmt, mipLevels);

            textures_[h.id] = std::move(e);
            return h;
        }

        void SetSwapChain(DX12SwapChain* sc) { swapChain_ = sc; }

        std::string_view GetName() const override { return "DirectX12 RHI"; }
        Backend GetBackend() const noexcept override { return Backend::DirectX12; }

        // ---------------- Textures (RenderGraph transient) ----------------
        TextureHandle CreateTexture2D(Extent2D extent, Format format) override
        {
            TextureHandle h{ ++nextTexId_ };
            TextureEntry e{};
            e.extent = extent;
            e.format = format;

            const DXGI_FORMAT dxFmt = ToDXGIFormat(format);

            D3D12_HEAP_PROPERTIES hp{};
            hp.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_RESOURCE_DESC rd{};
            rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            rd.Width = extent.width;
            rd.Height = extent.height;
            rd.DepthOrArraySize = 1;
            rd.MipLevels = 1;
            rd.Format = dxFmt;
            rd.SampleDesc.Count = 1;
            rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

            D3D12_CLEAR_VALUE cv{};
            cv.Format = dxFmt;

            D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;

            if (IsDepthFormat(format))
            {
                rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                cv.DepthStencil.Depth = 1.0f;
                cv.DepthStencil.Stencil = 0;
                initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                ThrowIfFailed(core_.device->CreateCommittedResource(
                    &hp, D3D12_HEAP_FLAG_NONE, &rd, initState, &cv, IID_PPV_ARGS(&e.resource)),
                    "DX12: Create depth texture failed");

                EnsureDSVHeap();
                e.dsv = AllocateDSV(e.resource.Get(), dxFmt);
            }
            else
            {
                rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                cv.Color[0] = 0.0f; cv.Color[1] = 0.0f; cv.Color[2] = 0.0f; cv.Color[3] = 1.0f;
                initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
                ThrowIfFailed(core_.device->CreateCommittedResource(
                    &hp, D3D12_HEAP_FLAG_NONE, &rd, initState, &cv, IID_PPV_ARGS(&e.resource)),
                    "DX12: Create color texture failed");

                EnsureRTVHeap();
                e.rtv = AllocateRTV(e.resource.Get(), dxFmt);
            }

            textures_[h.id] = std::move(e);
            return h;
        }

        void DestroyTexture(TextureHandle texture) noexcept override
        {
            if (texture.id == 0) return;
            textures_.erase(texture.id);
            // SRV slot reclaim (простая версия): не реюзаем, ок для демо.
        }

        // ---------------- Framebuffers ----------------
        FrameBufferHandle CreateFramebuffer(TextureHandle color, TextureHandle depth) override
        {
            FrameBufferHandle fb{ ++nextFBId_ };
            FramebufferEntry e{};
            e.color = color;
            e.depth = depth;
            framebuffers_[fb.id] = e;
            return fb;
        }

        void DestroyFramebuffer(FrameBufferHandle fb) noexcept override
        {
            if (fb.id == 0) return;
            framebuffers_.erase(fb.id);
        }

        // ---------------- Buffers ----------------
        BufferHandle CreateBuffer(const BufferDesc& desc) override
        {
            BufferHandle h{ ++nextBufId_ };
            BufferEntry e{};
            e.desc = desc;

            const UINT64 sz = static_cast<UINT64>(desc.sizeInBytes);

            D3D12_HEAP_PROPERTIES hp{};
            hp.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC rd{};
            rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            rd.Width = std::max<UINT64>(1, sz);
            rd.Height = 1;
            rd.DepthOrArraySize = 1;
            rd.MipLevels = 1;
            rd.Format = DXGI_FORMAT_UNKNOWN;
            rd.SampleDesc.Count = 1;
            rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            ThrowIfFailed(core_.device->CreateCommittedResource(
                &hp, D3D12_HEAP_FLAG_NONE, &rd,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&e.resource)),
                "DX12: CreateBuffer failed");

            buffers_[h.id] = std::move(e);
            return h;
        }

        void UpdateBuffer(BufferHandle buffer, std::span<const std::byte> data, std::size_t offsetBytes = 0) override
        {
            auto it = buffers_.find(buffer.id);
            if (it == buffers_.end()) return;

            void* p = nullptr;
            D3D12_RANGE readRange{ 0, 0 };
            ThrowIfFailed(it->second.resource->Map(0, &readRange, &p), "DX12: Map buffer failed");

            std::memcpy(static_cast<std::uint8_t*>(p) + offsetBytes, data.data(), data.size());

            it->second.resource->Unmap(0, nullptr);
        }

        void DestroyBuffer(BufferHandle buffer) noexcept override
        {
            buffers_.erase(buffer.id);
        }

        // ---------------- Input layouts ----------------
        InputLayoutHandle CreateInputLayout(const InputLayoutDesc& desc) override
        {
            InputLayoutHandle h{ ++nextLayoutId_ };
            InputLayoutEntry e{};
            e.strideBytes = desc.strideBytes;

            e.semanticStorage.reserve(desc.attributes.size());
            e.elems.reserve(desc.attributes.size());

            for (const auto& a : desc.attributes)
            {
                e.semanticStorage.emplace_back(SemanticName(a.semantic));

                D3D12_INPUT_ELEMENT_DESC d{};
                d.SemanticName = e.semanticStorage.back().c_str();
                d.SemanticIndex = a.semanticIndex;
                d.Format = ToDXGIVertexFormat(a.format);
                d.InputSlot = a.inputSlot;
                d.AlignedByteOffset = a.offsetBytes;
                d.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                d.InstanceDataStepRate = 0;

                e.elems.push_back(d);
            }

            layouts_[h.id] = std::move(e);
            return h;
        }

        void DestroyInputLayout(InputLayoutHandle layout) noexcept override
        {
            layouts_.erase(layout.id);
        }

        // ---------------- Shaders / Pipelines ----------------
        ShaderHandle CreateShader(ShaderStage stage, std::string_view debugName, std::string_view sourceOrBytecode) override
        {
            ShaderHandle h{ ++nextShaderId_ };
            ShaderEntry e{};
            e.stage = stage;
            e.name = std::string(debugName);

            const char* entry = (stage == ShaderStage::Vertex) ? "VSMain" : "PSMain";
            const char* target = (stage == ShaderStage::Vertex) ? "vs_5_1" : "ps_5_1";

            UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            ComPtr<ID3DBlob> code;
            ComPtr<ID3DBlob> errors;

            HRESULT hr = D3DCompile(
                sourceOrBytecode.data(),
                sourceOrBytecode.size(),
                e.name.c_str(),
                nullptr, nullptr,
                entry, target,
                flags, 0,
                &code, &errors);

            if (FAILED(hr))
            {
                std::string err = "DX12: shader compile failed: ";
                if (errors) err += std::string(reinterpret_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
                throw std::runtime_error(err);
            }

            e.blob = code;
            shaders_[h.id] = std::move(e);
            return h;
        }

        void DestroyShader(ShaderHandle shader) noexcept override
        {
            shaders_.erase(shader.id);
        }

        PipelineHandle CreatePipeline(std::string_view debugName, ShaderHandle vertexShader, ShaderHandle pixelShader) override
        {
            PipelineHandle h{ ++nextPsoId_ };
            PipelineEntry e{};
            e.debugName = std::string(debugName);
            e.vs = vertexShader;
            e.ps = pixelShader;
            pipelines_[h.id] = std::move(e);
            return h;
        }

        void DestroyPipeline(PipelineHandle pso) noexcept override
        {
            pipelines_.erase(pso.id);
            // PSO cache entries можно чистить отдельно, но для демо ок.
        }

        // ---------------- Submission ----------------
        void SubmitCommandList(CommandList&& commandList) override
        {
            if (!swapChain_)
                throw std::runtime_error("DX12: swapchain is not set on device (CreateDX12SwapChain must set it).");

            // Reset native command list
            ThrowIfFailed(cmdAlloc_->Reset(), "DX12: cmdAlloc reset failed");
            ThrowIfFailed(cmdList_->Reset(cmdAlloc_.Get(), nullptr), "DX12: cmdList reset failed");

            // Set descriptor heaps (SRV)
            ID3D12DescriptorHeap* heaps[] = { srvHeap_.Get() };
            cmdList_->SetDescriptorHeaps(1, heaps);

            // State while parsing high-level commands
            GraphicsState curState{};
            PipelineHandle curPipe{};
            InputLayoutHandle curLayout{};
            BufferHandle vb{};
            std::uint32_t vbStride = 0;
            std::uint32_t vbOffset = 0;

            BufferHandle ib{};
            IndexType ibType = IndexType::UINT16;
            std::uint32_t ibOffset = 0;

            // Bound textures by slot (we only реально используем slot 0)
            std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 8> boundTex{};
            for (auto& t : boundTex)
                t = srvHeap_->GetGPUDescriptorHandleForHeapStart(); // null SRV slot0

			// Per-draw constants (raw bytes).
			// The renderer is responsible for packing the layout expected by HLSL.
			std::array<std::byte, 256> perDrawBytes{};
			std::uint32_t perDrawSize = 0;
			std::uint32_t perDrawSlot = 0;

            constantBufferCursor_ = 0;

			auto WriteCBAndBind = [&]()
                {
					const std::uint32_t used = (perDrawSize == 0) ? 1u : perDrawSize;
					const std::uint32_t cbSize = AlignUp(used, 256);
                    if (constantBufferCursor_ + cbSize > (64u * 1024u))
                        constantBufferCursor_ = 0; // wrap (демо)

					if (perDrawSize != 0)
					{
						std::memcpy(constantBufferMapped_ + constantBufferCursor_, perDrawBytes.data(), perDrawSize);
					}

                    const D3D12_GPU_VIRTUAL_ADDRESS gpuVA = constantBufferUpload_->GetGPUVirtualAddress() + constantBufferCursor_;
					cmdList_->SetGraphicsRootConstantBufferView(perDrawSlot, gpuVA);

                    constantBufferCursor_ += cbSize;
                };

            auto ResolveTextureHandleFromDesc = [&](TextureDescIndex idx) -> TextureHandle
                {
                    auto it = descToTex_.find(idx);
                    if (it == descToTex_.end()) return {};
                    return it->second;
                };

            auto GetTextureSRV = [&](TextureHandle th) -> D3D12_GPU_DESCRIPTOR_HANDLE
                {
                    if (!th) return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    auto it = textures_.find(th.id);
                    if (it == textures_.end()) return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    if (!it->second.hasSRV) return srvHeap_->GetGPUDescriptorHandleForHeapStart();
                    return it->second.srvGpu;
                };

            auto EnsurePSO = [&](PipelineHandle p, InputLayoutHandle layout) -> ID3D12PipelineState*
                {
                    const std::uint64_t key =
                        (static_cast<std::uint64_t>(p.id) << 32ull) |
                        (static_cast<std::uint64_t>(layout.id) & 0xffffffffull);

                    if (auto it = psoCache_.find(key); it != psoCache_.end())
                        return it->second.Get();

                    auto pit = pipelines_.find(p.id);
                    if (pit == pipelines_.end())
                        throw std::runtime_error("DX12: pipeline handle not found");

                    auto vsIt = shaders_.find(pit->second.vs.id);
                    auto psIt = shaders_.find(pit->second.ps.id);
                    if (vsIt == shaders_.end() || psIt == shaders_.end())
                        throw std::runtime_error("DX12: shader handle not found");

                    auto layIt = layouts_.find(layout.id);
                    if (layIt == layouts_.end())
                        throw std::runtime_error("DX12: input layout handle not found");

                    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
                    d.pRootSignature = rootSig_.Get();

                    d.VS = { vsIt->second.blob->GetBufferPointer(), vsIt->second.blob->GetBufferSize() };
                    d.PS = { psIt->second.blob->GetBufferPointer(), psIt->second.blob->GetBufferSize() };

                    d.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
                    d.SampleMask = UINT_MAX;

                    // Rasterizer from current state
                    d.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
                    d.RasterizerState.CullMode = ToD3DCull(curState.rasterizer.cullMode);
                    d.RasterizerState.FrontCounterClockwise = (curState.rasterizer.frontFace == FrontFace::CounterClockwise) ? TRUE : FALSE;

                    // Depth
                    d.DepthStencilState = CD3D12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                    d.DepthStencilState.DepthEnable = curState.depth.testEnable ? TRUE : FALSE;
                    d.DepthStencilState.DepthWriteMask = curState.depth.writeEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
                    d.DepthStencilState.DepthFunc = ToD3DCompare(curState.depth.depthCompareOp);

                    d.InputLayout = { layIt->second.elems.data(), static_cast<UINT>(layIt->second.elems.size()) };
                    d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

                    d.NumRenderTargets = 1;
                    d.RTVFormats[0] = swapChain_->BackBufferFormat();
                    d.DSVFormat = swapChain_->DepthFormat();

                    d.SampleDesc.Count = 1;

                    ComPtr<ID3D12PipelineState> pso;
                    ThrowIfFailed(core_.device->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&pso)),
                        "DX12: CreateGraphicsPipelineState failed");

                    psoCache_[key] = pso;
                    return pso.Get();
                };

            // Parse high-level commands and record native D3D12
            for (auto& c : commandList.commands)
            {
                std::visit([&](auto&& cmd)
                    {
                        using T = std::decay_t<decltype(cmd)>;

                        if constexpr (std::is_same_v<T, CommandBeginPass>)
                        {
                            // frameBuffer.id == 0 => swapchain backbuffer
                            D3D12_CPU_DESCRIPTOR_HANDLE rtv = swapChain_->CurrentRTV();
                            D3D12_CPU_DESCRIPTOR_HANDLE dsv = swapChain_->DSV();
                            cmdList_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

                            // viewport & scissor
                            D3D12_VIEWPORT vp{};
                            vp.TopLeftX = 0;
                            vp.TopLeftY = 0;
                            vp.Width = static_cast<float>(cmd.desc.extent.width);
                            vp.Height = static_cast<float>(cmd.desc.extent.height);
                            vp.MinDepth = 0.0f;
                            vp.MaxDepth = 1.0f;
                            cmdList_->RSSetViewports(1, &vp);

                            D3D12_RECT sc{};
                            sc.left = 0;
                            sc.top = 0;
                            sc.right = static_cast<LONG>(cmd.desc.extent.width);
                            sc.bottom = static_cast<LONG>(cmd.desc.extent.height);
                            cmdList_->RSSetScissorRects(1, &sc);

                            // Clear
                            if (cmd.desc.clearDesc.clearColor)
                            {
                                cmdList_->ClearRenderTargetView(rtv, cmd.desc.clearDesc.color.data(), 0, nullptr);
                            }
                            if (cmd.desc.clearDesc.clearDepth)
                            {
                                cmdList_->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, cmd.desc.clearDesc.depth, 0, 0, nullptr);
                            }
                        }
                        else if constexpr (std::is_same_v<T, CommandEndPass>)
                        {
                            // no-op
                        }
                        else if constexpr (std::is_same_v<T, CommandSetViewport>)
                        {
                            D3D12_VIEWPORT vp{};
                            vp.TopLeftX = static_cast<float>(cmd.x);
                            vp.TopLeftY = static_cast<float>(cmd.y);
                            vp.Width = static_cast<float>(cmd.width);
                            vp.Height = static_cast<float>(cmd.height);
                            vp.MinDepth = 0.0f;
                            vp.MaxDepth = 1.0f;
                            cmdList_->RSSetViewports(1, &vp);

                            D3D12_RECT sc{};
                            sc.left = cmd.x;
                            sc.top = cmd.y;
                            sc.right = cmd.x + cmd.width;
                            sc.bottom = cmd.y + cmd.height;
                            cmdList_->RSSetScissorRects(1, &sc);
                        }
                        else if constexpr (std::is_same_v<T, CommandSetState>)
                        {
                            curState = cmd.state;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindPipeline>)
                        {
                            curPipe = cmd.pso;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindInputLayout>)
                        {
                            curLayout = cmd.layout;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindVertexBuffer>)
                        {
                            vb = cmd.buffer;
                            vbStride = cmd.strideBytes;
                            vbOffset = cmd.offsetBytes;
                        }
                        else if constexpr (std::is_same_v<T, CommandBindIndexBuffer>)
                        {
                            ib = cmd.buffer;
                            ibType = cmd.indexType;
                            ibOffset = cmd.offsetBytes;
                        }
                        else if constexpr (std::is_same_v<T, CommnadBindTextue2D>)
                        {
                            if (cmd.slot < boundTex.size())
                                boundTex[cmd.slot] = GetTextureSRV(cmd.texture);
                        }
                        else if constexpr (std::is_same_v<T, CommandTextureDesc>)
                        {
                            if (cmd.slot < boundTex.size())
                            {
                                TextureHandle th = ResolveTextureHandleFromDesc(cmd.texture);
                                boundTex[cmd.slot] = GetTextureSRV(th);
                            }
                        }
						else if constexpr (std::is_same_v<T, CommandSetUniformInt> ||
							std::is_same_v<T, CommandUniformFloat4> ||
							std::is_same_v<T, CommandUniformMat4>)
						{
							// DX12 backend does not interpret the name-based uniform commands.
							// Use CommandSetConstants instead.
						}
						else if constexpr (std::is_same_v<T, CommandSetConstants>)
						{
							perDrawSlot = cmd.slot;
							perDrawSize = cmd.size;
							if (perDrawSize > 256)
								perDrawSize = 256;
							if (perDrawSize != 0)
								std::memcpy(perDrawBytes.data(), cmd.data.data(), perDrawSize);
						}
                        else if constexpr (std::is_same_v<T, CommandDrawIndexed>)
                        {
                            // PSO + RootSig
                            ID3D12PipelineState* pso = EnsurePSO(curPipe, curLayout);
                            cmdList_->SetPipelineState(pso);
                            cmdList_->SetGraphicsRootSignature(rootSig_.Get());

                            // IA bindings
                            auto vbIt = buffers_.find(vb.id);
                            if (vbIt == buffers_.end())
                                throw std::runtime_error("DX12: vertex buffer not found");

                            D3D12_VERTEX_BUFFER_VIEW vbv{};
                            vbv.BufferLocation = vbIt->second.resource->GetGPUVirtualAddress() + vbOffset;
                            vbv.SizeInBytes = static_cast<UINT>(vbIt->second.desc.sizeInBytes - vbOffset);
                            vbv.StrideInBytes = vbStride;

                            cmdList_->IASetVertexBuffers(0, 1, &vbv);
                            cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                            if (ib)
                            {
                                auto ibIt = buffers_.find(ib.id);
                                if (ibIt == buffers_.end())
                                    throw std::runtime_error("DX12: index buffer not found");

                                D3D12_INDEX_BUFFER_VIEW ibv{};
                                ibv.BufferLocation = ibIt->second.resource->GetGPUVirtualAddress() + ibOffset
                                    + static_cast<UINT64>(cmd.firstIndex) * static_cast<UINT64>(IndexSizeBytes(cmd.indexType));
                                ibv.SizeInBytes = static_cast<UINT>(ibIt->second.desc.sizeInBytes - ibOffset);
                                ibv.Format = (cmd.indexType == IndexType::UINT16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

                                cmdList_->IASetIndexBuffer(&ibv);
                            }

                            // Root bindings: CBV (0) + SRV table (1)
                            WriteCBAndBind();
                            cmdList_->SetGraphicsRootDescriptorTable(1, boundTex[0]);

                            cmdList_->DrawIndexedInstanced(cmd.indexCount, 1, 0, cmd.baseVertex, 0);
                        }
                        else if constexpr (std::is_same_v<T, CommandDraw>)
                        {
                            ID3D12PipelineState* pso = EnsurePSO(curPipe, curLayout);
                            cmdList_->SetPipelineState(pso);
                            cmdList_->SetGraphicsRootSignature(rootSig_.Get());

                            auto vbIt = buffers_.find(vb.id);
                            if (vbIt == buffers_.end())
                                throw std::runtime_error("DX12: vertex buffer not found");

                            D3D12_VERTEX_BUFFER_VIEW vbv{};
                            vbv.BufferLocation = vbIt->second.resource->GetGPUVirtualAddress() + vbOffset;
                            vbv.SizeInBytes = static_cast<UINT>(vbIt->second.desc.sizeInBytes - vbOffset);
                            vbv.StrideInBytes = vbStride;

                            cmdList_->IASetVertexBuffers(0, 1, &vbv);
                            cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                            WriteCBAndBind();
                            cmdList_->SetGraphicsRootDescriptorTable(1, boundTex[0]);

                            cmdList_->DrawInstanced(cmd.vertexCount, 1, cmd.firstVertex, 0);
                        }
                        else
                        {
                            // other commands ignored
                        }

                    }, c);
            }

            ThrowIfFailed(cmdList_->Close(), "DX12: cmdList close failed");

            ID3D12CommandList* lists[] = { cmdList_.Get() };
            core_.cmdQueue->ExecuteCommandLists(1, lists);

            // Wait GPU (простая, но надежная синхронизация для демо)
            SignalAndWait();
        }

        // ---------------- Bindless descriptor indices ----------------
        TextureDescIndex AllocateTextureDesctiptor(TextureHandle tex) override
        {
            const auto idx = ++nextDescId_;
            descToTex_[idx] = tex;
            return idx;
        }

        void UpdateTextureDescriptor(TextureDescIndex idx, TextureHandle tex) override
        {
            descToTex_[idx] = tex;
        }

        void FreeTextureDescriptor(TextureDescIndex idx) noexcept override
        {
            descToTex_.erase(idx);
        }

        // ---------------- Fences (минимально) ----------------
        FenceHandle CreateFence(bool signaled = false) override
        {
            const auto id = ++nextFenceId_;
            fences_[id] = signaled;
            return FenceHandle{ id };
        }

        void DestroyFence(FenceHandle fence) noexcept override
        {
            fences_.erase(fence.id);
        }

        void SignalFence(FenceHandle fence) override
        {
            fences_[fence.id] = true;
        }

        void WaitFence(FenceHandle) override {}

        bool IsFenceSignaled(FenceHandle fence) override
        {
            auto it = fences_.find(fence.id);
            return it != fences_.end() && it->second;
        }

        // ---------- PUBLIC helpers for uploader (будет вызван из DX12TextureUploader) ----------
        // (сейчас заглушка – настоящий upload + mipmaps будет во Файле 2)
        TextureHandle CreateSampledTextureStub_OnlySRV(Extent2D extent, DXGI_FORMAT fmt)
        {
            // Create empty 1-mip texture + SRV (чтобы BindTexture2D работал)
            TextureHandle h{ ++nextTexId_ };
            TextureEntry e{};
            e.extent = extent;
            e.format = rhi::Format::RGBA8_UNORM;

            D3D12_HEAP_PROPERTIES hp{};
            hp.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_RESOURCE_DESC rd{};
            rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            rd.Width = extent.width;
            rd.Height = extent.height;
            rd.DepthOrArraySize = 1;
            rd.MipLevels = 1;
            rd.Format = fmt;
            rd.SampleDesc.Count = 1;
            rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            rd.Flags = D3D12_RESOURCE_FLAG_NONE;

            ThrowIfFailed(core_.device->CreateCommittedResource(
                &hp, D3D12_HEAP_FLAG_NONE, &rd,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr, IID_PPV_ARGS(&e.resource)),
                "DX12: Create sampled texture failed");

            AllocateSRV(e, fmt, 1);
            textures_[h.id] = std::move(e);
            return h;
        }

        ID3D12Device* NativeDevice() const { return core_.device.Get(); }
        ID3D12CommandQueue* NativeQueue() const { return core_.cmdQueue.Get(); }
        ID3D12DescriptorHeap* NativeSRVHeap() const { return srvHeap_.Get(); }
        UINT NativeSRVInc() const { return srvInc_; }

    private:
        friend class DX12SwapChain;

        struct BufferEntry
        {
            BufferDesc desc{};
            ComPtr<ID3D12Resource> resource;
        };

        struct InputLayoutEntry
        {
            std::vector<std::string> semanticStorage;
            std::vector<D3D12_INPUT_ELEMENT_DESC> elems;
            std::uint32_t strideBytes{ 0 };
        };

        struct ShaderEntry
        {
            ShaderStage stage{};
            std::string name;
            ComPtr<ID3DBlob> blob;
        };

        struct PipelineEntry
        {
            std::string debugName;
            ShaderHandle vs{};
            ShaderHandle ps{};
        };

        struct TextureEntry
        {
            Extent2D extent{};
            rhi::Format format{ rhi::Format::Unknown };
            ComPtr<ID3D12Resource> resource;

            // Render targets / depth
            bool hasRTV{ false };
            bool hasDSV{ false };
            D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
            D3D12_CPU_DESCRIPTOR_HANDLE dsv{};

            // Sampled
            bool hasSRV{ false };
            UINT srvIndex{ 0 };
            D3D12_GPU_DESCRIPTOR_HANDLE srvGpu{};
        };

        struct FramebufferEntry
        {
            TextureHandle color{};
            TextureHandle depth{};
        };

        void SignalAndWait()
        {
            const UINT64 v = ++fenceValue_;
            ThrowIfFailed(core_.cmdQueue->Signal(fence_.Get(), v), "DX12: Signal failed");
            if (fence_->GetCompletedValue() < v)
            {
                ThrowIfFailed(fence_->SetEventOnCompletion(v, fenceEvent_), "DX12: SetEventOnCompletion failed");
                WaitForSingleObject(fenceEvent_, INFINITE);
            }
        }

        void CreateRootSignature()
        {
            // Root params: 0 = CBV(b0), 1 = SRV table (t0)
            D3D12_DESCRIPTOR_RANGE1 range{};
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range.NumDescriptors = 1;
            range.BaseShaderRegister = 0;
            range.RegisterSpace = 0;
            range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
            range.OffsetInDescriptorsFromTableStart = 0;

            D3D12_ROOT_PARAMETER1 params[2]{};

            params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            params[0].Descriptor.ShaderRegister = 0;
            params[0].Descriptor.RegisterSpace = 0;
            params[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

            params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            params[1].DescriptorTable.NumDescriptorRanges = 1;
            params[1].DescriptorTable.pDescriptorRanges = &range;

            // Static sampler s0
            D3D12_STATIC_SAMPLER_DESC samp{};
            samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samp.MipLODBias = 0;
            samp.MaxAnisotropy = 1;
            samp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            samp.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            samp.MinLOD = 0.0f;
            samp.MaxLOD = D3D12_FLOAT32_MAX;
            samp.ShaderRegister = 0; // s0
            samp.RegisterSpace = 0;
            samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_VERSIONED_ROOT_SIGNATURE_DESC rs{};
            rs.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
            rs.Desc_1_1.NumParameters = 2;
            rs.Desc_1_1.pParameters = params;
            rs.Desc_1_1.NumStaticSamplers = 1;
            rs.Desc_1_1.pStaticSamplers = &samp;
            rs.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> blob;
            ComPtr<ID3DBlob> err;
            ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rs, &blob, &err),
                "DX12: Serialize root signature failed");

            ThrowIfFailed(core_.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig_)),
                "DX12: CreateRootSignature failed");
        }

        void EnsureRTVHeap()
        {
            if (rtvHeap_) return;
            D3D12_DESCRIPTOR_HEAP_DESC hd{};
            hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            hd.NumDescriptors = 256;
            hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(core_.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)),
                "DX12: Create RTV heap failed");
            rtvInc_ = core_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            nextRTV_ = 0;
        }

        void EnsureDSVHeap()
        {
            if (dsvHeap_) return;
            D3D12_DESCRIPTOR_HEAP_DESC hd{};
            hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            hd.NumDescriptors = 64;
            hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(core_.device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&dsvHeap_)),
                "DX12: Create DSV heap failed");
            dsvInc_ = core_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            nextDSV_ = 0;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateRTV(ID3D12Resource* res, DXGI_FORMAT fmt)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            h.ptr += static_cast<SIZE_T>(nextRTV_) * rtvInc_;
            ++nextRTV_;

            D3D12_RENDER_TARGET_VIEW_DESC d{};
            d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            d.Format = fmt;
            d.Texture2D.MipSlice = 0;
            d.Texture2D.PlaneSlice = 0;
            core_.device->CreateRenderTargetView(res, &d, h);
            return h;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDSV(ID3D12Resource* res, DXGI_FORMAT fmt)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE h = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
            h.ptr += static_cast<SIZE_T>(nextDSV_) * dsvInc_;
            ++nextDSV_;

            D3D12_DEPTH_STENCIL_VIEW_DESC d{};
            d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            d.Format = fmt;
            d.Flags = D3D12_DSV_FLAG_NONE;
            d.Texture2D.MipSlice = 0;
            core_.device->CreateDepthStencilView(res, &d, h);
            return h;
        }

        void AllocateSRV(TextureEntry& e, DXGI_FORMAT fmt, UINT mipLevels)
        {
            const UINT idx = nextSrvIndex_++;
            D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap_->GetCPUDescriptorHandleForHeapStart();
            cpu.ptr += static_cast<SIZE_T>(idx) * srvInc_;

            D3D12_SHADER_RESOURCE_VIEW_DESC s{};
            s.Format = fmt;
            s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            s.Texture2D.MostDetailedMip = 0;
            s.Texture2D.MipLevels = mipLevels;
            s.Texture2D.ResourceMinLODClamp = 0.0f;

            core_.device->CreateShaderResourceView(e.resource.Get(), &s, cpu);

            D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvHeap_->GetGPUDescriptorHandleForHeapStart();
            gpu.ptr += static_cast<SIZE_T>(idx) * srvInc_;

            e.hasSRV = true;
            e.srvIndex = idx;
            e.srvGpu = gpu;
        }

    private:
        dx12::Core core_{};

        ComPtr<ID3D12CommandAllocator> cmdAlloc_;
        ComPtr<ID3D12GraphicsCommandList> cmdList_;

        ComPtr<ID3D12Fence> fence_;
        HANDLE fenceEvent_{ nullptr };
        UINT64 fenceValue_{ 0 };

        // Shared root signature
        ComPtr<ID3D12RootSignature> rootSig_;

        // SRV heap
        ComPtr<ID3D12DescriptorHeap> srvHeap_;
        UINT srvInc_{ 0 };
        UINT nextSrvIndex_{ 1 };

        // RTV/DSV heaps for transient textures (swapchain has its own RTV/DSV)
        ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        ComPtr<ID3D12DescriptorHeap> dsvHeap_;
        UINT rtvInc_{ 0 };
        UINT dsvInc_{ 0 };
        UINT nextRTV_{ 0 };
        UINT nextDSV_{ 0 };

        // Constant upload
        ComPtr<ID3D12Resource> constantBufferUpload_;
        std::uint8_t* constantBufferMapped_{ nullptr };
        std::uint32_t constantBufferCursor_{ 0 };

        // Pointers
        DX12SwapChain* swapChain_{ nullptr };

        // Resource tables
        std::uint32_t nextBufId_{ 1 };
        std::uint32_t nextTexId_{ 1 };
        std::uint32_t nextShaderId_{ 1 };
        std::uint32_t nextPsoId_{ 1 };
        std::uint32_t nextLayoutId_{ 1 };
        std::uint32_t nextFBId_{ 1 };
        std::uint32_t nextDescId_{ 1 };
        std::uint32_t nextFenceId_{ 1 };

        std::unordered_map<std::uint32_t, BufferEntry> buffers_;
        std::unordered_map<std::uint32_t, TextureEntry> textures_;
        std::unordered_map<std::uint32_t, ShaderEntry> shaders_;
        std::unordered_map<std::uint32_t, PipelineEntry> pipelines_;
        std::unordered_map<std::uint32_t, InputLayoutEntry> layouts_;
        std::unordered_map<std::uint32_t, FramebufferEntry> framebuffers_;

        std::unordered_map<TextureDescIndex, TextureHandle> descToTex_;
        std::unordered_map<std::uint32_t, bool> fences_;

        std::unordered_map<std::uint64_t, ComPtr<ID3D12PipelineState>> psoCache_;
    };

    DX12SwapChain::DX12SwapChain(DX12Device& owner, DX12SwapChainDesc desc)
        : device_(owner)
        , chainSwapDesc_(std::move(desc))
    {
        if (!chainSwapDesc_.hwnd)
            throw std::runtime_error("DX12SwapChain: hwnd is null");

        ComPtr<IDXGIFactory6> factory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "DX12: CreateDXGIFactory2 failed");

        DXGI_SWAP_CHAIN_DESC1 sc{};
        sc.Width = chainSwapDesc_.base.extent.width;
        sc.Height = chainSwapDesc_.base.extent.height;
        sc.Format = ToDXGIFormat(chainSwapDesc_.base.backbufferFormat);
        bbFormat_ = sc.Format;
        sc.SampleDesc.Count = 1;
        sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sc.BufferCount = std::max(2u, chainSwapDesc_.bufferCount);
        sc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sc.Scaling = DXGI_SCALING_STRETCH;
        sc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ComPtr<IDXGISwapChain1> sc1;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            device_.NativeQueue(),
            chainSwapDesc_.hwnd,
            &sc,
            nullptr, nullptr,
            &sc1),
            "DX12: CreateSwapChainForHwnd failed");

        ThrowIfFailed(sc1.As(&swapChain_), "DX12: swapchain As IDXGISwapChain4 failed");
        ThrowIfFailed(factory->MakeWindowAssociation(chainSwapDesc_.hwnd, DXGI_MWA_NO_ALT_ENTER), "DX12: MakeWindowAssociation failed");

        // RTV heap for backbuffers
        {
            D3D12_DESCRIPTOR_HEAP_DESC hd{};
            hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            hd.NumDescriptors = sc.BufferCount;
            hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(device_.NativeDevice()->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap_)),
                "DX12: Create swapchain RTV heap failed");
            rtvInc_ = device_.NativeDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        backBuffers_.resize(sc.BufferCount);
        for (UINT i = 0; i < sc.BufferCount; ++i)
        {
            ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])),
                "DX12: GetBuffer failed");

            D3D12_CPU_DESCRIPTOR_HANDLE h = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
            h.ptr += static_cast<SIZE_T>(i) * rtvInc_;
            device_.NativeDevice()->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, h);
        }

        frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

        // Depth (D32)
        depthFormat_ = DXGI_FORMAT_D32_FLOAT;
        {
            D3D12_DESCRIPTOR_HEAP_DESC hd{};
            hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            hd.NumDescriptors = 1;
            hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(device_.NativeDevice()->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&dsvHeap_)),
                "DX12: Create swapchain DSV heap failed");

            D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

            D3D12_HEAP_PROPERTIES hp{};
            hp.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_RESOURCE_DESC rd{};
            rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            rd.Width = chainSwapDesc_.base.extent.width;
            rd.Height = chainSwapDesc_.base.extent.height;
            rd.DepthOrArraySize = 1;
            rd.MipLevels = 1;
            rd.Format = depthFormat_;
            rd.SampleDesc.Count = 1;
            rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE cv{};
            cv.Format = depthFormat_;
            cv.DepthStencil.Depth = 1.0f;
            cv.DepthStencil.Stencil = 0;

            ThrowIfFailed(device_.NativeDevice()->CreateCommittedResource(
                &hp, D3D12_HEAP_FLAG_NONE, &rd,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &cv, IID_PPV_ARGS(&depth_)),
                "DX12: Create depth buffer failed");

            D3D12_DEPTH_STENCIL_VIEW_DESC vd{};
            vd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            vd.Format = depthFormat_;
            vd.Flags = D3D12_DSV_FLAG_NONE;
            vd.Texture2D.MipSlice = 0;

            device_.NativeDevice()->CreateDepthStencilView(depth_.Get(), &vd, dsv);
            dsv_ = dsv;
        }
    }

    SwapChainDesc DX12SwapChain::GetDesc() const
    {
        return chainSwapDesc_.base;
    }

    FrameBufferHandle DX12SwapChain::GetCurrentBackBuffer() const
    {
        // как в GL: 0 означает swapchain backbuffer
        return FrameBufferHandle{ 0 };
    }

    void DX12SwapChain::EnsureSizeUpToDate()
    {
        // демо-версия без resize; можно расширить позже
    }

    void DX12SwapChain::Present()
    {
        const UINT syncInterval = chainSwapDesc_.base.vsync ? 1u : 0u;
        ThrowIfFailed(swapChain_->Present(syncInterval, 0), "DX12: Present failed");
        frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
    }

    // Public factory functions
    inline std::unique_ptr<IRHIDevice> CreateDX12Device()
    {
        return std::make_unique<DX12Device>();
    }

    inline std::unique_ptr<IRHISwapChain> CreateDX12SwapChain(IRHIDevice& device, DX12SwapChainDesc desc)
    {
        auto* dxDev = dynamic_cast<DX12Device*>(&device);
        if (!dxDev)
            throw std::runtime_error("CreateDX12SwapChain: device is not DX12Device");

        auto sc = std::make_unique<DX12SwapChain>(*dxDev, std::move(desc));
        dxDev->SetSwapChain(sc.get());
        return sc;
    }

#else
inline std::unique_ptr<IRHIDevice> CreateDX12Device() { return CreateNullDevice(); }
#endif
} // namespace rhi
