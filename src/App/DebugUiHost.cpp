import core;
import std;

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "DebugUiHost.h"

#if defined(CORE_USE_DX12)
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#endif

namespace appUi
{
#if defined(CORE_USE_DX12)
    void InitializeImGui(
        appWin32::AppShellContext& shell,
        HWND hwnd,
        rhi::IRHIDevice& device,
        rhi::Format backbufferFormat,
        int backbufferCount)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(hwnd);
        device.InitImGui(hwnd, backbufferCount, backbufferFormat);
        shell.imguiInitialized = true;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void ShutdownImGui(appWin32::AppShellContext& shell, rhi::IRHIDevice& device)
    {
        if (!shell.imguiInitialized)
        {
            return;
        }

        device.ShutdownImGui();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        shell.imguiInitialized = false;
    }

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
        appDebugTools::SphereCcdSandboxState& sphereCcdSandboxState)
    {
        if (!shell.imguiInitialized || !shell.showDebugWindow || !shell.debugWindow || !shell.debugWindow->hwnd)
        {
            return nullptr;
        }

        if (!IsWindowVisible(shell.debugWindow->hwnd))
        {
            return nullptr;
        }

        device.ImGuiNewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        rendern::ui::BeginDebugDockSpace();
        rendern::ui::DrawRendererDebugUI(settings, scene, cameraController);

        ImGui::Begin("App Runtime");
        const bool inGameMode = runtimeMode == rendern::GameplayRuntimeMode::Game;
        ImGui::TextUnformatted(inGameMode ? "Mode: Game" : "Mode: Editor");
        if (ImGui::Button(inGameMode ? "Return to Editor Mode" : "Enter Game Mode (F5)"))
        {
            runtimeMode = inGameMode ? rendern::GameplayRuntimeMode::Editor : rendern::GameplayRuntimeMode::Game;
        }
        ImGui::End();

        appDebugTools::DrawSphereCcdSandboxUi(sphereCcdSandboxState);

        //if (runtimeMode == rendern::GameplayRuntimeMode::Editor)
        //{
            rendern::ui::DrawLevelEditorUI(levelAsset, levelInstance, assets, scene, cameraController);
        //}
        /*
        else
        {
            ImGui::Begin("Level Editor");
            ImGui::TextUnformatted("Level editor interaction is disabled in Game mode.");
            ImGui::TextUnformatted("Return to Editor mode to use gizmos, selection and viewport editing.");
            ImGui::End();
        }
        */

        ImGui::Render();
        return static_cast<const void*>(ImGui::GetDrawData());
    }

    rendern::InputCapture GetInputCaptureForImGui(const appWin32::AppShellContext& shell)
    {
        rendern::InputCapture capture{};
        if (shell.imguiInitialized && shell.showDebugWindow && shell.debugWindow && shell.debugWindow->hwnd)
        {
            if (IsWindowVisible(shell.debugWindow->hwnd) && GetForegroundWindow() == shell.debugWindow->hwnd)
            {
                const ImGuiIO& io = ImGui::GetIO();
                capture.captureKeyboard = io.WantCaptureKeyboard;
                capture.captureMouse = io.WantCaptureMouse;
            }
        }
        return capture;
    }

    void RenderImGuiToSwapChainIfEnabled(
        appWin32::AppShellContext& shell,
        rhi::IRHIDevice& device,
        rhi::IRHISwapChain& swapChain,
        const void* imguiDrawData)
    {
        if (!imguiDrawData || !shell.imguiInitialized || !shell.showDebugWindow || !shell.debugWindow || !shell.debugWindow->hwnd)
        {
            return;
        }
        if (!IsWindowVisible(shell.debugWindow->hwnd))
        {
            return;
        }

        const rhi::Extent2D extent = swapChain.GetDesc().extent;

        rhi::CommandList cmd{};

        rhi::BeginPassDesc begin{};
        begin.frameBuffer = swapChain.GetCurrentBackBuffer();
        begin.extent = extent;
        begin.swapChain = &swapChain;
        begin.clearDesc.clearColor = true;
        begin.clearDesc.clearDepth = false;
        begin.clearDesc.color = { 0.08f, 0.08f, 0.08f, 1.0f };

        cmd.BeginPass(begin);
        cmd.SetViewport(0, 0, static_cast<int>(extent.width), static_cast<int>(extent.height));
        cmd.DX12ImGuiRender(imguiDrawData);
        cmd.EndPass();

        device.SubmitCommandList(std::move(cmd));
        swapChain.Present();
    }
#else
    void InitializeImGui(appWin32::AppShellContext&, HWND, rhi::IRHIDevice&, rhi::Format, int)
    {
    }

    void ShutdownImGui(appWin32::AppShellContext&, rhi::IRHIDevice&)
    {
    }

    const void* BuildImGuiFrameIfEnabled(
        appWin32::AppShellContext&,
        rhi::IRHIDevice&,
        rendern::RendererSettings&,
        rendern::Scene&,
        rendern::CameraController&,
        rendern::LevelAsset&,
        rendern::LevelInstance&,
        AssetManager&,
        rendern::GameplayRuntimeMode&,
        appDebugTools::SphereCcdSandboxState&)
    {
        return nullptr;
    }

    rendern::InputCapture GetInputCaptureForImGui(const appWin32::AppShellContext&)
    {
        return {};
    }

    void RenderImGuiToSwapChainIfEnabled(appWin32::AppShellContext&, rhi::IRHIDevice&, rhi::IRHISwapChain&, const void*)
    {
    }
#endif
}
