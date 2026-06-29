#pragma once

#include <array>
#include <cstdint>

// Internal renderer debug UI ViewModel helpers. This header intentionally relies on
// the including translation unit importing the core module pieces that declare
// rendern::RendererSettings, rendern::Scene, RendererSettingEditState, and the
// renderer settings edit-state helpers. It is included from
// ImGuiDebugUI.cppm after its module imports and from focused unit tests after
// `import core`; keep it free of ImGui dependencies so it compiles in test
// configurations with or without CORE_USE_DX12.

namespace rendern::ui
{
    struct RendererSettingsEditStateViewModel
    {
        const char* displayLabel{""};
        bool isEnabled{true};
        const char* disabledReason{""};
    };

    struct RendererPipelineSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "Renderer pipeline" };
        bool enableDeferred{ false };
        bool enableDepthPrepass{ false };
        bool enableFrustumCulling{ true };
        bool debugPrintDrawCalls{ false };
        rendern::RendererSettingEditState deferredEditState{};
    };

    struct SSAOSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "SSAO" };
        bool enableSSAO{ true };
        float radius{ 1.0f };
        float bias{ 0.02f };
        float strength{ 1.25f };
        float power{ 1.5f };
        float blurDepthThreshold{ 0.0025f };
        bool canEditSettings{ true };
        const char* disabledReason{ "" };
        rendern::RendererSettingEditState sectionEditState{};
    };

    struct FogSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "Fog" };
        bool enableFog{ false };
        std::uint32_t mode{ 0u };
        float start{ 15.0f };
        float end{ 80.0f };
        float density{ 0.02f };
        std::array<float, 3> color{ 0.60f, 0.70f, 0.80f };
        bool canEditSettings{ false };
    };

    struct AntiAliasingSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "Anti-Aliasing" };
        std::uint32_t mode{ 0u };
        float fxaaSubpix{ 0.75f };
        float fxaaEdgeThreshold{ 0.166f };
        float fxaaEdgeThresholdMin{ 0.0833f };
        bool canEditFxaaSettings{ false };
        const char* fxaaDisabledReason{ "FXAA controls are available only when FXAA is selected." };
        rendern::RendererSettingEditState fxaaEditState{};
    };

    struct HdrBloomSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "HDR / Bloom" };
        bool enableHDR{ true };
        std::uint32_t toneMapMode{ 2u };
        float hdrExposure{ 1.0f };
        bool enableBloom{ true };
        float bloomThreshold{ 1.0f };
        float bloomSoftKnee{ 0.5f };
        float bloomIntensity{ 0.08f };
        float bloomClamp{ 16.0f };
        float bloomRadius{ 1.0f };
        bool canEditHdrSettings{ true };
        bool canEditBloomSettings{ true };
        const char* hdrDisabledReason{ "HDR controls are available only when HDR is enabled." };
        const char* bloomDisabledReason{ "Bloom controls are available only when HDR and bloom are enabled." };
        rendern::RendererSettingEditState hdrToggleEditState{};
        rendern::RendererSettingEditState hdrControlsEditState{};
        rendern::RendererSettingEditState bloomControlsEditState{};
    };

    struct ShadowSettingsViewModel
    {
        RendererSettingsEditStateViewModel category{ "Shadow settings" };
        bool showCubeAtlas{ false };
        std::uint32_t debugCubeAtlasIndex{ 0u };
        std::uint32_t debugShadowCubeMapType{ 1u };
        float dirShadowBaseBiasTexels{ 0.6f };
        float spotShadowBaseBiasTexels{ 1.0f };
        float pointShadowBaseBiasTexels{ 0.4f };
        float shadowSlopeScaleTexels{ 2.0f };
    };

    struct DebugDrawSettingsViewModel
    {
        bool showLightGizmos{ false };
        bool showPickingRay{ false };
        bool showAnimationRuntimeOverlay{ false };
    };

    struct RendererSettingsViewModel
    {
        RendererPipelineSettingsViewModel pipeline{};
        SSAOSettingsViewModel ssao{};
        FogSettingsViewModel fog{};
        AntiAliasingSettingsViewModel antiAliasing{};
        HdrBloomSettingsViewModel hdrBloom{};
        ShadowSettingsViewModel shadows{};
    };

    [[nodiscard]] inline RendererSettingsViewModel BuildRendererSettingsViewModel(const rendern::RendererSettings& rendererSettings)
    {
        RendererSettingsViewModel rendererSettingsViewModel{};
        const bool bIsDeferredEnabled = rendererSettings.enableDeferred;
        const bool bIsFxaaSelected = rendererSettings.antiAliasingMode == 1u;
        const bool bIsHdrEnabled = rendererSettings.enableHDR;
        const bool bIsBloomEnabled = rendererSettings.enableBloom;

        rendererSettingsViewModel.pipeline.enableDeferred = rendererSettings.enableDeferred;
        rendererSettingsViewModel.pipeline.enableDepthPrepass = rendererSettings.enableDepthPrepass;
        rendererSettingsViewModel.pipeline.enableFrustumCulling = rendererSettings.enableFrustumCulling;
        rendererSettingsViewModel.pipeline.debugPrintDrawCalls = rendererSettings.debugPrintDrawCalls;
        rendererSettingsViewModel.pipeline.deferredEditState = rendern::GetDeferredRendererEditState();

        rendererSettingsViewModel.ssao.enableSSAO = rendererSettings.enableSSAO;
        rendererSettingsViewModel.ssao.radius = rendererSettings.ssaoRadius;
        rendererSettingsViewModel.ssao.bias = rendererSettings.ssaoBias;
        rendererSettingsViewModel.ssao.strength = rendererSettings.ssaoStrength;
        rendererSettingsViewModel.ssao.power = rendererSettings.ssaoPower;
        rendererSettingsViewModel.ssao.blurDepthThreshold = rendererSettings.ssaoBlurDepthThreshold;
        rendererSettingsViewModel.ssao.category.isEnabled = true;
        rendererSettingsViewModel.ssao.category.disabledReason = "";
        rendererSettingsViewModel.ssao.sectionEditState = rendern::GetSSAOSectionEditState(rendererSettings);
        rendererSettingsViewModel.ssao.canEditSettings = bIsDeferredEnabled && rendererSettings.enableSSAO;
        rendererSettingsViewModel.ssao.disabledReason = bIsDeferredEnabled
            ? "SSAO controls are available only when SSAO is enabled."
            : rendererSettingsViewModel.ssao.sectionEditState.warning;

        rendererSettingsViewModel.fog.enableFog = rendererSettings.enableFog;
        rendererSettingsViewModel.fog.mode = rendererSettings.fogMode;
        rendererSettingsViewModel.fog.start = rendererSettings.fogStart;
        rendererSettingsViewModel.fog.end = rendererSettings.fogEnd;
        rendererSettingsViewModel.fog.density = rendererSettings.fogDensity;
        rendererSettingsViewModel.fog.color = rendererSettings.fogColor;
        rendererSettingsViewModel.fog.canEditSettings = rendererSettings.enableFog;

        rendererSettingsViewModel.antiAliasing.mode = rendererSettings.antiAliasingMode;
        rendererSettingsViewModel.antiAliasing.fxaaSubpix = rendererSettings.fxaaSubpix;
        rendererSettingsViewModel.antiAliasing.fxaaEdgeThreshold = rendererSettings.fxaaEdgeThreshold;
        rendererSettingsViewModel.antiAliasing.fxaaEdgeThresholdMin = rendererSettings.fxaaEdgeThresholdMin;
        rendererSettingsViewModel.antiAliasing.canEditFxaaSettings = bIsFxaaSelected;
        rendererSettingsViewModel.antiAliasing.fxaaEditState = rendern::GetFxaaControlsEditState(rendererSettings);

        rendererSettingsViewModel.hdrBloom.enableHDR = rendererSettings.enableHDR;
        rendererSettingsViewModel.hdrBloom.toneMapMode = rendererSettings.toneMapMode;
        rendererSettingsViewModel.hdrBloom.hdrExposure = rendererSettings.hdrExposure;
        rendererSettingsViewModel.hdrBloom.enableBloom = rendererSettings.enableBloom;
        rendererSettingsViewModel.hdrBloom.bloomThreshold = rendererSettings.bloomThreshold;
        rendererSettingsViewModel.hdrBloom.bloomSoftKnee = rendererSettings.bloomSoftKnee;
        rendererSettingsViewModel.hdrBloom.bloomIntensity = rendererSettings.bloomIntensity;
        rendererSettingsViewModel.hdrBloom.bloomClamp = rendererSettings.bloomClamp;
        rendererSettingsViewModel.hdrBloom.bloomRadius = rendererSettings.bloomRadius;
        rendererSettingsViewModel.hdrBloom.canEditHdrSettings = bIsHdrEnabled;
        rendererSettingsViewModel.hdrBloom.canEditBloomSettings = bIsHdrEnabled && bIsBloomEnabled;
        rendererSettingsViewModel.hdrBloom.hdrToggleEditState = rendern::GetHDRToggleEditState();
        rendererSettingsViewModel.hdrBloom.hdrControlsEditState = rendern::GetHDRControlsEditState(rendererSettings);
        rendererSettingsViewModel.hdrBloom.bloomControlsEditState = rendern::GetBloomControlsEditState(rendererSettings);

        rendererSettingsViewModel.shadows.showCubeAtlas = rendererSettings.ShowCubeAtlas;
        rendererSettingsViewModel.shadows.debugCubeAtlasIndex = rendererSettings.debugCubeAtlasIndex;
        rendererSettingsViewModel.shadows.debugShadowCubeMapType = rendererSettings.debugShadowCubeMapType;
        rendererSettingsViewModel.shadows.dirShadowBaseBiasTexels = rendererSettings.dirShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.spotShadowBaseBiasTexels = rendererSettings.spotShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.pointShadowBaseBiasTexels = rendererSettings.pointShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.shadowSlopeScaleTexels = rendererSettings.shadowSlopeScaleTexels;

        return rendererSettingsViewModel;
    }

    [[nodiscard]] inline DebugDrawSettingsViewModel BuildDebugDrawSettingsViewModel(
        const rendern::RendererSettings& rendererSettings,
        const rendern::Scene& scene)
    {
        DebugDrawSettingsViewModel debugDrawSettings{};
        debugDrawSettings.showLightGizmos = rendererSettings.drawLightGizmos;
        debugDrawSettings.showPickingRay = scene.debugPickRay.enabled;
        debugDrawSettings.showAnimationRuntimeOverlay = rendererSettings.drawAnimationRuntimeOverlay;
        return debugDrawSettings;
    }

    inline void ApplyDebugDrawSettingsViewModel(
        rendern::RendererSettings& rendererSettings,
        rendern::Scene& scene,
        const DebugDrawSettingsViewModel& debugDrawSettings)
    {
        rendererSettings.drawLightGizmos = debugDrawSettings.showLightGizmos;
        scene.debugPickRay.enabled = debugDrawSettings.showPickingRay;
        rendererSettings.drawAnimationRuntimeOverlay = debugDrawSettings.showAnimationRuntimeOverlay;
    }

}