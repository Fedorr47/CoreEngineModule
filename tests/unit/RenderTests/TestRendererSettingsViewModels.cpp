#include <gtest/gtest.h>

import core;

#include "Render/ImGui/RendererSettingsViewModels.h"

namespace
{
    void ExpectRendererSnapshotMatchesSettings(
        const rendern::RendererSettings& rendererSettings,
        const rendern::ui::RendererSettingsViewModel& viewModel)
    {
        EXPECT_EQ(viewModel.pipeline.enableDeferred, rendererSettings.enableDeferred);
        EXPECT_EQ(viewModel.pipeline.enableDepthPrepass, rendererSettings.enableDepthPrepass);
        EXPECT_EQ(viewModel.pipeline.enableFrustumCulling, rendererSettings.enableFrustumCulling);
        EXPECT_EQ(viewModel.pipeline.debugPrintDrawCalls, rendererSettings.debugPrintDrawCalls);

        EXPECT_EQ(viewModel.ssao.enableSSAO, rendererSettings.enableSSAO);
        EXPECT_FLOAT_EQ(viewModel.ssao.radius, rendererSettings.ssaoRadius);
        EXPECT_FLOAT_EQ(viewModel.ssao.bias, rendererSettings.ssaoBias);
        EXPECT_FLOAT_EQ(viewModel.ssao.strength, rendererSettings.ssaoStrength);
        EXPECT_FLOAT_EQ(viewModel.ssao.power, rendererSettings.ssaoPower);
        EXPECT_FLOAT_EQ(viewModel.ssao.blurDepthThreshold, rendererSettings.ssaoBlurDepthThreshold);

        EXPECT_EQ(viewModel.fog.enableFog, rendererSettings.enableFog);
        EXPECT_EQ(viewModel.fog.mode, rendererSettings.fogMode);
        EXPECT_FLOAT_EQ(viewModel.fog.start, rendererSettings.fogStart);
        EXPECT_FLOAT_EQ(viewModel.fog.end, rendererSettings.fogEnd);
        EXPECT_FLOAT_EQ(viewModel.fog.density, rendererSettings.fogDensity);
        EXPECT_EQ(viewModel.fog.color, rendererSettings.fogColor);

        EXPECT_EQ(viewModel.antiAliasing.mode, rendererSettings.antiAliasingMode);
        EXPECT_FLOAT_EQ(viewModel.antiAliasing.fxaaSubpix, rendererSettings.fxaaSubpix);
        EXPECT_FLOAT_EQ(viewModel.antiAliasing.fxaaEdgeThreshold, rendererSettings.fxaaEdgeThreshold);
        EXPECT_FLOAT_EQ(viewModel.antiAliasing.fxaaEdgeThresholdMin, rendererSettings.fxaaEdgeThresholdMin);

        EXPECT_EQ(viewModel.hdrBloom.enableHDR, rendererSettings.enableHDR);
        EXPECT_EQ(viewModel.hdrBloom.toneMapMode, rendererSettings.toneMapMode);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.hdrExposure, rendererSettings.hdrExposure);
        EXPECT_EQ(viewModel.hdrBloom.enableBloom, rendererSettings.enableBloom);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomThreshold, rendererSettings.bloomThreshold);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomSoftKnee, rendererSettings.bloomSoftKnee);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomIntensity, rendererSettings.bloomIntensity);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomClamp, rendererSettings.bloomClamp);
        EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomRadius, rendererSettings.bloomRadius);

        EXPECT_EQ(viewModel.shadows.showCubeAtlas, rendererSettings.ShowCubeAtlas);
        EXPECT_EQ(viewModel.shadows.debugCubeAtlasIndex, rendererSettings.debugCubeAtlasIndex);
        EXPECT_EQ(viewModel.shadows.debugShadowCubeMapType, rendererSettings.debugShadowCubeMapType);
        EXPECT_FLOAT_EQ(viewModel.shadows.dirShadowBaseBiasTexels, rendererSettings.dirShadowBaseBiasTexels);
        EXPECT_FLOAT_EQ(viewModel.shadows.spotShadowBaseBiasTexels, rendererSettings.spotShadowBaseBiasTexels);
        EXPECT_FLOAT_EQ(viewModel.shadows.pointShadowBaseBiasTexels, rendererSettings.pointShadowBaseBiasTexels);
        EXPECT_FLOAT_EQ(viewModel.shadows.shadowSlopeScaleTexels, rendererSettings.shadowSlopeScaleTexels);
    }
}

// Protects the default pre-ImGui snapshot contract: UI defaults should only change when RendererSettings defaults change.
TEST(RendererSettingsViewModels, DefaultSnapshotMirrorsRendererSettingsDefaults)
{
    const rendern::RendererSettings rendererSettings{};

    const rendern::ui::RendererSettingsViewModel viewModel = rendern::ui::BuildRendererSettingsViewModel(rendererSettings);
    
    ExpectRendererSnapshotMatchesSettings(rendererSettings, viewModel);
    EXPECT_FALSE(viewModel.pipeline.deferredEditState.bCanEditNow);
    EXPECT_FALSE(viewModel.ssao.canEditSettings);
    EXPECT_FALSE(viewModel.fog.canEditSettings);
    EXPECT_FALSE(viewModel.antiAliasing.canEditFxaaSettings);
    EXPECT_TRUE(viewModel.hdrBloom.canEditHdrSettings);
    EXPECT_TRUE(viewModel.hdrBloom.canEditBloomSettings);
}

// Guards the per-frame ViewModel copy contract so drawing code does not need to re-read mutable RendererSettings.
TEST(RendererSettingsViewModels, ModifiedSettingsAreReflectedInSnapshot)
{
    rendern::RendererSettings rendererSettings{};
    rendererSettings.enableDepthPrepass = true;
    rendererSettings.enableDeferred = true;
    rendererSettings.enableFrustumCulling = false;
    rendererSettings.debugPrintDrawCalls = true;
    rendererSettings.enableSSAO = true;
    rendererSettings.ssaoRadius = 2.5f;
    rendererSettings.ssaoBias = 0.04f;
    rendererSettings.ssaoStrength = 1.75f;
    rendererSettings.ssaoPower = 2.25f;
    rendererSettings.ssaoBlurDepthThreshold = 0.01f;
    rendererSettings.enableFog = true;
    rendererSettings.fogMode = 2u;
    rendererSettings.fogStart = 3.0f;
    rendererSettings.fogEnd = 45.0f;
    rendererSettings.fogDensity = 0.12f;
    rendererSettings.fogColor = { 0.2f, 0.3f, 0.4f };
    rendererSettings.antiAliasingMode = 1u;
    rendererSettings.fxaaSubpix = 0.5f;
    rendererSettings.fxaaEdgeThreshold = 0.125f;
    rendererSettings.fxaaEdgeThresholdMin = 0.0312f;
    rendererSettings.enableHDR = true;
    rendererSettings.toneMapMode = 1u;
    rendererSettings.hdrExposure = 1.5f;
    rendererSettings.enableBloom = true;
    rendererSettings.bloomThreshold = 1.25f;
    rendererSettings.bloomSoftKnee = 0.25f;
    rendererSettings.bloomIntensity = 0.2f;
    rendererSettings.bloomClamp = 12.0f;
    rendererSettings.bloomRadius = 2.0f;
    rendererSettings.ShowCubeAtlas = true;
    rendererSettings.debugCubeAtlasIndex = 4u;
    rendererSettings.debugShadowCubeMapType = 0u;
    rendererSettings.dirShadowBaseBiasTexels = 0.8f;
    rendererSettings.spotShadowBaseBiasTexels = 1.5f;
    rendererSettings.pointShadowBaseBiasTexels = 0.7f;
    rendererSettings.shadowSlopeScaleTexels = 3.0f;

    const rendern::ui::RendererSettingsViewModel viewModel = rendern::ui::BuildRendererSettingsViewModel(rendererSettings);
    
    ExpectRendererSnapshotMatchesSettings(rendererSettings, viewModel);
    EXPECT_TRUE(viewModel.ssao.canEditSettings);
    EXPECT_TRUE(viewModel.fog.canEditSettings);
    EXPECT_TRUE(viewModel.antiAliasing.canEditFxaaSettings);
}

// These edit-state flags drive ImGui disabled scopes; regressions here would expose controls for unsupported edits.
TEST(RendererSettingsViewModels, DisabledStatesTrackCurrentViewModelDependencies)
{
    rendern::RendererSettings rendererSettings{};
    rendererSettings.enableDeferred = false;
    rendererSettings.enableSSAO = true;
    rendererSettings.enableFog = false;
    rendererSettings.antiAliasingMode = 0u;
    rendererSettings.enableHDR = false;
    rendererSettings.enableBloom = true;

    const rendern::ui::RendererSettingsViewModel disabledViewModel = rendern::ui::BuildRendererSettingsViewModel(rendererSettings);
    
    EXPECT_FALSE(disabledViewModel.ssao.canEditSettings);
    EXPECT_FALSE(disabledViewModel.ssao.sectionEditState.bCanEditNow);
    EXPECT_FALSE(disabledViewModel.fog.canEditSettings);
    EXPECT_FALSE(disabledViewModel.antiAliasing.canEditFxaaSettings);
    EXPECT_FALSE(disabledViewModel.antiAliasing.fxaaEditState.bCanEditNow);
    EXPECT_FALSE(disabledViewModel.hdrBloom.canEditHdrSettings);
    EXPECT_FALSE(disabledViewModel.hdrBloom.canEditBloomSettings);
    EXPECT_FALSE(disabledViewModel.hdrBloom.hdrControlsEditState.bCanEditNow);
    EXPECT_FALSE(disabledViewModel.hdrBloom.bloomControlsEditState.bCanEditNow);

    rendererSettings.enableDeferred = true;
    rendererSettings.enableFog = true;
    rendererSettings.antiAliasingMode = 1u;
    rendererSettings.enableHDR = true;
    rendererSettings.enableBloom = true;

    const rendern::ui::RendererSettingsViewModel enabledViewModel = rendern::ui::BuildRendererSettingsViewModel(rendererSettings);

    EXPECT_TRUE(enabledViewModel.ssao.canEditSettings);
    EXPECT_TRUE(enabledViewModel.ssao.sectionEditState.bCanEditNow);
    EXPECT_TRUE(enabledViewModel.fog.canEditSettings);
    EXPECT_TRUE(enabledViewModel.antiAliasing.canEditFxaaSettings);
    EXPECT_TRUE(enabledViewModel.antiAliasing.fxaaEditState.bCanEditNow);
    EXPECT_TRUE(enabledViewModel.hdrBloom.canEditHdrSettings);
    EXPECT_TRUE(enabledViewModel.hdrBloom.canEditBloomSettings);
    EXPECT_TRUE(enabledViewModel.hdrBloom.hdrControlsEditState.bCanEditNow);
    EXPECT_TRUE(enabledViewModel.hdrBloom.bloomControlsEditState.bCanEditNow);
}

// Representative UI edits must continue through renderer_settings_commands so clamping remains centralized.
TEST(RendererSettingsViewModels, SnapshotReflectsCommandClampedSettings)
{
    rendern::RendererSettings rendererSettings{};
    
    rendern::renderer_settings_commands::SetSSAORadius(rendererSettings, 99.0f);
    rendern::renderer_settings_commands::SetFogMode(rendererSettings, 99);
    rendern::renderer_settings_commands::SetHDRExposure(rendererSettings, -3.0f);
    rendern::renderer_settings_commands::SetBloomRadius(rendererSettings, 99.0f);
    rendern::renderer_settings_commands::SetShadowBiasSettings(rendererSettings, -1.0f, 99.0f, 99.0f, 99.0f);

    const rendern::ui::RendererSettingsViewModel viewModel = rendern::ui::BuildRendererSettingsViewModel(rendererSettings);

    EXPECT_FLOAT_EQ(viewModel.ssao.radius, 5.0f);
    EXPECT_EQ(viewModel.fog.mode, 2u);
    EXPECT_FLOAT_EQ(viewModel.hdrBloom.hdrExposure, 0.0f);
    EXPECT_FLOAT_EQ(viewModel.hdrBloom.bloomRadius, 4.0f);
    EXPECT_FLOAT_EQ(viewModel.shadows.dirShadowBaseBiasTexels, 0.0f);
    EXPECT_FLOAT_EQ(viewModel.shadows.spotShadowBaseBiasTexels, 10.0f);
    EXPECT_FLOAT_EQ(viewModel.shadows.pointShadowBaseBiasTexels, 10.0f);
    EXPECT_FLOAT_EQ(viewModel.shadows.shadowSlopeScaleTexels, 10.0f);
}

// Protects CR-289's narrow debug-draw boundary: UI reads a copied snapshot and applies edits back explicitly.
TEST(DebugDrawSettingsViewModel, SnapshotAndApplyCoverExistingFlags)
{
    rendern::RendererSettings rendererSettings{};
    rendern::Scene scene{};
    rendererSettings.drawLightGizmos = true;
    rendererSettings.drawAnimationRuntimeOverlay = false;
    scene.debugPickRay.enabled = true;

    const rendern::ui::DebugDrawSettingsViewModel snapshot = rendern::ui::BuildDebugDrawSettingsViewModel(rendererSettings, scene);
    
    EXPECT_TRUE(snapshot.showLightGizmos);
    EXPECT_TRUE(snapshot.showPickingRay);
    EXPECT_FALSE(snapshot.showAnimationRuntimeOverlay);

    rendern::ui::DebugDrawSettingsViewModel editedSnapshot = snapshot;
    editedSnapshot.showLightGizmos = false;
    editedSnapshot.showPickingRay = false;
    editedSnapshot.showAnimationRuntimeOverlay = true;

    rendern::ui::ApplyDebugDrawSettingsViewModel(rendererSettings, scene, editedSnapshot);

    EXPECT_FALSE(rendererSettings.drawLightGizmos);
    EXPECT_FALSE(scene.debugPickRay.enabled);
    EXPECT_TRUE(rendererSettings.drawAnimationRuntimeOverlay);
}