#pragma once

#include "Win32AppShell.h"

namespace appBootstrap
{
    rhi::Backend ParseBackendFromArgs(int argc, char** argv);
    bool CanUseDebugWindow([[maybe_unused]] rhi::Backend backend);

    void CreatePrimaryWindowSet(
        int mainWidth,
        int mainHeight,
        const std::wstring& mainTitle,
        bool canUseDebugWindow,
        appWin32::Win32Window& outMainWindow
#if defined(CORE_USE_DX12)
        , appWin32::Win32Window* outDebugWindow
#endif
    );

    void BindWin32Input(rendern::Win32Input& input);

    void CreateDeviceAndSwapChain(
        rhi::Backend backend,
        HWND hwnd,
        int initialWidth,
        int initialHeight,
        std::unique_ptr<rhi::IRHIDevice>& outDevice,
        std::unique_ptr<rhi::IRHISwapChain>& outSwapChain);

#if defined(CORE_USE_DX12)
    void CreateDebugSwapChainIfNeeded(
        rhi::Backend backend,
        rhi::IRHIDevice& device,
        const appWin32::Win32Window& debugWindow,
        std::unique_ptr<rhi::IRHISwapChain>& outDebugSwapChain);
#endif

    std::unique_ptr<ITextureUploader> CreateTextureUploader(rhi::Backend backend, rhi::IRHIDevice& device);
}
