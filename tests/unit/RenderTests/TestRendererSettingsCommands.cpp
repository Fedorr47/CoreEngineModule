#include <gtest/gtest.h>

import core;

TEST(RendererSettingsCommands, DeferredCommandMutatesOnlyDeferredFlag)
{
    rendern::RendererSettings rendererSettings{};
    rendererSettings.enableDepthPrepass = true;
    rendererSettings.enableDeferred = false;
    rendererSettings.enableFrustumCulling = true;
    rendererSettings.debugPrintDrawCalls = true;

    rendern::renderer_settings_commands::SetDeferredEnabled(rendererSettings, true);

    EXPECT_TRUE(rendererSettings.enableDeferred);
    EXPECT_TRUE(rendererSettings.enableDepthPrepass);
    EXPECT_TRUE(rendererSettings.enableFrustumCulling);
    EXPECT_TRUE(rendererSettings.debugPrintDrawCalls);
}

TEST(RendererSettingsCommands, FogCommandsPreserveUiSanitizationContracts)
{
    rendern::RendererSettings rendererSettings{};

    rendern::renderer_settings_commands::SetFogMode(rendererSettings, 99);
    EXPECT_EQ(rendererSettings.fogMode, 2u);

    rendern::renderer_settings_commands::SetFogMode(rendererSettings, -5);
    EXPECT_EQ(rendererSettings.fogMode, 0u);

    // Linear fog edits historically swapped the endpoints after ImGui clamped them.
    rendern::renderer_settings_commands::SetFogRange(rendererSettings, 90.0f, 10.0f);
    EXPECT_FLOAT_EQ(rendererSettings.fogStart, 10.0f);
    EXPECT_FLOAT_EQ(rendererSettings.fogEnd, 90.0f);
}

TEST(RendererSettingsCommands, AntiAliasingAndShadowDebugIndicesAreClamped)
{
    rendern::RendererSettings rendererSettings{};

    rendern::renderer_settings_commands::SetAntiAliasingMode(rendererSettings, 42);
    EXPECT_EQ(rendererSettings.antiAliasingMode, 1u);

    rendern::renderer_settings_commands::SetAntiAliasingMode(rendererSettings, -7);
    EXPECT_EQ(rendererSettings.antiAliasingMode, 0u);

    rendern::renderer_settings_commands::SetDebugShadowCubeMapType(rendererSettings, 42);
    EXPECT_EQ(rendererSettings.debugShadowCubeMapType, 1u);

    rendern::renderer_settings_commands::SetDebugCubeAtlasIndex(rendererSettings, -12);
    EXPECT_EQ(rendererSettings.debugCubeAtlasIndex, 0u);
}

TEST(RendererSettingsCommands, EditPolicyMetadataBlocksUnsafeRuntimeToggles)
{
    const rendern::RendererSettingEditState deferredEditState = rendern::GetDeferredRendererEditState();
    EXPECT_EQ(deferredEditState.editPolicy, rendern::RendererSettingEditPolicy::RequiresResourceRebuild);
    EXPECT_FALSE(deferredEditState.bCanEditNow);

    const rendern::RendererSettingEditState hdrToggleEditState = rendern::GetHDRToggleEditState();
    EXPECT_EQ(hdrToggleEditState.editPolicy, rendern::RendererSettingEditPolicy::RequiresResourceRebuild);
    EXPECT_FALSE(hdrToggleEditState.bCanEditNow);
}

TEST(RendererSettingsCommands, EditPolicyMetadataTracksDependencyDisabledControls)
{
    rendern::RendererSettings rendererSettings{};
    rendererSettings.enableDeferred = false;
    rendererSettings.antiAliasingMode = 0u;
    rendererSettings.enableHDR = false;
    rendererSettings.enableBloom = true;

    EXPECT_FALSE(rendern::GetSSAOSectionEditState(rendererSettings).bCanEditNow);
    EXPECT_FALSE(rendern::GetFxaaControlsEditState(rendererSettings).bCanEditNow);
    EXPECT_FALSE(rendern::GetHDRControlsEditState(rendererSettings).bCanEditNow);
    EXPECT_FALSE(rendern::GetBloomControlsEditState(rendererSettings).bCanEditNow);

    rendererSettings.enableDeferred = true;
    rendererSettings.antiAliasingMode = 1u;
    rendererSettings.enableHDR = true;
    rendererSettings.enableBloom = true;

    EXPECT_TRUE(rendern::GetSSAOSectionEditState(rendererSettings).bCanEditNow);
    EXPECT_TRUE(rendern::GetFxaaControlsEditState(rendererSettings).bCanEditNow);
    EXPECT_TRUE(rendern::GetHDRControlsEditState(rendererSettings).bCanEditNow);
    EXPECT_TRUE(rendern::GetBloomControlsEditState(rendererSettings).bCanEditNow);
}
