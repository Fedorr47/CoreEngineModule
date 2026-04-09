static bool PumpAndCheckRunning(AppState& app)
{
    appWin32::PumpMessages(app.window);
    if (!app.window.running)
    {
        return false;
    }
    return true;
}

static void ApplyPendingResize(AppState& app)
{
    appRuntime::ApplyPendingResize(app.window, app.swapChain.get());
#if defined(CORE_USE_DX12)
    appRuntime::ApplyPendingResize(app.debugWindow, app.debugSwapChain.get());
#endif
}

static bool ShouldSkipFrame(AppState& app)
{
    if (appRuntime::ShouldSkipMainViewportFrame(app.window))
    {
        appWin32::TinySleep();
        return true;
    }
    return false;
}

static const float UpdateFrameTimingAndLoadingOverlay(AppState& app)
{
    app.statsTimer.Tick();
    const double rawFrameDeltaSeconds = app.statsTimer.GetDeltaTime();

    app.frameTimer.Tick();
    const float deltaSeconds = static_cast<float>(app.frameTimer.GetDeltaTime());

    FrameStatsOverlayState& frameStats = app.frameStatsOverlay;
    if (rawFrameDeltaSeconds > 0.0)
    {
        frameStats.accumulatedSeconds += rawFrameDeltaSeconds;
        ++frameStats.accumulatedFrames;

        if (!frameStats.initialized)
        {
            frameStats.displayFps = static_cast<float>(1.0 / rawFrameDeltaSeconds);
            frameStats.displayMs = static_cast<float>(rawFrameDeltaSeconds * 1000.0);
            frameStats.initialized = true;
        }

        if (frameStats.accumulatedSeconds >= 0.25 || frameStats.accumulatedFrames >= 32u)
        {
            const double sampleSeconds = std::max(frameStats.accumulatedSeconds, 1e-6);
            const float sampleFps = static_cast<float>(static_cast<double>(frameStats.accumulatedFrames) / sampleSeconds);
            const float sampleMs = static_cast<float>((sampleSeconds * 1000.0) / static_cast<double>(frameStats.accumulatedFrames));

            if (!std::isfinite(frameStats.displayFps) || !std::isfinite(frameStats.displayMs)
                || frameStats.displayFps <= 0.0f || frameStats.displayMs <= 0.0f)
            {
                frameStats.displayFps = sampleFps;
                frameStats.displayMs = sampleMs;
            }
            else
            {
                constexpr float kStatsBlend = 0.35f;
                frameStats.displayFps = std::lerp(frameStats.displayFps, sampleFps, kStatsBlend);
                frameStats.displayMs = std::lerp(frameStats.displayMs, sampleMs, kStatsBlend);
            }

            frameStats.accumulatedSeconds = 0.0;
            frameStats.accumulatedFrames = 0u;
        }
    }
    app.rendererSettings.mainViewportFpsDisplay = frameStats.displayFps;
    app.rendererSettings.mainViewportFrameMsDisplay = frameStats.displayMs;

    const AssetStreamingStats streamingStats = app.assets->GetStreamingStats();
    const bool hasPendingStreaming = streamingStats.HasPendingWork();
    const float targetProgress01 = streamingStats.Completion01();

    auto& overlay = app.loadingOverlay;
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

    app.rendererSettings.loadingOverlayVisible = overlay.visible;
    app.rendererSettings.loadingOverlayProgressBar = overlay.visible
        ? std::clamp(overlay.displayProgressBar, hasPendingStreaming ? 0.02f : 1.0f, 1.0f)
        : 0.0f;

    app.rendererSettings.loadingOverlayTotalUnits = streamingStats.total.totalEntries;
    app.rendererSettings.loadingOverlayCompletedUnits = streamingStats.total.loadedEntries + streamingStats.total.failedEntries;
    
    return deltaSeconds;
}

static void UpdateInputAndCamera(AppState& app, float deltaSeconds)
{
    app.win32Input.SetCaptureMode(appUi::GetInputCaptureForImGui());
    app.win32Input.NewFrame(app.window.hwnd);
    if (app.gameplayMode == rendern::GameplayRuntimeMode::Editor)
    {
        app.cameraController->Update(
            deltaSeconds, 
            app.win32Input.State(), 
            app.scene.camera);
    }
}

static void UpdateEditorViewportInteraction(AppState& app)
{
    if (app.win32Input.State().KeyPressed(VK_F5))
    {
        app.gameplayMode = (app.gameplayMode == rendern::GameplayRuntimeMode::Editor)
            ? rendern::GameplayRuntimeMode::Game
            : rendern::GameplayRuntimeMode::Editor;
    }

    if (app.win32Input.State().KeyPressed(VK_F6))
    {
        app.rendererSettings.drawAnimationRuntimeOverlay = !app.rendererSettings.drawAnimationRuntimeOverlay;
    }
    if (app.gameplayMode == rendern::GameplayRuntimeMode::Game)
    {
        ResetEditorInteractionState(app);
    }
    else
    {
        appEditor::ApplyGizmoModeHotkeys(
            app.editorViewportInteraction, 
            app.scene, 
            app.win32Input.State());
        appEditor::SyncEditorGizmoVisuals(
            app.editorViewportInteraction, 
            *app.levelAsset, 
            *app.levelInstance,
            app.scene);
        appEditor::UpdateViewportGizmoHover(
            app.editorViewportInteraction, 
            app.window.hwnd, app.window.width, 
            app.window.height, 
            app.scene, 
            app.win32Input.State());
        appEditor::HandleViewportMouseInteraction(
            app.editorViewportInteraction, 
            app.window.hwnd, 
            app.window.width, 
            app.window.height, 
            *app.levelAsset, 
            *app.levelInstance, 
            *app.assets, 
            app.scene, 
            app.win32Input.State());
    }
}

static void UpdateGameplayAndAnimation(AppState& app, float deltaSeconds)
{
    if (app.gameplayRuntime)
    {
        rendern::GameplayUpdateContext gameplayCtx{};
        gameplayCtx.deltaSeconds = deltaSeconds;
        gameplayCtx.mode = app.gameplayMode;
        gameplayCtx.input = &app.win32Input.State();
        gameplayCtx.levelAsset = app.levelAsset.get();
        gameplayCtx.levelInstance = app.levelInstance.get();
        gameplayCtx.scene = &app.scene;

        app.gameplayRuntime->BeginFrame();
        app.gameplayRuntime->PreAnimationUpdate(gameplayCtx);
    }

    app.scene.UpdateSkinned(deltaSeconds);

    if (app.gameplayRuntime)
    {
        rendern::GameplayUpdateContext gameplayCtx{};
        gameplayCtx.deltaSeconds = deltaSeconds;
        gameplayCtx.mode = app.gameplayMode;
        gameplayCtx.input = &app.win32Input.State();
        gameplayCtx.levelAsset = app.levelAsset.get();
        gameplayCtx.levelInstance = app.levelInstance.get();
        gameplayCtx.scene = &app.scene;
        app.gameplayRuntime->PostAnimationUpdate(gameplayCtx);
    }
    
    UpdateGameplayMovementDebug(app);
    UpdateAnimationRuntimeDebug(app);
    app.scene.UpdateParticles(deltaSeconds);
}

static void RenderMainViewport(AppState& app)
{
    app.renderer->SetSettings(app.rendererSettings);
    app.renderer->RenderFrame(*app.swapChain, app.scene, /*imguiDrawData=*/nullptr);
}

static void RenderDebugWindowIfNeeded(AppState& app, const void* imguiDrawData)
{
#if defined(CORE_USE_DX12)
    if (appRuntime::CanRenderDebugSwapChain(app.debugWindow, app.debugSwapChain.get()))
    {
        appUi::RenderImGuiToSwapChainIfEnabled(*app.device, *app.debugSwapChain, imguiDrawData);
    }
#endif
}