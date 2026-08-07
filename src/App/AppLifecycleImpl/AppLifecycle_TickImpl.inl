static bool PumpAndCheckRunning(AppState& app)
{
    appWin32::PumpMessages(app.windowState.mainWindow);
    if (!app.windowState.mainWindow.running)
    {
        return false;
    }
    return true;
}

static void ApplyPendingResize(AppState& app)
{
    appRuntime::ApplyPendingResize(app.windowState.mainWindow, app.graphicsState.swapChain.get());
#if defined(CORE_USE_DX12)
    appRuntime::ApplyPendingResize(app.windowState.debugWindow, app.graphicsState.debugSwapChain.get());
#endif
}

static bool ShouldSkipFrame(AppState& app)
{
    if (appRuntime::ShouldSkipMainViewportFrame(app.windowState.mainWindow))
    {
        appWin32::TinySleep();
        return true;
    }
    return false;
}

static float UpdateFrameTimingAndLoadingOverlay(AppState& app)
{
    app.frameState.frameTimer.Tick();

    const double rawFrameDeltaSeconds = app.frameState.frameTimer.GetDeltaTime();
    const float rawFrameTimeMs = static_cast<float>(rawFrameDeltaSeconds * 1000.0);
    app.graphicsState.rendererSettings.performanceSnapshot.rawFrameTimeMs = rawFrameTimeMs;
    const float deltaSeconds = static_cast<float>(rawFrameDeltaSeconds);

    FrameStatsOverlayState& frameStats = app.frameState.frameStatsOverlay;

    if (rawFrameDeltaSeconds > 0.0 && std::isfinite(rawFrameDeltaSeconds))
    {
        frameStats.accumulatedSeconds += rawFrameDeltaSeconds;
        ++frameStats.accumulatedFrames;

        if (frameStats.accumulatedSeconds >= 0.25 || frameStats.accumulatedFrames >= 32u)
        {
            const double sampleSeconds = std::max(frameStats.accumulatedSeconds, 1e-6);
            const float sampleMs =
                static_cast<float>((sampleSeconds * 1000.0) / static_cast<double>(frameStats.accumulatedFrames));

            if (!frameStats.initialized ||
                !std::isfinite(frameStats.displayMs) ||
                frameStats.displayMs <= 0.0f)
            {
                frameStats.displayMs = sampleMs;
                frameStats.initialized = true;
            }
            else
            {
                constexpr float kStatsBlend = 0.35f;
                frameStats.displayMs = std::lerp(frameStats.displayMs, sampleMs, kStatsBlend);
            }

            frameStats.displayFps = frameStats.displayMs > 0.0f
                ? 1000.0f / frameStats.displayMs
                : 0.0f;

            frameStats.accumulatedSeconds = 0.0;
            frameStats.accumulatedFrames = 0u;
        }
    }

    app.graphicsState.rendererSettings.performanceSnapshot.fps = frameStats.displayFps;
    app.graphicsState.rendererSettings.performanceSnapshot.frameTimeMs = frameStats.displayMs;

    const AssetStreamingStats streamingStats = app.contentState.assets->GetStreamingStats();
    const bool hasPendingStreaming = streamingStats.HasPendingWork();
    const float targetProgress01 = streamingStats.Completion01();

    auto& overlay = app.frameState.loadingOverlay;
    const float lerpAlpha = std::clamp(deltaSeconds * (hasPendingStreaming ? 4.0f : 10.0f), 0.0f, 1.0f);
    overlay.displayProgressBar = std::lerp(overlay.displayProgressBar, targetProgress01, lerpAlpha);

    if (hasPendingStreaming)
    {
        overlay.visible = true;
        overlay.completedHoldSeconds = 0.0f;
    }
    else
    {
        overlay.displayProgressBar = std::max(overlay.displayProgressBar, 1.0f);
        overlay.completedHoldSeconds += deltaSeconds;
        if (overlay.completedHoldSeconds >= 0.35f)
        {
            overlay.visible = false;
        }
    }

    app.graphicsState.rendererSettings.loadingOverlayVisible = overlay.visible;
    app.graphicsState.rendererSettings.loadingOverlayProgressBar = overlay.visible
        ? std::clamp(overlay.displayProgressBar, hasPendingStreaming ? 0.02f : 1.0f, 1.0f)
        : 0.0f;

    app.graphicsState.rendererSettings.loadingOverlayTotalUnits = streamingStats.total.totalEntries;
    app.graphicsState.rendererSettings.loadingOverlayCompletedUnits = streamingStats.total.loadedEntries + streamingStats.total.failedEntries;
    
    return deltaSeconds;
}

static void UpdateInputAndCamera(AppState& app, float deltaSeconds)
{
    app.windowState.input.SetCaptureMode(appUi::GetInputCaptureForImGui(app.windowState.shell));
    app.windowState.input.NewFrame(app.windowState.mainWindow.hwnd);
    if (app.runtimeState.gameplayMode == rendern::GameplayRuntimeMode::Editor)
    {
        app.runtimeState.cameraController->Update(
            deltaSeconds, 
            app.windowState.input.State(), 
            app.runtimeState.scene.camera);
    }
}

static void UpdateEditorViewportInteraction(AppState& app)
{
   
    if (app.windowState.input.State().KeyPressed(VK_F5))
    {
        const rendern::GameplayRuntimeMode requestedMode =
            (app.runtimeState.gameplayMode == rendern::GameplayRuntimeMode::Editor)
            ? rendern::GameplayRuntimeMode::Game
            : rendern::GameplayRuntimeMode::Editor;
        
        std::string error;
        if (!SetGameplayMode(app, requestedMode, error))
        {
            std::cerr << "[Physics] Failed to change runtime mode: " << error << '\n';
        }
    }
    
    if (app.windowState.input.State().KeyPressed(VK_F6))
    {
        app.graphicsState.rendererSettings.drawAnimationRuntimeOverlay = 
            !app.graphicsState.rendererSettings.drawAnimationRuntimeOverlay;
    }

    if (app.windowState.input.State().KeyPressed(VK_F7))
    {
        app.graphicsState.rendererSettings.enableDebugWindowRender =
            !app.graphicsState.rendererSettings.enableDebugWindowRender;
    }
    if (app.runtimeState.gameplayMode == rendern::GameplayRuntimeMode::Game)
    {
        ResetEditorInteractionState(app);
    }
    else
    {
        appEditor::ApplyGizmoModeHotkeys(
            app.runtimeState.editorViewportInteraction, 
            app.runtimeState.scene, 
            app.windowState.input.State());
        appEditor::SyncEditorGizmoVisuals(
            app.runtimeState.editorViewportInteraction, 
            *app.contentState.levelAsset, 
            *app.runtimeState.levelInstance,
            app.runtimeState.scene);
        appEditor::UpdateViewportGizmoHover(
            app.runtimeState.editorViewportInteraction, 
            app.windowState.mainWindow.hwnd, 
            app.windowState.mainWindow.width, 
            app.windowState.mainWindow.height, 
            app.runtimeState.scene, 
            app.windowState.input.State());
        appEditor::HandleViewportMouseInteraction(
            app.runtimeState.editorViewportInteraction, 
            app.windowState.mainWindow.hwnd, 
            app.windowState.mainWindow.width, 
            app.windowState.mainWindow.height, 
            *app.contentState.levelAsset, 
            *app.runtimeState.levelInstance, 
            *app.contentState.assets, 
            app.runtimeState.scene, 
            app.windowState.input.State());
    }
}

const rendern::GameplayUpdateContext BuildGameplayUpdateContext(
    AppState& app, 
    float deltaSeconds)
{
    rendern::GameplayUpdateContext gameplayCtx{};
    gameplayCtx.deltaSeconds = deltaSeconds;
    gameplayCtx.mode = app.runtimeState.gameplayMode;
    gameplayCtx.input = &app.windowState.input.State();
    gameplayCtx.levelAsset = app.contentState.levelAsset.get();
    gameplayCtx.levelInstance = app.runtimeState.levelInstance.get();
    gameplayCtx.scene = &app.runtimeState.scene;
    return gameplayCtx;
}

static void UpdateGameplayAndAnimation(AppState& app, float deltaSeconds)
{
    CORE_ASSERT_RUNTIME_THREAD();

    const rendern::GameplayUpdateContext gameplayCtx = BuildGameplayUpdateContext(app, deltaSeconds);
    auto* gameplayRuntime = app.runtimeState.gameplayRuntime.get();
    
    if (gameplayRuntime)
    {
        app.runtimeState.gameplayRuntime->BeginFrame();
        app.runtimeState.gameplayRuntime->PreAnimationUpdate(gameplayCtx);
    }

    app.runtimeState.scene.UpdateSkinned(deltaSeconds);

    if (app.runtimeState.gameplayRuntime)
    {
        app.runtimeState.gameplayRuntime->PostAnimationUpdate(gameplayCtx);
    }
    
    UpdateGameplayMovementDebug(app);
    UpdateAnimationRuntimeDebug(app);
    app.runtimeState.scene.UpdateParticles(deltaSeconds);
}

static void UpdatePhysics(AppState& app, const float deltaSeconds)
{
    CORE_ASSERT_PHYSICS_THREAD();

    auto* physicsWorld = app.physicsState.joltPhysicsWorld.get();
    if (physicsWorld == nullptr
        || app.runtimeState.gameplayMode != rendern::GameplayRuntimeMode::Game)
    {
        return;
    }

    physicsWorld->Update(deltaSeconds);
    std::string error;
    if (!app.physicsState.levelPhysicsRuntime->Synchronize(
        *app.contentState.levelAsset,
        *app.runtimeState.levelInstance,
        app.runtimeState.scene,
        error))
    {
        std::cerr << "[Physics] " << error << '\n';
    }
}

static void RenderMainViewport(AppState& app)
{
    CORE_ASSERT_RENDER_THREAD();

    app.graphicsState.renderer->SetSettings(app.graphicsState.rendererSettings);
    app.graphicsState.renderer->RenderFrame(
        *app.graphicsState.swapChain, rendern::RenderSceneExtractor::BuildFrameView(app.runtimeState.scene), /*imguiDrawData=*/nullptr);
}

static void RenderDebugWindowIfNeeded(AppState& app, const void* imguiDrawData)
{
    CORE_ASSERT_RENDER_THREAD();

#if defined(CORE_USE_DX12)
    if (appRuntime::CanRenderDebugSwapChain(app.windowState.debugWindow, app.graphicsState.debugSwapChain.get()))
    {
        app.graphicsState.debugSwapChain->SetVSyncEnabled(app.graphicsState.rendererSettings.enableVSync);
        appUi::RenderImGuiToSwapChainIfEnabled(
            app.windowState.shell,
            *app.graphicsState.device, *app.graphicsState.debugSwapChain, imguiDrawData);
    }
#endif
}
