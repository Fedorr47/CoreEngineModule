DX12SwapChain::DX12SwapChain(DX12Device& owner, DX12SwapChainDesc desc)
    : device_(owner)
    , chainSwapDesc_(std::move(desc))
{
    if (!chainSwapDesc_.hwnd)
    {
        throw std::runtime_error("DX12SwapChain: hwnd is null");
    }

    ComPtr<IDXGIFactory6> factory;
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "DX12: CreateDXGIFactory2 failed");

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = chainSwapDesc_.base.extent.width;
    swapChainDesc.Height = chainSwapDesc_.base.extent.height;
    swapChainDesc.Format = ToDXGIFormat(chainSwapDesc_.base.backbufferFormat);
    bbFormat_ = swapChainDesc.Format;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = std::max(2u, chainSwapDesc_.bufferCount);
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        device_.NativeQueue(),
        chainSwapDesc_.hwnd,
        &swapChainDesc,
        nullptr, nullptr,
        &swapChain1),
        "DX12: CreateSwapChainForHwnd failed");

    ThrowIfFailed(swapChain1.As(&swapChain_), "DX12: swapchain As IDXGISwapChain4 failed");
    ThrowIfFailed(factory->MakeWindowAssociation(chainSwapDesc_.hwnd, DXGI_MWA_NO_ALT_ENTER), "DX12: MakeWindowAssociation failed");

    // RTV heap for backbuffers
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = swapChainDesc.BufferCount;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device_.NativeDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap_)),
            "DX12: Create swapchain RTV heap failed");
        rtvInc_ = device_.NativeDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    backBuffers_.resize(swapChainDesc.BufferCount);
    for (UINT i = 0; i < swapChainDesc.BufferCount; ++i)
    {
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])),
            "DX12: GetBuffer failed");

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        descHandle.ptr += static_cast<SIZE_T>(i) * rtvInc_;
        device_.NativeDevice()->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, descHandle);
    }

    currBackBuffer_ = swapChain_->GetCurrentBackBufferIndex();

    // Depth (D32)
    depthFormat_ = DXGI_FORMAT_D32_FLOAT;
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device_.NativeDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&dsvHeap_)),
            "DX12: Create swapchain DSV heap failed");

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = chainSwapDesc_.base.extent.width;
        resourceDesc.Height = chainSwapDesc_.base.extent.height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = depthFormat_;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = depthFormat_;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(device_.NativeDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depth_)),
            "DX12: Create depth buffer failed");

        D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
        viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        viewDesc.Format = depthFormat_;
        viewDesc.Flags = D3D12_DSV_FLAG_NONE;
        viewDesc.Texture2D.MipSlice = 0;

        device_.NativeDevice()->CreateDepthStencilView(depth_.Get(), &viewDesc, dsv);
        dsv_ = dsv;
    }

    backBufferStates_.resize(backBuffers_.size());
    ResetBackBufferStates(D3D12_RESOURCE_STATE_PRESENT);
    currBackBuffer_ = swapChain_->GetCurrentBackBufferIndex();
}

SwapChainDesc DX12SwapChain::GetDesc() const
{
    return chainSwapDesc_.base;
}

FrameBufferHandle DX12SwapChain::GetCurrentBackBuffer() const
{
    // similar to GL: 0 stands to swapchain backbuffer
    return FrameBufferHandle{ 0 };
}



void DX12SwapChain::Resize(Extent2D newExtent)
{
    // NOTE: ResizeBuffers requires that all references to the swapchain buffers are released.
    if (newExtent.width == 0 || newExtent.height == 0)
    {
        // Minimized / hidden. Keep desc in sync, but don't touch DXGI buffers.
        chainSwapDesc_.base.extent = newExtent;
        return;
    }

    if (newExtent.width == chainSwapDesc_.base.extent.width && newExtent.height == chainSwapDesc_.base.extent.height)
    {
        return;
    }

    // Make sure GPU is not using the current backbuffers/depth.
    device_.WaitIdle();

    // Release current backbuffer/depth resources before ResizeBuffers.
    for (auto& bb : backBuffers_)
    {
        bb.Reset();
    }
    depth_.Reset();

    const UINT bufferCount = static_cast<UINT>(backBuffers_.size());

    ThrowIfFailed(swapChain_->ResizeBuffers(
        bufferCount,
        static_cast<UINT>(newExtent.width),
        static_cast<UINT>(newExtent.height),
        bbFormat_,
        0),
        "DX12: ResizeBuffers failed");

    // Recreate RTVs.
    for (UINT i = 0; i < bufferCount; ++i)
    {
        ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i])),
            "DX12: GetBuffer failed");

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
        descHandle.ptr += static_cast<SIZE_T>(i) * rtvInc_;
        device_.NativeDevice()->CreateRenderTargetView(backBuffers_[i].Get(), nullptr, descHandle);
    }

    // Recreate depth buffer + DSV.
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = static_cast<UINT64>(newExtent.width);
        resourceDesc.Height = static_cast<UINT>(newExtent.height);
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = depthFormat_;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = depthFormat_;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(device_.NativeDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depth_)),
            "DX12: Create depth buffer failed (Resize)");

        D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
        viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        viewDesc.Format = depthFormat_;
        viewDesc.Flags = D3D12_DSV_FLAG_NONE;
        viewDesc.Texture2D.MipSlice = 0;

        device_.NativeDevice()->CreateDepthStencilView(depth_.Get(), &viewDesc, dsv);
        dsv_ = dsv;
    }

    chainSwapDesc_.base.extent = newExtent;

    ResetBackBufferStates(D3D12_RESOURCE_STATE_PRESENT);
    currBackBuffer_ = swapChain_->GetCurrentBackBufferIndex();
}

void DX12SwapChain::EnsureSizeUpToDate()
{
    // TODO: could be extended
}

void DX12SwapChain::Present()
{
    const UINT syncInterval = chainSwapDesc_.base.vsync ? 1u : 0u;
    ThrowIfFailed(swapChain_->Present(syncInterval, 0), "DX12: Present failed");
    currBackBuffer_ = swapChain_->GetCurrentBackBufferIndex();
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
    {
        throw std::runtime_error("CreateDX12SwapChain: device is not DX12Device");
    }

    auto swapChainDesc = std::make_unique<DX12SwapChain>(*dxDev, std::move(desc));
    return swapChainDesc;
}
