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
#include "GameplayPhysicsCharacterIntegration.h"

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
    
    [[nodiscard]] const char* ToAIActionExecutionStatusText(
            const rendern::AIActionExecutionStatus status) noexcept
    {
        switch (status)
        {
        case rendern::AIActionExecutionStatus::Running:
            return "Running";

        case rendern::AIActionExecutionStatus::Succeeded:
            return "Succeeded";

        case rendern::AIActionExecutionStatus::Failed:
            return "Failed";

        case rendern::AIActionExecutionStatus::Cancelled:
            return "Cancelled";

        case rendern::AIActionExecutionStatus::NotStarted:
        default:
            return "NotStarted";
        }
    }
    
    void DrawGameplayAIMovementDevelopmentControls(
            rendern::GameplayRuntime& gameplayRuntime,
            rendern::LevelAsset& level,
            rendern::LevelInstance& levelInstance,
            rendern::Scene& scene,
            physics::JoltPhysicsWorld* physicsWorld)
    {
        ImGui::SeparatorText("AI Movement");

        const rendern::GameplayRuntimeMode currentMode =
            gameplayRuntime.GetCurrentMode();

        const bool bIsGameMode =
            currentMode ==
            rendern::GameplayRuntimeMode::Game;
        
        if (rendern::IsGameplayAIStepDebugScenario(level))
        {
            ImGui::TextUnformatted("AI Physics Step-Up Debug");
            ImGui::BeginDisabled(!bIsGameMode);
            if (ImGui::Button("Start Route"))
            {
                rendern::GameplayUpdateContext context{};
                context.mode = currentMode;
                context.levelAsset = &level;
                context.levelInstance = &levelInstance;
                context.scene = &scene;
                (void)rendern::StartGameplayAIStepDebugRoute(gameplayRuntime, context);
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset NPC"))
            {
                const rendern::EntityHandle entity =
                    rendern::ResetGameplayAIStepDebugNPC(gameplayRuntime, level);
                if (physicsWorld != nullptr && entity != rendern::kNullEntity)
                {
                    (void)appRuntime::TeleportGameplayPhysicsCharacterToGameplayTransform(
                        gameplayRuntime, *physicsWorld, entity);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop Route"))
            {
                rendern::CancelGameplayAIStepDebugRoute(gameplayRuntime, level);
            }
            ImGui::EndDisabled();
            if (!bIsGameMode)
            {
                ImGui::TextDisabled("Enter Game mode to control the route.");
            }
            return;
        }

        ImGui::Text(
            "Actual runtime mode: %s",
            bIsGameMode
                ? "Game"
                : "Editor");

        const rendern::AIActionExecutionStatus actionStatus =
            rendern::
                GetGameplayAIMovementDevelopmentScenarioStatus(
                    gameplayRuntime,
                    level);

        ImGui::Text(
            "Route status: %s",
            ToAIActionExecutionStatusText(actionStatus));

        ImGui::BeginDisabled(!bIsGameMode);

        if (ImGui::Button("Start / Restart AI Route"))
        {
            rendern::GameplayUpdateContext context{};
            context.mode = currentMode;
            context.levelAsset = &level;
            context.levelInstance = &levelInstance;
            context.scene = &scene;

            (void)rendern::
                StartGameplayAIMovementDevelopmentScenario(
                    gameplayRuntime,
                    context);
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel AI Route"))
        {
            rendern::
                CancelGameplayAIMovementDevelopmentScenario(
                    gameplayRuntime,
                    level);
        }

        ImGui::EndDisabled();

        if (!bIsGameMode)
        {
            ImGui::TextDisabled(
                "Enter Game mode to control the route.");
        }

        ImGui::TextUnformatted(
            "Scenario nodes: AI_Move_Agent and at least two "
            "AI_Move_Point_<number> nodes.");
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
        rendern::GameplayRuntime* gameplayRuntime,
        physics::JoltPhysicsWorld* physicsWorld)
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

        const bool bIsGameModeRequested =
            runtimeMode == rendern::GameplayRuntimeMode::Game;

        ImGui::TextUnformatted(
            bIsGameModeRequested
                ? "Mode: Game"
                : "Mode: Editor");

        if (ImGui::Button(
        bIsGameModeRequested
            ? "Return to Editor Mode"
            : "Enter Game Mode (F5)"))
        {
            shell.requestedGameplayModeToggle = true;
        }

        if (gameplayRuntime != nullptr)
        {
            DrawGameplayAIMovementDevelopmentControls(
                *gameplayRuntime,
                levelAsset,
                levelInstance,
                scene,
                physicsWorld);
        }

        ImGui::End();

        //if (runtimeMode == rendern::GameplayRuntimeMode::Editor)
        //{
            rendern::ui::DrawLevelEditorUI(levelAsset, levelInstance, assets, scene, cameraController, gameplayRuntime);
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
        rendern::GameplayRuntime*,
        physics::JoltPhysicsWorld*)
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
