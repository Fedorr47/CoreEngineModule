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
        appEditor::EndAllGizmoDrags(app.editorViewportInteraction, app.scene);
        appEditor::ClearAllGizmoHover(app.editorViewportInteraction, app.scene);
        appEditor::ResetGizmoState(app.scene.editorTranslateGizmo);
        appEditor::ResetGizmoState(app.scene.editorRotateGizmo);
        appEditor::ResetGizmoState(app.scene.editorScaleGizmo);
        app.scene.editorParticleEmitterTranslateDrag = {};
        app.scene.EditorClearSelection();
    }

    static void UpdateGameplayMovementDebug(AppState& app)
    {
        app.scene.gameplayMovementDebug.Clear();

        if (!app.gameplayRuntime || !app.rendererSettings.drawGameplayMovementDebug)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = app.gameplayRuntime->GetControlledEntity();
        const auto& entities = app.gameplayRuntime->GetNodeBoundEntities();
        app.scene.gameplayMovementDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (app.rendererSettings.drawGameplayMovementDebugOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const rendern::GameplayTransformComponent* transform = app.gameplayRuntime->GetWorld().TryGetTransform(entity);
            const rendern::GameplayCharacterMotorComponent* motor = app.gameplayRuntime->GetWorld().TryGetCharacterMotor(entity);
            if (transform == nullptr || motor == nullptr)
            {
                continue;
            }

            const rendern::GameplayLocomotionComponent* locomotion = app.gameplayRuntime->GetWorld().TryGetLocomotion(entity);
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
            app.scene.gameplayMovementDebug.samples.push_back(sample);
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
        app.scene.animationRuntimeDebug.Clear();

        if (!app.gameplayRuntime || !app.rendererSettings.drawAnimationRuntimeOverlay || app.gameplayMode != rendern::GameplayRuntimeMode::Game)
        {
            return;
        }

        if (!app.levelAsset || !app.levelInstance)
        {
            return;
        }

        const rendern::EntityHandle controlledEntity = app.gameplayRuntime->GetControlledEntity();
        const auto& entities = app.gameplayRuntime->GetNodeBoundEntities();
        app.scene.animationRuntimeDebug.samples.reserve(entities.size());

        for (const rendern::EntityHandle entity : entities)
        {
            if (app.rendererSettings.drawAnimationRuntimeOverlayOnlyControlled
                && controlledEntity != rendern::kNullEntity
                && entity != controlledEntity)
            {
                continue;
            }

            const auto& world = app.gameplayRuntime->GetWorld();
            const rendern::GameplayTransformComponent* transform = world.TryGetTransform(entity);
            const rendern::GameplayNodeLinkComponent* nodeLink = world.TryGetNodeLink(entity);
            const rendern::GameplayAnimationLinkComponent* animationLink = world.TryGetAnimationLink(entity);
            const rendern::GameplayAnimationStateComponent* animState = world.TryGetAnimationState(entity);
            const rendern::GameplayAnimationNotifyStateComponent* notifyState = world.TryGetAnimationNotifyState(entity);
            if (transform == nullptr || nodeLink == nullptr || animationLink == nullptr || animState == nullptr)
            {
                continue;
            }

            if (nodeLink->nodeIndex < 0 || static_cast<std::size_t>(nodeLink->nodeIndex) >= app.levelAsset->nodes.size())
            {
                continue;
            }

            const rendern::LevelNode& node = app.levelAsset->nodes[static_cast<std::size_t>(nodeLink->nodeIndex)];
            rendern::SkinnedDrawItem* skinnedItem = app.levelInstance->GetSkinnedDrawItem(app.scene, animationLink->skinnedDrawIndex);
            if (skinnedItem == nullptr)
            {
                continue;
            }

            const rendern::AnimationControllerRuntime& runtime = skinnedItem->controller;
            const float secondaryWeight = std::clamp(runtime.blendSecondaryAlpha, 0.0f, 1.0f);
            const float tertiaryWeight = std::clamp(runtime.blendTertiaryAlpha, 0.0f, 1.0f);
            const float primaryWeight = std::max(0.0f, 1.0f - secondaryWeight - tertiaryWeight);
            const float transitionAlpha = (runtime.transitionDurationSeconds > 1e-6f)
                ? std::clamp(runtime.transitionElapsedSeconds / runtime.transitionDurationSeconds, 0.0f, 1.0f)
                : (runtime.transitionActive ? 1.0f : 0.0f);

            rendern::AnimationRuntimeDebugSample sample{};
            sample.entity = entity;
            sample.origin = transform->position;
            sample.nodeName = node.name;
            sample.controllerAssetId = animState->controllerAssetId;
            sample.currentStateName = animState->currentStateName;
            sample.previousStateName = animState->previousStateName;
            sample.requestedStateName = runtime.requestedStateName;
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
            sample.transitionActive = runtime.transitionActive;
            sample.controlled = entity == controlledEntity;
            app.scene.animationRuntimeDebug.samples.push_back(std::move(sample));
        }
    }

    void InitializeApp(AppState& app, int argc, char** argv)
    {
        app.requestedBackend = appBootstrap::ParseAppArguments(argc, argv, app.appArguments);
        app.canUseDebugWindow = appBootstrap::CanUseDebugWindow(app.requestedBackend);

        appBootstrap::CreatePrimaryWindowSet(
            app.config.windowWidth,
            app.config.windowHeight,
            app.config.windowTitle,
            app.canUseDebugWindow,
            app.window
#if defined(CORE_USE_DX12)
            , &app.debugWindow
#endif
        );

        appBootstrap::BindWin32Input(app.win32Input);
        appBootstrap::CreateDeviceAndSwapChain(
            app.requestedBackend,
            app.window.hwnd,
            app.config.windowWidth,
            app.config.windowHeight,
            app.device,
            app.swapChain);

#if defined(CORE_USE_DX12)
        appBootstrap::CreateDebugSwapChainIfNeeded(app.requestedBackend, *app.device, app.debugWindow, app.debugSwapChain);
#endif

        app.jobSystem = std::make_unique<rendern::JobSystemThreadPool>(ComputeStreamingWorkerCount());

        app.textureUploader = appBootstrap::CreateTextureUploader(app.device->GetBackend(), *app.device);
        app.textureIO = std::make_unique<TextureIO>(app.textureDecoder, *app.textureUploader, *app.jobSystem, app.renderQueue);
        app.meshIO = std::make_unique<rendern::MeshIO>(*app.device, *app.jobSystem, app.renderQueue);
        app.assets = std::make_unique<AssetManager>(*app.textureIO, *app.meshIO);
        
        const std::string defaultLevelName = std::string(DefaultStartupLevelName);
        const auto mapIt = app.appArguments.find(std::string(MAP_LITERAL));
        const bool hasOverride =
            mapIt != app.appArguments.end()
            && !mapIt->second.empty()
            && !mapIt->second.front().empty();
        
        if (!hasOverride)
        {
            std::cerr << "[Startup] Using default startup level: " << defaultLevelName << '\n';
            app.levelAsset = std::make_unique<rendern::LevelAsset>(
                rendern::LoadLevelAssetFromJson(defaultLevelName));
            app.currentLevelName = defaultLevelName;
        }
        else
        {
            const std::string& overrideLevelName = mapIt->second.front();
            std::cerr << "[Startup] Trying startup level override: " << overrideLevelName << '\n';

            try
            {
                app.levelAsset = std::make_unique<rendern::LevelAsset>(
                    rendern::LoadLevelAssetFromJson(overrideLevelName));
                app.currentLevelName = overrideLevelName;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[Startup] Override failed: " << e.what()
          << ". Falling back to default: " << defaultLevelName << '\n';
                
                app.levelAsset = std::make_unique<rendern::LevelAsset>(
                    rendern::LoadLevelAssetFromJson(defaultLevelName));
                app.currentLevelName = defaultLevelName;
            }
        }
        std::cerr << "[Startup] Chosen startup level: " << app.currentLevelName << '\n';
        
        app.rendererSettings.drawLightGizmos = true;
        app.rendererSettings.loadingOverlayVisible = true;
        app.rendererSettings.loadingOverlayProgressBar = 0.0f;
        app.renderer = std::make_unique<rendern::Renderer>(*app.device, app.rendererSettings);

#if defined(CORE_USE_DX12)
        if (app.requestedBackend == rhi::Backend::DirectX12 && app.debugSwapChain && app.debugWindow.hwnd)
        {
            appUi::InitializeImGui(app.debugWindow.hwnd, *app.device, app.debugSwapChain->GetDesc().backbufferFormat, /*backbufferCount=*/2);
        }
#endif

        app.scene.Clear();
        app.bindless = std::make_unique<rendern::BindlessTable>(*app.device);
        app.levelInstance = std::make_unique<rendern::LevelInstance>(rendern::InstantiateLevel(
            app.scene,
            *app.assets,
            *app.bindless,
            *app.levelAsset,
            mathUtils::Mat4(1.0f)));

        app.gameplayRuntime = std::make_unique<rendern::GameplayRuntime>();
        app.gameplayRuntime->Initialize(*app.levelAsset, *app.levelInstance, app.scene);

        app.cameraController = std::make_unique<rendern::CameraController>();
        app.cameraController->ResetFromCamera(app.scene.camera);

        app.gameplayMode = rendern::GameplayRuntimeMode::Editor;

        app.frameTimer.SetMaxDelta(0.05);
        app.frameTimer.Reset();
        app.statsTimer.SetMaxDelta(10.0);
        app.statsTimer.Reset();
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

        appRuntime::DriveAssetStreaming(*app.assets, *app.levelInstance, *app.bindless, app.scene, app.config.uploadBudget);
        
        const float deltaSeconds = UpdateFrameTimingAndLoadingOverlay(app);
        UpdateInputAndCamera(app, deltaSeconds);
        UpdateEditorViewportInteraction(app);
        UpdateGameplayAndAnimation(app, deltaSeconds);
        
        const void* imguiDrawData = appUi::BuildImGuiFrameIfEnabled(
           *app.device,
           app.rendererSettings,
           app.scene,
           *app.cameraController,
           *app.levelAsset,
           *app.levelInstance,
           *app.assets,
           app.gameplayMode);

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

        appRuntime::ShutdownRuntime(
            *app.device,
            *app.renderer,
            *app.levelInstance,
            *app.bindless,
            *app.jobSystem,
            *app.assets,
            app.window
#if defined(CORE_USE_DX12)
            , &app.debugWindow
#endif
        );

#if defined(CORE_USE_DX12)
        app.debugSwapChain.reset();
#endif
        app.swapChain.reset();
        app.renderer.reset();
        app.gameplayRuntime.reset();
        app.levelInstance.reset();
        app.bindless.reset();
        app.levelAsset.reset();
        app.assets.reset();
        app.meshIO.reset();
        app.textureIO.reset();
        app.textureUploader.reset();
        app.jobSystem.reset();
        app.device.reset();
        app.cameraController.reset();
        app.initialized = false;
    }
}
