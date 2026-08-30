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
#include "Development/AppDevelopmentScenarioRuntime.h"

#if defined(CORE_USE_DX12)
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#endif

namespace appUi
{
#if defined(CORE_USE_DX12)
    const std::string& GetImGuiLayoutPath()
    {
        static const std::string path = []
        {
            const std::filesystem::path directory =
                std::filesystem::path("Saved") / "Editor";

            std::filesystem::create_directories(directory);

            return (directory / "imgui_layout.ini").string();
        }();

        return path;
    }
    
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

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // ImGui keeps this pointer for the lifetime of the context,
        // so the backing string must have stable storage.
        io.IniFilename = GetImGuiLayoutPath().c_str();

        ImGui_ImplWin32_Init(hwnd);
        device.InitImGui(hwnd, backbufferCount, backbufferFormat);

        shell.imguiInitialized = true;
    }

    void ShutdownImGui(appWin32::AppShellContext& shell, rhi::IRHIDevice& device)
    {
        if (!shell.imguiInitialized)
        {
            return;
        }
        
        const ImGuiIO& io = ImGui::GetIO();
        if (io.IniFilename != nullptr)
        {
            ImGui::SaveIniSettingsToDisk(io.IniFilename);
        }
        
        device.ShutdownImGui();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        shell.imguiInitialized = false;
    }
    
    void DrawDevelopmentScenarioControls(
        appDevelopment::AppDevelopmentScenarioRuntime& runtime,
        appDevelopment::ScenarioContext& context)
    {
        const appDevelopment::ScenarioView view = runtime.GetView(context);
        if (!view.active)
        {
            return;
        }
    
        ImGui::SeparatorText("Development Scenario");
        ImGui::TextUnformatted(view.title);
        if (view.description[0] != '\0')
        {
            ImGui::TextUnformatted(view.description);
        }

        for (std::size_t index = 0; index < view.statusCount; ++index)
        {
            ImGui::Text(
            "%s: %s",
                view.statuses[index].label,
                view.statuses[index].value);
        }
        
        const auto& steeringStates = context.gameplayRuntime.GetSteeringDebugRegistry().States();
        if (!steeringStates.empty())
        {
            const auto sideLabel = [](rendern::GameplayObstacleAvoidanceSide value)
            {
                switch (value)
                {
                case rendern::GameplayObstacleAvoidanceSide::Left: return "Left";
                case rendern::GameplayObstacleAvoidanceSide::Right: return "Right";
                default: return "None";
                }
            };
            const auto drawClearance = [](const char* label,
                const rendern::GameplayObstacleProbeDebugState& probe,
                const bool clearAsText)
            {
                if (!probe.queried)
                {
                    ImGui::Text("%s: N/A", label);
                }
                else if (clearAsText && !probe.hit)
                {
                    ImGui::Text("%s: Clear", label);
                }
                else
                {
                    ImGui::Text("%s: %.2f", label, probe.clearance);
                }
            };
            ImGui::SeparatorText("Steering Debug");
            constexpr std::array orderedModes{
                std::pair{rendern::GameplaySteeringDebugMode::Follow, "Follow"},
                std::pair{rendern::GameplaySteeringDebugMode::Flee, "Flee"},
                std::pair{rendern::GameplaySteeringDebugMode::Route, "Route"}};
            for (const auto& [mode, label] : orderedModes)
            {
                const rendern::GameplaySteeringDebugState* selected = nullptr;
                rendern::EntityHandle selectedAgent = rendern::kNullEntity;
                for (const auto& [agent, state] : steeringStates)
                {
                    if (state.mode == mode &&
                        (selected == nullptr || agent < selectedAgent))
                    {
                        selected = &state;
                        selectedAgent = agent;
                    }
                }
                if (selected == nullptr)
                {
                    continue;
                }

                const auto& avoidance = selected->avoidance;
                ImGui::Text("[%s]", label);
                ImGui::Text("Avoidance: %s", avoidance.active ? "Active" : "Inactive");
                ImGui::Text("Chosen side: %s", sideLabel(avoidance.chosenSide));
                drawClearance("Forward hit", avoidance.forward, true);
                drawClearance("Left clearance", avoidance.left, false);
                drawClearance("Right clearance", avoidance.right, false);
                ImGui::Text("Move magnitude: %.2f", avoidance.finalMovement.moveMagnitude);
            }
        }

        ImGui::BeginDisabled(!view.commandsEnabled);
        if (view.canStart && ImGui::Button(view.startLabel))
        {
            runtime.Execute(appDevelopment::ScenarioCommand::Start, context);
        }
        if (view.canReset)
        {
            if (view.canStart) ImGui::SameLine();
            if (ImGui::Button(view.resetLabel))
                runtime.Execute(appDevelopment::ScenarioCommand::Reset, context);
        }
        if (view.canStop)
        {
            if (view.canStart || view.canReset) ImGui::SameLine();
            if (ImGui::Button(view.stopLabel))
                runtime.Execute(appDevelopment::ScenarioCommand::Stop, context);
        }
        
        ImGui::EndDisabled();

        if (!view.commandsEnabled && context.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
            ImGui::TextDisabled("Enter Game mode to control the scenario.");
        }
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
        appDevelopment::AppDevelopmentScenarioRuntime* developmentScenarioRuntime,
        appDevelopment::ScenarioContext* developmentScenarioContext)
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

        if (developmentScenarioRuntime != nullptr && developmentScenarioContext != nullptr)
        {
            DrawDevelopmentScenarioControls(
                *developmentScenarioRuntime, *developmentScenarioContext);
        }

        ImGui::End();
        
        rendern::ui::DrawLevelEditorUI(levelAsset, levelInstance, assets, scene, cameraController);
        rendern::ui::DrawAnimationDebugUI(levelAsset, levelInstance, scene, gameplayRuntime);
        rendern::ui::DrawInputBindingsUI(gameplayRuntime);
        rendern::ui::DrawGOAPDebugUI(gameplayRuntime);

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
        appDevelopment::AppDevelopmentScenarioRuntime*,
        appDevelopment::ScenarioContext*)
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
