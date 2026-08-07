#pragma once

#include <cstdint>
#include <cstdio>

#include "Win32AppShell.h"
#include "DebugUiHost.h"
#include "EditorViewportInteraction.h"
#include "AppRuntimeHelpers.h"
#include "AppBootstrap.h"

namespace physics
{
    class JoltRuntime;
    class JoltPhysicsWorld;
    class LevelPhysicsRuntime;
}

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
        rendern::GameplayRuntimeMode gameplayMode{ rendern::GameplayRuntimeMode::Editor };
    };
    
    struct CpuFrameTimings
    {
        double totalBeforeSleepMs = 0.0;
        double totalWithSleepMs = 0.0;

        double updateFrameTimingMs = 0.0;
        double inputMs = 0.0;
        double editorInteractionMs = 0.0;
        double gameplayAndAnimationMs = 0.0;
        double buildImGuiMs = 0.0;

        double renderMainViewportMs = 0.0;
        double renderDebugWindowMs = 0.0;
        double tinySleepMs = 0.0;
        double streamingMs = 0.0;
    };
    
    struct AppFrameState
    {
        GameTimer frameTimer{};
        GameTimer statsTimer{};
        LoadingOverlayState loadingOverlay{};
        FrameStatsOverlayState frameStatsOverlay{};
        CpuFrameTimings cpuFrameTimings{};
        std::uint64_t frameIndex = 0u;
    };
    
    struct AppPhysicsState
    {
        AppPhysicsState();
        ~AppPhysicsState();

        AppPhysicsState(const AppPhysicsState&) = delete;
        AppPhysicsState& operator=(const AppPhysicsState&) = delete;
        AppPhysicsState(AppPhysicsState&&) = delete;
        AppPhysicsState& operator=(AppPhysicsState&&) = delete;
        
        void Initialize();
        void Shutdown() noexcept;

        std::unique_ptr<physics::JoltRuntime> joltRuntime;
        std::unique_ptr<physics::JoltPhysicsWorld> joltPhysicsWorld;
        std::unique_ptr<physics::LevelPhysicsRuntime> levelPhysicsRuntime;
    };
    
    struct AppState
    {
        AppConfig           config{};
        AppLaunchState      launchState{};
        AppWindowState      windowState{};
        AppGraphicsState    graphicsState{};
        AppContentState     contentState{};
        AppPhysicsState     physicsState{};
        AppRuntimeState     runtimeState{};
        AppFrameState       frameState{};
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
    [[nodiscard]] bool SetGameplayMode(AppState& app, rendern::GameplayRuntimeMode mode, std::string& errorMessage);
    void ShutdownApp(AppState& app);
}
