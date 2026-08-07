#include "Core/ThreadAffinity/ThreadAffinityAssertions.h"
#include "Physics/Jolt/JoltRuntime.h"
#include "Physics/Jolt/JoltPhysicsWorld.h"
#include "Physics/LevelPhysicsRuntime.h"

import core;
import std;

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "AppLifecycle.h"
#include "AppArguments.h"
#include "Jolt/Core/IncludeWindows.h"

namespace appLifecycle
{
    AppPhysicsState::AppPhysicsState() = default;
    AppPhysicsState::~AppPhysicsState()
    {
        Shutdown();
    }
    
    void AppPhysicsState::Initialize()
    {
        auto newRuntime = std::make_unique<physics::JoltRuntime>();
        if (!newRuntime->Initialize())
        {
            throw std::runtime_error("Failed to initialize the Jolt runtime: "
                "another process-wide Jolt owner already exists.");
        }
    
        auto newPhysicsWorld = std::make_unique<physics::JoltPhysicsWorld>(*newRuntime);
        if (!newPhysicsWorld->Initialize())
        {
            newPhysicsWorld.reset();
            newRuntime.reset();
            throw std::runtime_error("Failed to initialize the Jolt physics world.");
        }
    
        joltRuntime = std::move(newRuntime);
        joltPhysicsWorld = std::move(newPhysicsWorld);
        levelPhysicsRuntime = std::make_unique<physics::LevelPhysicsRuntime>(*joltPhysicsWorld);
    }
    
    void AppPhysicsState::Shutdown() noexcept
    {
        levelPhysicsRuntime.reset();
        joltPhysicsWorld.reset();
        joltRuntime.reset();
    }
    
    std::uint32_t ComputeStreamingWorkerCount(const unsigned int hardwareThreadCount) noexcept
    {
        if (hardwareThreadCount <= 1u)
        {
            return 1u;
        }

        unsigned int workableCount = hardwareThreadCount - 1u;
        if (workableCount < 1u)
            workableCount = 1u;
        if (workableCount > 8u)
            workableCount = 8u;

        return static_cast<std::uint32_t>(workableCount);
    }
    
    std::uint32_t ComputeStreamingWorkerCount() noexcept
    {
        return ComputeStreamingWorkerCount(std::thread::hardware_concurrency());
    }

    static void ResetEditorInteractionState(AppState& app)
    {
        auto& runtime = app.runtimeState;
        appEditor::EndAllGizmoDrags(runtime.editorViewportInteraction, runtime.scene);
        appEditor::ClearAllGizmoHover(runtime.editorViewportInteraction, runtime.scene);
        appEditor::ResetGizmoState(runtime.scene.editorTranslateGizmo);
        appEditor::ResetGizmoState(runtime.scene.editorRotateGizmo);
        appEditor::ResetGizmoState(runtime.scene.editorScaleGizmo);
        runtime.scene.editorParticleEmitterTranslateDrag = {};
        runtime.scene.EditorClearSelection();
    }

    static void UpdateGameplayMovementDebug(AppState& app)
    {
        auto& runtimeState = app.runtimeState;
        auto& graphicState = app.graphicsState;
        runtimeState.scene.gameplayMovementDebug.Clear();

        if (!runtimeState.gameplayRuntime || !graphicState.rendererSettings.drawGameplayMovementDebug)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = runtimeState.gameplayRuntime->GetControlledEntity();
        const auto& entities = runtimeState.gameplayRuntime->GetNodeBoundEntities();
        runtimeState.scene.gameplayMovementDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (graphicState.rendererSettings.drawGameplayMovementDebugOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const rendern::GameplayTransformComponent* transform = runtimeState.gameplayRuntime->GetWorld().TryGetTransform(entity);
            const rendern::GameplayCharacterMotorComponent* motor = runtimeState.gameplayRuntime->GetWorld().TryGetCharacterMotor(entity);
            if (transform == nullptr || motor == nullptr)
            {
                continue;
            }

            const rendern::GameplayLocomotionComponent* locomotion = runtimeState.gameplayRuntime->GetWorld().TryGetLocomotion(entity);
            const bool isRunning = locomotion != nullptr && locomotion->isRunning;
            const float targetSpeed = isRunning ? motor->maxRunSpeed : motor->maxWalkSpeed;

            const float yawRad = mathUtils::DegToRad(transform->rotationDegrees.y);
            const mathUtils::Vec3 facingForward(std::sin(yawRad), 0.0f, std::cos(yawRad));

            rendern::GameplayMovementDebugSample sample{};
            sample.entity = entity;
            sample.origin = transform->position;
            sample.velocity = motor->velocity;
            sample.targetVelocity = motor->desiredMoveWorld * targetSpeed;
            sample.desiredMoveWorld = motor->desiredMoveWorld;
            sample.facingForward = facingForward;
            sample.forwardSpeed = locomotion != nullptr ? locomotion->forwardSpeed : 0.0f;
            sample.rightSpeed = locomotion != nullptr ? locomotion->rightSpeed : 0.0f;
            sample.planarSpeed = locomotion != nullptr ? locomotion->planarSpeed : mathUtils::Length(motor->velocity);
            sample.controlled = entity == controlledEntity;
            runtimeState.scene.gameplayMovementDebug.samples.push_back(sample);
        }
    }

    static float AnimationRuntimeGetNormalizedTime(const rendern::AnimatorState& animator)
    {
        if (animator.clip == nullptr || animator.clip->ticksPerSecond <= 0.0f)
        {
            return 0.0f;
        }

        const float durationSeconds = animator.clip->durationTicks / animator.clip->ticksPerSecond;
        if (durationSeconds <= 1e-6f)
        {
            return 0.0f;
        }

        const float normalized = animator.timeSeconds / durationSeconds;
        if (animator.looping)
        {
            const float wrapped = normalized - std::floor(normalized);
            return std::clamp(wrapped, 0.0f, 1.0f);
        }

        return std::clamp(normalized, 0.0f, 1.0f);
    }

    static void UpdateAnimationRuntimeDebug(AppState& app)
    {
        auto& runtimeState = app.runtimeState;
        auto& graphicState = app.graphicsState;
        auto& contentState = app.contentState;
        runtimeState.scene.animationRuntimeDebug.Clear();

        if (!runtimeState.gameplayRuntime 
            || !graphicState.rendererSettings.drawAnimationRuntimeOverlay 
            || runtimeState.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
            return;
        }

        if (!contentState.levelAsset || !runtimeState.levelInstance)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = runtimeState.gameplayRuntime->GetControlledEntity();
        const auto& entities = runtimeState.gameplayRuntime->GetNodeBoundEntities();
        runtimeState.scene.animationRuntimeDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (graphicState.rendererSettings.drawAnimationRuntimeOverlayOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const auto& world = runtimeState.gameplayRuntime->GetWorld();
            const rendern::GameplayTransformComponent* transform = world.TryGetTransform(entity);
            const rendern::GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);
            const rendern::GameplayAnimationLinkComponent* animationLink = world.TryGetAnimationLink(entity);
            const rendern::GameplayAnimationStateComponent* animState = world.TryGetAnimationState(entity);
            const rendern::GameplayAnimationNotifyStateComponent* notifyState = world.TryGetAnimationNotifyState(entity);
            if (transform == nullptr || nodeLink == nullptr || animationLink == nullptr || animState == nullptr)
            {
                continue;
            }

            if (nodeLink->nodeIndex < 0 || static_cast<std::size_t>(nodeLink->nodeIndex) >= contentState.levelAsset->nodes.size())
            {
                continue;
            }

            const rendern::LevelNode& node = contentState.levelAsset->nodes[static_cast<std::size_t>(nodeLink->nodeIndex)];
            rendern::SkinnedDrawItem* skinnedItem = runtimeState.levelInstance->GetSkinnedDrawItem(runtimeState.scene, animationLink->skinnedDrawIndex);
            if (skinnedItem == nullptr)
            {
                continue;
            }

            const rendern::AnimationControllerRuntime& runtimeController = skinnedItem->controller;
            const float secondaryWeight = std::clamp(runtimeController.blendSecondaryAlpha, 0.0f, 1.0f);
            const float tertiaryWeight = std::clamp(runtimeController.blendTertiaryAlpha, 0.0f, 1.0f);
            const float primaryWeight = std::max(0.0f, 1.0f - secondaryWeight - tertiaryWeight);
            const float transitionAlpha = (runtimeController.transitionDurationSeconds > 1e-6f)
                ? std::clamp(runtimeController.transitionElapsedSeconds / runtimeController.transitionDurationSeconds, 0.0f, 1.0f)
                : (runtimeController.transitionActive ? 1.0f : 0.0f);

            rendern::AnimationRuntimeDebugSample sample{};
            sample.entity = entity;
            sample.origin = transform->position;
            sample.nodeName = node.name;
            sample.controllerAssetId = animState->controllerAssetId;
            sample.currentStateName = animState->currentStateName;
            sample.previousStateName = animState->previousStateName;
            sample.requestedStateName = runtimeController.requestedStateName;
            sample.modeName = animState->modeName;
            sample.primaryClipName = animState->primaryClipName;
            sample.secondaryClipName = animState->secondaryClipName;
            sample.tertiaryClipName = animState->tertiaryClipName;
            sample.blendParameterNameX = animState->blendParameterNameX;
            sample.blendParameterNameY = animState->blendParameterNameY;
            sample.blendParameterValueX = animState->blendParameterValueX;
            sample.blendParameterValueY = animState->blendParameterValueY;
            sample.lastNotifyId = notifyState != nullptr ? notifyState->lastNotifyId : std::string{};
            sample.normalizedTime = AnimationRuntimeGetNormalizedTime(skinnedItem->animator);
            sample.primaryWeight = primaryWeight;
            sample.secondaryWeight = secondaryWeight;
            sample.tertiaryWeight = tertiaryWeight;
            sample.transitionAlpha = transitionAlpha;
            sample.transitionActive = runtimeController.transitionActive;
            sample.controlled = entity == controlledEntity;
            runtimeState.scene.animationRuntimeDebug.samples.push_back(std::move(sample));
        }
    }

    void InitializeApp(AppState& app, int argc, char** argv)
    {
        auto& runtimeState      = app.runtimeState;
        auto& graphicState      = app.graphicsState;
        auto& launchState       = app.launchState;
        auto& windowState       = app.windowState;
        auto& contentState      = app.contentState;
        auto& frameState        = app.frameState;
        auto& physicsState      = app.physicsState;
        
        const appBootstrap::ParsedBackend parsedBackend =
        appBootstrap::ParseAppArguments(argc, argv, launchState.appArguments);
        rhi::Backend backend = rhi::Backend::DirectX12;
        if (parsedBackend == appBootstrap::ParsedBackend::Null)
        {
            backend = rhi::Backend::Null;
        }
        
        launchState.requestedBackend = backend;
        launchState.canUseDebugWindow = appBootstrap::CanUseDebugWindow(launchState.requestedBackend);
        windowState.shell.input = &windowState.input;

        threadAffinity::ResetOwnerThreadRegistry();
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Runtime);
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Physics);
        
        physicsState.Initialize();

        appBootstrap::CreatePrimaryWindowSet(
            windowState.shell,
            app.config.windowWidth,
            app.config.windowHeight,
            app.config.windowTitle,
            launchState.canUseDebugWindow,
            windowState.mainWindow
#if defined(CORE_USE_DX12)
            , &windowState.debugWindow
#endif
        );

        appBootstrap::CreateDeviceAndSwapChain(
            launchState.requestedBackend,
            windowState.mainWindow.hwnd,
            app.config.windowWidth,
            app.config.windowHeight,
            graphicState.device,
            graphicState.swapChain);

#if defined(CORE_USE_DX12)
        appBootstrap::CreateDebugSwapChainIfNeeded(
            launchState.requestedBackend, 
            *graphicState.device, 
            windowState.debugWindow, 
            graphicState.debugSwapChain);
#endif

        contentState.jobSystem = std::make_unique<rendern::JobSystemThreadPool>(ComputeStreamingWorkerCount());

        contentState.textureUploader = appBootstrap::CreateTextureUploader(graphicState.device->GetBackend(), *graphicState.device);
        contentState.textureIO = std::make_unique<TextureIO>(
            contentState.textureDecoder, *contentState.textureUploader, *contentState.jobSystem, contentState.renderQueue);
        contentState.meshIO = std::make_unique<rendern::MeshIO>(*graphicState.device, *contentState.jobSystem, contentState.renderQueue);
        contentState.assets = std::make_unique<AssetManager>(*contentState.textureIO, *contentState.meshIO);
        
        const std::string defaultLevelName = std::string(DefaultStartupLevelName);
        const auto mapIt = launchState.appArguments.find(std::string(MAP_LITERAL));
        const bool hasOverride =
            mapIt != launchState.appArguments.end()
            && !mapIt->second.empty()
            && !mapIt->second.front().empty();
        
        if (!hasOverride)
        {
            std::cerr << "[Startup] Using default startup level: " << defaultLevelName << '\n';
            contentState.levelAsset = std::make_unique<rendern::LevelAsset>(
                rendern::LoadLevelAssetFromJson(defaultLevelName));
            launchState.currentLevelName = defaultLevelName;
        }
        else
        {
            const std::string& overrideLevelName = mapIt->second.front();
            std::cerr << "[Startup] Trying startup level override: " << overrideLevelName << '\n';

            try
            {
                contentState.levelAsset = std::make_unique<rendern::LevelAsset>(
                    rendern::LoadLevelAssetFromJson(overrideLevelName));
                launchState.currentLevelName = overrideLevelName;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[Startup] Override failed: " << e.what()
          << ". Falling back to default: " << defaultLevelName << '\n';
                
                contentState.levelAsset = std::make_unique<rendern::LevelAsset>(
                    rendern::LoadLevelAssetFromJson(defaultLevelName));
                launchState.currentLevelName = defaultLevelName;
            }
        }
        std::cerr << "[Startup] Chosen startup level: " << launchState.currentLevelName << '\n';
        
        graphicState.rendererSettings.drawLightGizmos = true;
        graphicState.rendererSettings.loadingOverlayVisible = true;
        graphicState.rendererSettings.loadingOverlayProgressBar = 0.0f;
        graphicState.renderer = std::make_unique<rendern::Renderer>(*graphicState.device, graphicState.rendererSettings);

#if defined(CORE_USE_DX12)
        if (launchState.requestedBackend == rhi::Backend::DirectX12 
            && graphicState.debugSwapChain 
            && windowState.debugWindow.hwnd)
        {
            appUi::InitializeImGui(
                windowState.shell,
                windowState.debugWindow.hwnd, 
                *graphicState.device, 
                graphicState.debugSwapChain->GetDesc().backbufferFormat, 
                /*backbufferCount=*/2);
        }
#endif

        runtimeState.scene.Clear();
        graphicState.bindless = std::make_unique<rendern::BindlessTable>(*graphicState.device);
        runtimeState.levelInstance = std::make_unique<rendern::LevelInstance>(rendern::InstantiateLevel(
            runtimeState.scene,
            *contentState.assets,
            *graphicState.bindless,
            *contentState.levelAsset,
            mathUtils::Mat4(1.0f)));

        runtimeState.gameplayRuntime = std::make_unique<rendern::GameplayRuntime>();
        runtimeState.gameplayRuntime->Initialize(
            *contentState.levelAsset, *runtimeState.levelInstance, runtimeState.scene);

        runtimeState.cameraController = std::make_unique<rendern::CameraController>();
        runtimeState.cameraController->ResetFromCamera(runtimeState.scene.camera);

        runtimeState.gameplayMode = rendern::GameplayRuntimeMode::Editor;

        frameState.frameTimer.SetMaxDelta(0.05);
        frameState.frameTimer.Reset();
        frameState.statsTimer.SetMaxDelta(10.0);
        frameState.statsTimer.Reset();
        
        app.initialized = true;
        app.mainThreadId = std::this_thread::get_id();
    }
    
    bool SetGameplayMode(
        AppState& app, 
        rendern::GameplayRuntimeMode mode, 
        std::string& errorMessage)
    {
        auto& runtimeState = app.runtimeState;
        auto& physicsState = app.physicsState;
        if (runtimeState.gameplayMode == mode)
        {
            return true;
        }
        if (mode == rendern::GameplayRuntimeMode::Game)
        {
            if (!physicsState.levelPhysicsRuntime->EnterGame(
                *app.contentState.levelAsset,
                *runtimeState.levelInstance,
                runtimeState.scene,
                errorMessage))
            {
                return false;
            }
        }
        else
        {
            const bool leftGame = physicsState.levelPhysicsRuntime->LeaveGame(
                *app.contentState.levelAsset,
                *runtimeState.levelInstance,
                runtimeState.scene,
                errorMessage);
            if (!leftGame)
            {
                return false;
            }
        }
        runtimeState.gameplayMode = mode;
        return true;
    }
    
#include "AppLifecycleImpl/AppLifecycle_TickImpl.inl"
    
    static double Ms(auto duration)
    {
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    static void UpdatePerformanceSnapshot(
        rendern::RendererSettings& rendererSettings,
        const FrameStatsOverlayState& frameStats,
        float rawFrameTimeMs,
        const CpuFrameTimings& cpuFrameTimings,
        const rendern::RendererCpuTimingSnapshot& rendererCpuTimings)
    {
        rendern::PerformanceSnapshot& performanceSnapshot = rendererSettings.performanceSnapshot;
        performanceSnapshot.fps = frameStats.displayFps;
        performanceSnapshot.frameTimeMs = frameStats.displayMs;
        performanceSnapshot.rawFrameTimeMs = rawFrameTimeMs;
        performanceSnapshot.cpuFrameStages.totalBeforeSleepMs = static_cast<float>(cpuFrameTimings.totalBeforeSleepMs);
        performanceSnapshot.cpuFrameStages.totalWithSleepMs = static_cast<float>(cpuFrameTimings.totalWithSleepMs);
        performanceSnapshot.cpuFrameStages.updateFrameTimingMs = static_cast<float>(cpuFrameTimings.updateFrameTimingMs);
        performanceSnapshot.cpuFrameStages.inputMs = static_cast<float>(cpuFrameTimings.inputMs);
        performanceSnapshot.cpuFrameStages.editorInteractionMs = static_cast<float>(cpuFrameTimings.editorInteractionMs);
        performanceSnapshot.cpuFrameStages.streamingMs = static_cast<float>(cpuFrameTimings.streamingMs);
        performanceSnapshot.cpuFrameStages.gameplayAndAnimationMs = static_cast<float>(cpuFrameTimings.gameplayAndAnimationMs);
        performanceSnapshot.cpuFrameStages.buildImGuiMs = static_cast<float>(cpuFrameTimings.buildImGuiMs);
        performanceSnapshot.cpuFrameStages.renderMainViewportMs = static_cast<float>(cpuFrameTimings.renderMainViewportMs);
        performanceSnapshot.cpuFrameStages.renderDebugWindowMs = static_cast<float>(cpuFrameTimings.renderDebugWindowMs);
        performanceSnapshot.cpuFrameStages.tinySleepMs = static_cast<float>(cpuFrameTimings.tinySleepMs);
        performanceSnapshot.rendererCpuTimings = rendererCpuTimings;
        performanceSnapshot.hasGpuTimings = false;

        const float frameTimeSampleMs = performanceSnapshot.rawFrameTimeMs > 0.0f
            ? performanceSnapshot.rawFrameTimeMs
            : performanceSnapshot.frameTimeMs;
        performanceSnapshot.PushFrameTimeSample(frameTimeSampleMs);
    }
    
    static bool TryOpenLevel(
        AppState& app,
        std::string_view requestedPath,
        std::string& outError)
    {
        CORE_ASSERT_MAIN_THREAD();

        auto& runtimeState = app.runtimeState;
        auto& contentState = app.contentState;
        auto& graphicState = app.graphicsState;
        auto& launchState = app.launchState;
        auto& frameState = app.frameState;

        try
        {
            // Prepare the complete candidate before touching the current level.
            auto candidateLevel =
                std::make_unique<rendern::LevelAsset>(rendern::LoadLevelAssetFromJson(requestedPath));

            rendern::Scene candidateScene{};

            auto candidateLevelInstance =
                std::make_unique<rendern::LevelInstance>(
                    rendern::InstantiateLevel(
                        candidateScene,
                        *contentState.assets,
                        *graphicState.bindless,
                        *candidateLevel,
                        mathUtils::Mat4(1.0f)));

            // Level replacement is rare. Prefer a safe GPU synchronization
            // point over keeping old descriptors alive across the switch.
            graphicState.device->WaitIdle();

            ResetEditorInteractionState(app);

            if (runtimeState.gameplayRuntime)
            {
                runtimeState.gameplayRuntime->Shutdown();
                runtimeState.gameplayRuntime.reset();
            }

            if (runtimeState.levelInstance)
            {
                runtimeState.levelInstance->FreeDescriptors(*graphicState.bindless);
            }

            runtimeState.scene = std::move(candidateScene);
            contentState.levelAsset = std::move(candidateLevel);
            runtimeState.levelInstance = std::move(candidateLevelInstance);
            
            // The new level owns its active texture handles.
            // Resources owned only by the previous level can now be released.
            contentState.assets->UnloadUnused();

            runtimeState.gameplayRuntime = std::make_unique<rendern::GameplayRuntime>();

            runtimeState.gameplayRuntime->Initialize(
                *contentState.levelAsset,
                *runtimeState.levelInstance,
                runtimeState.scene);

            runtimeState.cameraController->ResetFromCamera(runtimeState.scene.camera);

            runtimeState.gameplayMode = rendern::GameplayRuntimeMode::Editor;
            
            rendern::ui::ResetLevelEditorUIState();

            launchState.currentLevelName = contentState.levelAsset->sourcePath;

            frameState.loadingOverlay = LoadingOverlayState{};

            graphicState.rendererSettings.loadingOverlayVisible = true;
            graphicState.rendererSettings.loadingOverlayProgressBar = 0.0f;

            if (app.physicsState.joltPhysicsWorld)
            {
                app.physicsState.joltPhysicsWorld->ResetSimulationClock();
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            outError = exception.what();
            return false;
        }
    }

    bool TickApp(AppState& app)
    {
        CORE_ASSERT_MAIN_THREAD();

        using Clock = std::chrono::steady_clock;
        const auto frameStart = std::chrono::steady_clock::now();
        if (!PumpAndCheckRunning(app))
        {
            return false;
        }
        
        std::string requestedLevelPath =
            std::exchange(app.windowState.shell.requestedLevelPath,{});

        if (!requestedLevelPath.empty())
        {
            std::string levelOpenError;

            const bool bLevelOpened = TryOpenLevel(app, requestedLevelPath, levelOpenError);
            if (!bLevelOpened)
            {
                std::cerr
                    << "[Level] Failed to open level '"
                    << requestedLevelPath.data()
                    << "': "
                    << levelOpenError.data()
                    << '\n';
            }
        }

        ApplyPendingResize(app);
        if (ShouldSkipFrame(app))
        {
            return true;
        }
        
        auto& runtimeState      = app.runtimeState;
        auto& graphicState      = app.graphicsState;
        auto& contentState      = app.contentState;

        const auto streamingStart = Clock::now();
        appRuntime::DriveAssetStreaming(
            *contentState.assets, 
            *runtimeState.levelInstance, 
            *graphicState.bindless, 
            runtimeState.scene, 
            app.config.uploadBudget);
        const auto streamingEnd = Clock::now();
        
        const auto timingStart = Clock::now();
        const float deltaSeconds = UpdateFrameTimingAndLoadingOverlay(app);
        const auto timingEnd = Clock::now();

        const auto inputStart = Clock::now();
        UpdateInputAndCamera(app, deltaSeconds);
        const auto inputEnd = Clock::now();

        const auto editorStart = Clock::now();
        UpdateEditorViewportInteraction(app);
        const auto editorEnd = Clock::now();

        const auto gameplayStart = Clock::now();
        UpdateGameplayAndAnimation(app, deltaSeconds);
        const auto gameplayEnd = Clock::now();
        
        UpdatePhysics(app, deltaSeconds);
        
        const auto imguiStart = Clock::now();
        const void* imguiDrawData = appUi::BuildImGuiFrameIfEnabled(
           app.windowState.shell,
           *graphicState.device,
           graphicState.rendererSettings,
           runtimeState.scene,
           *runtimeState.cameraController,
           *contentState.levelAsset,
           *runtimeState.levelInstance,
           *contentState.assets,
           runtimeState.gameplayMode,
           runtimeState.gameplayRuntime.get());
        const auto imguiEnd = Clock::now();

        const auto mainRenderStart = Clock::now();
        RenderMainViewport(app);
        const auto mainRenderEnd = Clock::now();
        const rendern::RendererCpuTimingSnapshot rendererCpuTimings = graphicState.renderer->GetLastCpuTimings();

        const auto debugRenderStart = Clock::now();
        if (graphicState.rendererSettings.enableDebugWindowRender)
        {
            RenderDebugWindowIfNeeded(app, imguiDrawData);
        }
        const auto debugRenderEnd = Clock::now();

        const auto beforeSleep = Clock::now();
        if (graphicState.rendererSettings.enableTinySleep)
        {
            appWin32::TinySleep();
        }
        const auto afterSleep = Clock::now();
        
        auto& cpu = app.frameState.cpuFrameTimings;

        cpu.totalBeforeSleepMs = Ms(beforeSleep - frameStart);
        cpu.totalWithSleepMs = Ms(afterSleep - frameStart);

        cpu.updateFrameTimingMs = Ms(timingEnd - timingStart);
        cpu.inputMs = Ms(inputEnd - inputStart);
        cpu.editorInteractionMs = Ms(editorEnd - editorStart);
        cpu.gameplayAndAnimationMs = Ms(gameplayEnd - gameplayStart);
        cpu.buildImGuiMs = Ms(imguiEnd - imguiStart);
        cpu.renderMainViewportMs = Ms(mainRenderEnd - mainRenderStart);
        cpu.renderDebugWindowMs = Ms(debugRenderEnd - debugRenderStart);
        cpu.tinySleepMs = Ms(afterSleep - beforeSleep);
        cpu.streamingMs = Ms(streamingEnd - streamingStart);
        
        UpdatePerformanceSnapshot(
            graphicState.rendererSettings,
            app.frameState.frameStatsOverlay,
            graphicState.rendererSettings.performanceSnapshot.rawFrameTimeMs,
            cpu,
            rendererCpuTimings);
        
        ++app.frameState.frameIndex;
        const std::uint32_t performanceLogFrameInterval = std::max(1u, graphicState.rendererSettings.performanceLogFrameInterval);
        if (graphicState.rendererSettings.logCpuFrameTimings &&
            (app.frameState.frameIndex % performanceLogFrameInterval) == 0u)
        {
            const rendern::PerformanceSnapshot& performanceSnapshot = graphicState.rendererSettings.performanceSnapshot;
            const rendern::RendererCpuTimingSnapshot& snapshotRendererCpuTimings = performanceSnapshot.rendererCpuTimings;
            std::printf(
                "[Perf] FPS %.1f | frame %.2f ms raw %.2f ms | CPU %.2f/%.2f ms | render main %.2f debug %.2f | "
                "RG build %.2f exec %.2f submit %.2f present %.2f | build setup %.2f shadows %.2f reflection %.2f main %.2f | "
                "imgui %.2f | gameplay+anim %.2f | streaming %.2f | debugWindowRender %s | "
                "vsync %s | tearingSupported %s | presentMode %s | presentFlags 0x%X\n",
                performanceSnapshot.fps,
                performanceSnapshot.frameTimeMs,
                performanceSnapshot.rawFrameTimeMs,
                cpu.totalBeforeSleepMs,
                cpu.totalWithSleepMs,
                cpu.renderMainViewportMs,
                cpu.renderDebugWindowMs,
                snapshotRendererCpuTimings.renderGraphBuildMs,
                snapshotRendererCpuTimings.renderGraphExecuteTotalMs,
                snapshotRendererCpuTimings.renderGraphSubmitCommandListMs,
                snapshotRendererCpuTimings.presentMs,
                snapshotRendererCpuTimings.setupCsmMs,
                snapshotRendererCpuTimings.shadowPassBuildMs,
                snapshotRendererCpuTimings.reflectionCaptureBuildMs,
                snapshotRendererCpuTimings.mainPassBuildMs,
                cpu.buildImGuiMs,
                cpu.gameplayAndAnimationMs,
                cpu.streamingMs,
                graphicState.rendererSettings.enableDebugWindowRender ? "ON" : "OFF",
                snapshotRendererCpuTimings.presentDiagnostics.vsyncEnabled ? "ON" : "OFF",
                snapshotRendererCpuTimings.presentDiagnostics.tearingSupported ? "ON" : "OFF",
                rhi::ToString(snapshotRendererCpuTimings.presentDiagnostics.presentMode),
                snapshotRendererCpuTimings.presentDiagnostics.presentFlags);
            std::fflush(stdout);
        }
        
        return true;
    }

    void ShutdownApp(AppState& app)
    {
        CORE_ASSERT_MAIN_THREAD();

        if (!app.initialized)
        {
            return;
        }
        
        auto& runtimeState      = app.runtimeState;
        auto& graphicState      = app.graphicsState;
        auto& windowState       = app.windowState;
        auto& contentState      = app.contentState;
        auto& physicsState      = app.physicsState;
        
        if (physicsState.levelPhysicsRuntime && physicsState.levelPhysicsRuntime->IsActive())
        {
            physicsState.levelPhysicsRuntime->Shutdown();
        }
        
        appRuntime::ShutdownRuntime(
            windowState.shell,
            *graphicState.device,
            *graphicState.renderer,
            *runtimeState.levelInstance,
            *graphicState.bindless,
            *contentState.jobSystem,
            *contentState.assets
        );

#if defined(CORE_USE_DX12)
        graphicState.debugSwapChain.reset();
#endif
        graphicState.swapChain.reset();
        graphicState.renderer.reset();
        graphicState.bindless.reset();
        graphicState.device.reset();
        if (runtimeState.gameplayRuntime)
        {
            runtimeState.gameplayRuntime->Shutdown();
        }
        runtimeState.gameplayRuntime.reset();
        runtimeState.levelInstance.reset();
        runtimeState.cameraController.reset();
        physicsState.Shutdown();
        contentState.levelAsset.reset();
        contentState.assets.reset();
        contentState.meshIO.reset();
        contentState.textureIO.reset();
        contentState.textureUploader.reset();
        contentState.jobSystem.reset();

        threadAffinity::ResetOwnerThreadRegistry();
        app.initialized = false;
    }
}
