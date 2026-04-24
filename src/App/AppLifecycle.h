#pragma once

#include <cstdint>

#include "Win32AppShell.h"
#include "DebugUiHost.h"
#include "EditorViewportInteraction.h"
#include "AppRuntimeHelpers.h"
#include "AppBootstrap.h"
#include "DebugTools/SphereCcdSandbox.h"

namespace appLifecycle
{
    constexpr std::string_view DefaultStartupLevelName = "levels/demo.level.with_fsm_test.locomotion.phaseB.json";
    
    struct LoadingOverlayState
    {
        bool visible = true;
        float displayProgressBar = 0.0f;
        float completedHoldSeconds = 0.0f;
    };

    struct FrameStatsOverlayState
    {
        double accumulatedSeconds = 0.0;
        std::uint32_t accumulatedFrames = 0u;
        float displayFps = 0.0f;
        float displayMs = 0.0f;
        bool initialized = false;
    };

    struct AppConfig
    {
        int windowWidth = 1280;
        int windowHeight = 1024;
        std::wstring windowTitle = L"CoreEngineModule (DX12)";
        appRuntime::UploadBudget uploadBudget{};
    };
    
    struct AppLaunchState
    {
        rhi::Backend requestedBackend = rhi::Backend::DirectX12;
        bool canUseDebugWindow = false;
        std::map<std::string, std::vector<std::string>> appArguments{};
        std::string currentLevelName{};
    };
    
    struct AppWindowState
    {
        appWin32::AppShellContext shell{};
        appWin32::Win32Window mainWindow{};
#if defined(CORE_USE_DX12)
        appWin32::Win32Window debugWindow{};
#endif
        rendern::Win32Input input{};

        AppWindowState() = default;
        AppWindowState(const AppWindowState&) = delete;
        AppWindowState& operator=(const AppWindowState&) = delete;
        AppWindowState(AppWindowState&&) = delete;
        AppWindowState& operator=(AppWindowState&&) = delete;
    };
    
    struct AppGraphicsState
    {
        std::unique_ptr<rhi::IRHIDevice> device;
        std::unique_ptr<rhi::IRHISwapChain> swapChain;
#if defined(CORE_USE_DX12)
        std::unique_ptr<rhi::IRHISwapChain> debugSwapChain;
#endif
        rendern::RendererSettings rendererSettings{};
        std::unique_ptr<rendern::Renderer> renderer;
        std::unique_ptr<rendern::BindlessTable> bindless;
    };
    
    struct AppContentState
    {
        StbTextureDecoder textureDecoder{};
        std::unique_ptr<rendern::JobSystemThreadPool> jobSystem;
        rendern::RenderQueueImmediate renderQueue{};
        std::unique_ptr<ITextureUploader> textureUploader;
        std::unique_ptr<TextureIO> textureIO;
        std::unique_ptr<rendern::MeshIO> meshIO;
        std::unique_ptr<AssetManager> assets;
        std::unique_ptr<rendern::LevelAsset> levelAsset;
    };
    
    struct AppRuntimeState
    {
        rendern::Scene scene{};
        std::unique_ptr<rendern::LevelInstance> levelInstance;
        std::unique_ptr<rendern::GameplayRuntime> gameplayRuntime;
        std::unique_ptr<rendern::CameraController> cameraController;
        appEditor::EditorViewportInteraction editorViewportInteraction{};
        appDebugTools::SphereCcdSandboxState sphereCcdSandbox{};
        rendern::GameplayRuntimeMode gameplayMode{ rendern::GameplayRuntimeMode::Editor };
    };
    
    struct AppFrameState
    {
        GameTimer frameTimer{};
        GameTimer statsTimer{};
        LoadingOverlayState loadingOverlay{};
        FrameStatsOverlayState frameStatsOverlay{};
    };
    
    struct AppState
    {
        AppConfig           config{};
        AppLaunchState      launchState{};
        AppWindowState      windowState{};
        AppGraphicsState    graphicsState{};
        AppContentState     contentState{};
        AppRuntimeState     runtimeState{};
        AppFrameState      frameState{};
        bool initialized = false;
        
        std::thread::id mainThreadId;

        AppState() = default;
        AppState(const AppState&) = delete;
        AppState& operator=(const AppState&) = delete;
        AppState(AppState&&) = delete;
        AppState& operator=(AppState&&) = delete;
    };

    void InitializeApp(AppState& app, int argc, char** argv);
    bool TickApp(AppState& app);
    void ShutdownApp(AppState& app);
}
