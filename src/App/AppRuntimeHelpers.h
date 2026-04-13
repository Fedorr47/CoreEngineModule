#pragma once

#include "Win32AppShell.h"
#include "DebugUiHost.h"

namespace appRuntime
{
    struct UploadBudget
    {
        int maxTextureUploadsPerFrame = 8;
        int maxMeshUploadsPerFrame = 32;
        int maxTextureDeletesPerFrame = 2;
        int maxMeshDeletesPerFrame = 32;
    };

    inline void ApplyPendingResize(appWin32::Win32Window& window, rhi::IRHISwapChain* swapChain)
    {
        if (!swapChain || !window.pendingResize)
        {
            return;
        }

        window.pendingResize = false;
        if (window.pendingWidth <= 0 || window.pendingHeight <= 0)
        {
            return;
        }

        swapChain->Resize(rhi::Extent2D{
            static_cast<std::uint32_t>(window.pendingWidth),
            static_cast<std::uint32_t>(window.pendingHeight)
            });
    }

    inline bool ShouldSkipMainViewportFrame(const appWin32::Win32Window& window)
    {
        return window.minimized || window.width <= 0 || window.height <= 0;
    }

    inline void DriveAssetStreaming(
        AssetManager& assets,
        rendern::LevelInstance& levelInstance,
        rendern::BindlessTable& bindless,
        rendern::Scene& scene,
        const UploadBudget& budget)
    {
        assets.ProcessUploads(
            budget.maxTextureUploadsPerFrame,
            budget.maxTextureDeletesPerFrame,
            budget.maxMeshUploadsPerFrame,
            budget.maxMeshDeletesPerFrame);

        levelInstance.ResolveTextureBindings(assets, bindless, scene);
    }

    inline bool CanRenderDebugSwapChain(const appWin32::Win32Window& debugWindow, const rhi::IRHISwapChain* debugSwapChain)
    {
        return debugSwapChain
            && debugWindow.hwnd
            && !debugWindow.minimized
            && debugWindow.width > 0
            && debugWindow.height > 0;
    }

    inline void ShutdownRuntime(
        appWin32::AppShellContext& shell,
        rhi::IRHIDevice& device,
        rendern::Renderer& renderer,
        rendern::LevelInstance& levelInstance,
        rendern::BindlessTable& bindless,
        rendern::JobSystemThreadPool& jobSystem,
        AssetManager& assets
    )
    {
        appWin32::Win32Window* mainWindow = shell.mainWindow;
        appWin32::Win32Window* debugWindow = shell.debugWindow;

#if defined(CORE_USE_DX12)
        appUi::ShutdownImGui(shell, device);
#endif

        renderer.Shutdown();
        levelInstance.FreeDescriptors(bindless);

        jobSystem.WaitIdle();
        assets.ClearAll();
        assets.ProcessUploads(64, 256, 64, 256);

        if (mainWindow && mainWindow->hwnd)
        {
            appWin32::DestroyWindowSafe(mainWindow->hwnd);
        }

#if defined(CORE_USE_DX12)
        if (debugWindow && debugWindow->hwnd)
        {
            appWin32::DestroyWindowSafe(debugWindow->hwnd);
        }
#endif

        if (mainWindow)
        {
            *mainWindow = {};
        }

#if defined(CORE_USE_DX12)
        if (debugWindow)
        {
            *debugWindow = {};
        }
#endif

        shell = {};
    }
}
