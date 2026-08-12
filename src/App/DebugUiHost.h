#pragma once

#include "Win32AppShell.h"

namespace physics
{
    class JoltPhysicsWorld;
}

namespace appUi
{
    void InitializeImGui(appWin32::AppShellContext& shell, HWND hwnd, rhi::IRHIDevice& device, rhi::Format backbufferFormat, int backbufferCount);
    void ShutdownImGui(appWin32::AppShellContext& shell, rhi::IRHIDevice& device);

    const void* BuildImGuiFrameIfEnabled(
        appWin32::AppShellContext& shell,
        rhi::IRHIDevice& device,
        rendern::RendererSettings& settings,
        rendern::Scene& scene,
        rendern::CameraController& cameraController,
        rendern::LevelAsset& levelAsset,
        rendern::LevelInstance& levelInstance,
        AssetManager& assets,
        rendern::GameplayRuntimeMode& runtimeMode,
        rendern::GameplayRuntime* gameplayRuntime,
        physics::JoltPhysicsWorld* physicsWorld);

    rendern::InputCapture GetInputCaptureForImGui(const appWin32::AppShellContext& shell);
    void RenderImGuiToSwapChainIfEnabled(appWin32::AppShellContext& shell, rhi::IRHIDevice& device, rhi::IRHISwapChain& swapChain, const void* imguiDrawData);
}
