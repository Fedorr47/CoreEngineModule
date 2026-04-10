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

namespace appLifecycle
{
    static std::uint32_t ComputeStreamingWorkerCount() noexcept
    {
        const unsigned int hc = std::thread::hardware_concurrency();
        if (hc <= 1u)
        {
            return 1u;
        }

        unsigned int wc = hc - 1u;
        if (wc < 1u)
            wc = 1u;
        if (wc > 8u)
            wc = 8u;

        return static_cast<std::uint32_t>(wc);
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
        
        launchState.requestedBackend = appBootstrap::ParseAppArguments(argc, argv, launchState.appArguments);
        launchState.canUseDebugWindow = appBootstrap::CanUseDebugWindow(launchState.requestedBackend);

        appBootstrap::CreatePrimaryWindowSet(
            app.config.windowWidth,
            app.config.windowHeight,
            app.config.windowTitle,
            launchState.canUseDebugWindow,
            windowState.mainWindow
#if defined(CORE_USE_DX12)
            , &windowState.debugWindow
#endif
        );

        appBootstrap::BindWin32Input(windowState.input);
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
    }
    
#include "AppLifecycleImpl/AppLifecycle_TickImpl.inl"

    bool TickApp(AppState& app)
    {
        if (!PumpAndCheckRunning(app))
        {
            return false;
        }

        ApplyPendingResize(app);
        if (ShouldSkipFrame(app))
        {
            return true;
        }
        
        auto& runtimeState      = app.runtimeState;
        auto& graphicState      = app.graphicsState;
        auto& contentState      = app.contentState;

        appRuntime::DriveAssetStreaming(
            *contentState.assets, 
            *runtimeState.levelInstance, 
            *graphicState.bindless, 
            runtimeState.scene, 
            app.config.uploadBudget);
        
        const float deltaSeconds = UpdateFrameTimingAndLoadingOverlay(app);
        UpdateInputAndCamera(app, deltaSeconds);
        UpdateEditorViewportInteraction(app);
        UpdateGameplayAndAnimation(app, deltaSeconds);
        
        const void* imguiDrawData = appUi::BuildImGuiFrameIfEnabled(
           *graphicState.device,
           graphicState.rendererSettings,
           runtimeState.scene,
           *runtimeState.cameraController,
           *contentState.levelAsset,
           *runtimeState.levelInstance,
           *contentState.assets,
           runtimeState.gameplayMode);

        RenderMainViewport(app);
        RenderDebugWindowIfNeeded(app, imguiDrawData);

        appWin32::TinySleep();
        return true;
    }

    void ShutdownApp(AppState& app)
    {
        if (!app.initialized)
        {
            return;
        }
        
        auto& runtimeState      = app.runtimeState;
        auto& graphicState      = app.graphicsState;
        auto& windowState       = app.windowState;
        auto& contentState      = app.contentState;
        
        appRuntime::ShutdownRuntime(
            *graphicState.device,
            *graphicState.renderer,
            *runtimeState.levelInstance,
            *graphicState.bindless,
            *contentState.jobSystem,
            *contentState.assets,
            windowState.mainWindow
#if defined(CORE_USE_DX12)
            , &windowState.debugWindow
#endif
        );

#if defined(CORE_USE_DX12)
        graphicState.debugSwapChain.reset();
#endif
        graphicState.swapChain.reset();
        graphicState.renderer.reset();
        graphicState.bindless.reset();
        graphicState.device.reset();
        runtimeState.gameplayRuntime.reset();
        runtimeState.levelInstance.reset();
        runtimeState.cameraController.reset();
        contentState.levelAsset.reset();
        contentState.assets.reset();
        contentState.meshIO.reset();
        contentState.textureIO.reset();
        contentState.textureUploader.reset();
        contentState.jobSystem.reset();
        app.initialized = false;
    }
}
