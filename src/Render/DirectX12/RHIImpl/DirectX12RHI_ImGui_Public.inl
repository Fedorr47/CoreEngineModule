void InitImGui(void* hwnd, int framesInFlight, rhi::Format rtvFormat) override
{
    if (imguiInitialized_)
    {
        return;
    }
    if (!hwnd)
    {
        throw std::runtime_error("DX12: InitImGui: hwnd is null");
    }

    const DXGI_FORMAT fmt = ToDXGIFormat(rtvFormat);

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = srvHeap_->GetCPUDescriptorHandleForHeapStart();
    cpu.ptr += static_cast<SIZE_T>(kImGuiFontSrvIndex) * static_cast<SIZE_T>(srvInc_);

    D3D12_GPU_DESCRIPTOR_HANDLE gpu = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    gpu.ptr += static_cast<UINT64>(kImGuiFontSrvIndex) * static_cast<UINT64>(srvInc_);

    // The ImGui Win32 backend is initialized in the app; here we only setup the DX12 backend.
    if (!ImGui_ImplDX12_Init(NativeDevice(), framesInFlight, fmt, srvHeap_.Get(), cpu, gpu))
    {
        throw std::runtime_error("DX12: ImGui_ImplDX12_Init failed");
    }

    imguiInitialized_ = true;
}

void ImGuiNewFrame() override
{
    if (imguiInitialized_)
    {
        ImGui_ImplDX12_NewFrame();
    }
}

void ShutdownImGui() override
{
    if (imguiInitialized_)
    {
        ImGui_ImplDX12_Shutdown();
        imguiInitialized_ = false;
    }
}

