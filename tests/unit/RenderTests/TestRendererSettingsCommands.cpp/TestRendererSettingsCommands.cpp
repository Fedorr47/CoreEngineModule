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