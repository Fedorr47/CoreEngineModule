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

    struct RendererSettingsViewModel
    {
        RendererPipelineSettingsViewModel pipeline{};
        SSAOSettingsViewModel ssao{};
        FogSettingsViewModel fog{};
        AntiAliasingSettingsViewModel antiAliasing{};
        HdrBloomSettingsViewModel hdrBloom{};
        ShadowSettingsViewModel shadows{};
    };
    
    [[nodiscard]] static RendererSettingsViewModel BuildRendererSettingsViewModel(const rendern::RendererSettings& rendererSettings)
    {
        RendererSettingsViewModel rendererSettingsViewModel{};

        rendererSettingsViewModel.pipeline.enableDeferred = rendererSettings.enableDeferred;
        rendererSettingsViewModel.pipeline.enableDepthPrepass = rendererSettings.enableDepthPrepass;
        rendererSettingsViewModel.pipeline.enableFrustumCulling = rendererSettings.enableFrustumCulling;
        rendererSettingsViewModel.pipeline.debugPrintDrawCalls = rendererSettings.debugPrintDrawCalls;

        rendererSettingsViewModel.ssao.enableSSAO = rendererSettings.enableSSAO;
        rendererSettingsViewModel.ssao.radius = rendererSettings.ssaoRadius;
        rendererSettingsViewModel.ssao.bias = rendererSettings.ssaoBias;
        rendererSettingsViewModel.ssao.strength = rendererSettings.ssaoStrength;
        rendererSettingsViewModel.ssao.power = rendererSettings.ssaoPower;
        rendererSettingsViewModel.ssao.blurDepthThreshold = rendererSettings.ssaoBlurDepthThreshold;
        rendererSettingsViewModel.ssao.category.isEnabled = rendererSettings.enableDeferred;
        rendererSettingsViewModel.ssao.category.disabledReason = "SSAO is currently exposed only for the deferred renderer path.";
        rendererSettingsViewModel.ssao.canEditSettings = rendererSettings.enableDeferred && rendererSettings.enableSSAO;
        rendererSettingsViewModel.ssao.disabledReason = rendererSettings.enableDeferred
            ? "SSAO controls are available only when SSAO is enabled."
            : rendererSettingsViewModel.ssao.category.disabledReason;

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
        rendererSettingsViewModel.antiAliasing.canEditFxaaSettings = rendererSettings.antiAliasingMode == 1u;

        rendererSettingsViewModel.hdrBloom.enableHDR = rendererSettings.enableHDR;
        rendererSettingsViewModel.hdrBloom.toneMapMode = rendererSettings.toneMapMode;
        rendererSettingsViewModel.hdrBloom.hdrExposure = rendererSettings.hdrExposure;
        rendererSettingsViewModel.hdrBloom.enableBloom = rendererSettings.enableBloom;
        rendererSettingsViewModel.hdrBloom.bloomThreshold = rendererSettings.bloomThreshold;
        rendererSettingsViewModel.hdrBloom.bloomSoftKnee = rendererSettings.bloomSoftKnee;
        rendererSettingsViewModel.hdrBloom.bloomIntensity = rendererSettings.bloomIntensity;
        rendererSettingsViewModel.hdrBloom.bloomClamp = rendererSettings.bloomClamp;
        rendererSettingsViewModel.hdrBloom.bloomRadius = rendererSettings.bloomRadius;
        rendererSettingsViewModel.hdrBloom.canEditHdrSettings = rendererSettings.enableHDR;
        rendererSettingsViewModel.hdrBloom.canEditBloomSettings = rendererSettings.enableHDR && rendererSettings.enableBloom;

        rendererSettingsViewModel.shadows.showCubeAtlas = rendererSettings.ShowCubeAtlas;
        rendererSettingsViewModel.shadows.debugCubeAtlasIndex = rendererSettings.debugCubeAtlasIndex;
        rendererSettingsViewModel.shadows.debugShadowCubeMapType = rendererSettings.debugShadowCubeMapType;
        rendererSettingsViewModel.shadows.dirShadowBaseBiasTexels = rendererSettings.dirShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.spotShadowBaseBiasTexels = rendererSettings.spotShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.pointShadowBaseBiasTexels = rendererSettings.pointShadowBaseBiasTexels;
        rendererSettingsViewModel.shadows.shadowSlopeScaleTexels = rendererSettings.shadowSlopeScaleTexels;

        return rendererSettingsViewModel;
    }
    
    static void DrawSSAOSection(rendern::RendererSettings& rendererSettings, const SSAOSettingsViewModel& ssaoSettings)
    {
        if (!ssaoSettings.category.isEnabled)
            return;

        if (!ImGui::CollapsingHeader(ssaoSettings.category.displayLabel, ImGuiTreeNodeFlags_DefaultOpen))
            return;

        bool bShouldEnableSSAO = ssaoSettings.enableSSAO;
        if (ImGui::Checkbox("Enable SSAO", &bShouldEnableSSAO))
        {
            rendern::renderer_settings_commands::SetSSAOEnabled(rendererSettings, bShouldEnableSSAO);
        }
        ImGui::BeginDisabled(!ssaoSettings.canEditSettings);

        float requestedSSAORadius = ssaoSettings.radius;
        if (ImGui::SliderFloat("Radius", &requestedSSAORadius, 0.05f, 5.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetSSAORadius(rendererSettings, requestedSSAORadius);
        }
        float requestedSSAOBias = ssaoSettings.bias;
        if (ImGui::SliderFloat("Bias", &requestedSSAOBias, 0.0f, 0.25f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetSSAOBias(rendererSettings, requestedSSAOBias);
        }
        float requestedSSAOStrength = ssaoSettings.strength;
        if (ImGui::SliderFloat("Strength", &requestedSSAOStrength, 0.0f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetSSAOStrength(rendererSettings, requestedSSAOStrength);
        }
        float requestedSSAOPower = ssaoSettings.power;
        if (ImGui::SliderFloat("Power", &requestedSSAOPower, 0.5f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetSSAOPower(rendererSettings, requestedSSAOPower);
        }
        float requestedSSAOBlurDepthThreshold = ssaoSettings.blurDepthThreshold;
        if (ImGui::SliderFloat("Blur depth threshold", &requestedSSAOBlurDepthThreshold, 0.0f, 0.02f, "%.5f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetSSAOBlurDepthThreshold(rendererSettings, requestedSSAOBlurDepthThreshold);
        }

        if (ImGui::Button("SSAO defaults"))
        {
            rendern::renderer_settings_commands::ResetSSAODefaults(rendererSettings);
        }

        ImGui::EndDisabled();
        ImGui::Separator();
    }

    static void DrawFogSection(rendern::RendererSettings& rendererSettings, const FogSettingsViewModel& fogSettings)
    {
        if (!ImGui::CollapsingHeader(fogSettings.category.displayLabel, ImGuiTreeNodeFlags_DefaultOpen))
            return;

        bool bShouldEnableFog = fogSettings.enableFog;
        if (ImGui::Checkbox("Enable fog", &bShouldEnableFog))
        {
            rendern::renderer_settings_commands::SetFogEnabled(rendererSettings, bShouldEnableFog);
        }
        ImGui::BeginDisabled(!fogSettings.canEditSettings);

        const char* items[] = { "Linear", "Exp", "Exp2" };
        int requestedFogMode = static_cast<int>(fogSettings.mode);
        if (ImGui::Combo("Mode##Fog", &requestedFogMode, items, IM_ARRAYSIZE(items)))
        {
            rendern::renderer_settings_commands::SetFogMode(rendererSettings, requestedFogMode);
        }

        if (fogSettings.mode == 0u)
        {
            float requestedFogStart = fogSettings.start;
            float requestedFogEnd = fogSettings.end;
            const bool bChangedFogStart = ImGui::SliderFloat("Start", &requestedFogStart, 0.0f, 500.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            const bool bChangedFogEnd = ImGui::SliderFloat("End", &requestedFogEnd, 0.0f, 500.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (bChangedFogStart || bChangedFogEnd)
            {
                rendern::renderer_settings_commands::SetFogRange(rendererSettings, requestedFogStart, requestedFogEnd);
            }
        }
        else
        {
            float requestedFogDensity = fogSettings.density;
            if (ImGui::SliderFloat("Density", &requestedFogDensity, 0.0f, 0.25f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
            {
                rendern::renderer_settings_commands::SetFogDensity(rendererSettings, requestedFogDensity);
            }
        }

        std::array<float, 3> requestedFogColor = fogSettings.color;
        if (ImGui::ColorEdit3("Color", requestedFogColor.data()))
        {
            rendern::renderer_settings_commands::SetFogColor(rendererSettings, requestedFogColor);
        }

        if (ImGui::Button("Fog defaults"))
        {
            rendern::renderer_settings_commands::ResetFogDefaults(rendererSettings);
        }

        ImGui::EndDisabled();
        ImGui::Separator();
    }

    static void DrawAntiAliasingSection(
        rendern::RendererSettings& rendererSettings, 
        const AntiAliasingSettingsViewModel& antiAliasingSettings)
    {
        if (!ImGui::CollapsingHeader(antiAliasingSettings.category.displayLabel, ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const char* items[] = { "None", "FXAA" };
        int requestedAntiAliasingMode = static_cast<int>(antiAliasingSettings.mode);
        if (ImGui::Combo("Mode##AA", &requestedAntiAliasingMode, items, IM_ARRAYSIZE(items)))
        {
            rendern::renderer_settings_commands::SetAntiAliasingMode(rendererSettings, requestedAntiAliasingMode);
        }

        ImGui::BeginDisabled(!antiAliasingSettings.canEditFxaaSettings);
        float requestedFxaaSubpix = antiAliasingSettings.fxaaSubpix;
        if (ImGui::SliderFloat("FXAA Subpix", &requestedFxaaSubpix, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetFxaaSubpix(rendererSettings, requestedFxaaSubpix);
        }
        float requestedFxaaEdgeThreshold = antiAliasingSettings.fxaaEdgeThreshold;
        if (ImGui::SliderFloat("FXAA Edge Threshold", &requestedFxaaEdgeThreshold, 0.0312f, 0.333f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetFxaaEdgeThreshold(rendererSettings, requestedFxaaEdgeThreshold);
        }
        float requestedFxaaEdgeThresholdMin = antiAliasingSettings.fxaaEdgeThresholdMin;
        if (ImGui::SliderFloat("FXAA Edge Threshold Min", &requestedFxaaEdgeThresholdMin, 0.0f, 0.125f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetFxaaEdgeThresholdMin(rendererSettings, requestedFxaaEdgeThresholdMin);
        }

        if (ImGui::Button("FXAA defaults"))
        {
            rendern::renderer_settings_commands::ResetFxaaDefaults(rendererSettings);
        }

        ImGui::EndDisabled();
        ImGui::Separator();
    }

    static void DrawHdrBloomSection(
        rendern::RendererSettings& rendererSettings, 
        const HdrBloomSettingsViewModel& hdrBloomSettings)
    {
        if (!ImGui::CollapsingHeader(hdrBloomSettings.category.displayLabel, ImGuiTreeNodeFlags_DefaultOpen))
            return;

        bool bShouldEnableHDR = hdrBloomSettings.enableHDR;
        if (ImGui::Checkbox("Enable HDR", &bShouldEnableHDR))
        {
            rendern::renderer_settings_commands::SetHDREnabled(rendererSettings, bShouldEnableHDR);
        }
        ImGui::BeginDisabled(!hdrBloomSettings.canEditHdrSettings);

        const char* toneItems[] = { "Linear", "Reinhard", "ACES" };
        int requestedToneMapMode = static_cast<int>(hdrBloomSettings.toneMapMode);
        if (ImGui::Combo("Tonemap", &requestedToneMapMode, toneItems, IM_ARRAYSIZE(toneItems)))
        {
            rendern::renderer_settings_commands::SetToneMapMode(rendererSettings, requestedToneMapMode);
        }

        float requestedHDRExposure = hdrBloomSettings.hdrExposure;
        if (ImGui::SliderFloat("Exposure", &requestedHDRExposure, 0.1f, 8.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetHDRExposure(rendererSettings, requestedHDRExposure);
        }
        bool bShouldEnableBloom = hdrBloomSettings.enableBloom;
        if (ImGui::Checkbox("Enable Bloom", &bShouldEnableBloom))
        {
            rendern::renderer_settings_commands::SetBloomEnabled(rendererSettings, bShouldEnableBloom);
        }

        ImGui::BeginDisabled(!hdrBloomSettings.canEditBloomSettings);
        float requestedBloomThreshold = hdrBloomSettings.bloomThreshold;
        if (ImGui::SliderFloat("Bloom threshold", &requestedBloomThreshold, 0.1f, 8.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetBloomThreshold(rendererSettings, requestedBloomThreshold);
        }
        float requestedBloomSoftKnee = hdrBloomSettings.bloomSoftKnee;
        if (ImGui::SliderFloat("Bloom soft knee", &requestedBloomSoftKnee, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetBloomSoftKnee(rendererSettings, requestedBloomSoftKnee);
        }
        float requestedBloomIntensity = hdrBloomSettings.bloomIntensity;
        if (ImGui::SliderFloat("Bloom intensity", &requestedBloomIntensity, 0.0f, 1.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetBloomIntensity(rendererSettings, requestedBloomIntensity);
        }
        float requestedBloomClamp = hdrBloomSettings.bloomClamp;
        if (ImGui::SliderFloat("Bloom clamp", &requestedBloomClamp, 1.0f, 64.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetBloomClamp(rendererSettings, requestedBloomClamp);
        }
        float requestedBloomRadius = hdrBloomSettings.bloomRadius;
        if (ImGui::SliderFloat("Bloom radius", &requestedBloomRadius, 0.25f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            rendern::renderer_settings_commands::SetBloomRadius(rendererSettings, requestedBloomRadius);
        }
        ImGui::EndDisabled();

        if (ImGui::Button("HDR/Bloom defaults"))
        {
            rendern::renderer_settings_commands::ResetHDRBloomDefaults(rendererSettings);
        }

        ImGui::EndDisabled();
        ImGui::Separator();
    }

    static void DrawCameraDebugSection(rendern::Scene& scene, rendern::CameraController& camCtl)
    {
        if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        rendern::Camera& cam = scene.camera;

        if (DragVec3("Position", cam.position, 0.05f))
        {
            cam.target = cam.position + camCtl.Forward();
        }
        if (DragVec3("Target", cam.target, 0.05f))
        {
            camCtl.ResetFromCamera(cam);
        }

        constexpr float kRadToDeg = 57.29577951308232f;
        constexpr float kDegToRad = 0.017453292519943295f;

        float yawDeg = camCtl.YawRad() * kRadToDeg;
        float pitchDeg = camCtl.PitchRad() * kRadToDeg;

        bool changedAngles = false;
        changedAngles |= ImGui::SliderFloat("Yaw (deg)", &yawDeg, -180.0f, 180.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        changedAngles |= ImGui::SliderFloat("Pitch (deg)", &pitchDeg, -89.0f, 89.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

        if (changedAngles)
        {
            camCtl.SetYawPitchRad(yawDeg * kDegToRad, pitchDeg * kDegToRad, cam);
        }

        ImGui::SliderFloat("FOV Y (deg)", &cam.fovYDeg, 20.0f, 120.0f);
        ImGui::InputFloat("Near Z", &cam.nearZ, 0.01f, 0.1f, "%.4f");
        ImGui::InputFloat("Far Z", &cam.farZ, 1.0f, 10.0f, "%.1f");

        auto& s = camCtl.Settings();

        bool enabledCtl = camCtl.Enabled();
        if (ImGui::Checkbox("Enable controller", &enabledCtl))
        {
            camCtl.SetEnabled(enabledCtl);
        }
        ImGui::Checkbox("Invert Y", &s.invertY);
        ImGui::SliderFloat("Move speed", &s.moveSpeed, 0.1f, 50.0f);
        ImGui::SliderFloat("Sprint multiplier", &s.sprintMultiplier, 1.0f, 12.0f);
        ImGui::SliderFloat("Mouse sensitivity", &s.mouseSensitivity, 0.0005f, 0.01f, "%.4f", ImGuiSliderFlags_Logarithmic);

        if (ImGui::Button("Reset view"))
        {
            cam.position = mathUtils::Vec3(5.0f, 10.0f, 10.0f);
            cam.target = mathUtils::Vec3(0.0f, 0.0f, 0.0f);
            cam.up = mathUtils::Vec3(0.0f, 1.0f, 0.0f);
            cam.fovYDeg = 60.0f;
            cam.nearZ = 0.01f;
            cam.farZ = 200.0f;
            camCtl.ResetFromCamera(cam);
        }

        ImGui::TextDisabled("Controls: hold RMB to look, WASD move, QE up/down, Shift sprint");
    }

    static void DrawShadowAndDebugSection(
        rendern::RendererSettings& rs,
        rendern::Scene& scene,
        const ShadowSettingsViewModel& shadowSettings)
    {
        int current = static_cast<int>(shadowSettings.debugShadowCubeMapType);
        std::vector<const char*> citems;
        citems.reserve(2);
        citems.push_back("Point");
        citems.push_back("Reflection");

        ImGui::Separator();
        ImGui::Text("Shadow cube atlas");
        bool bShouldShowCubeAtlas = shadowSettings.showCubeAtlas;
        if (ImGui::Checkbox("Show cube atlas", &bShouldShowCubeAtlas))
        {
            rendern::renderer_settings_commands::SetShowCubeAtlas(rs, bShouldShowCubeAtlas);
        }

        int debugCubeAtlasIndex = static_cast<int>(shadowSettings.debugCubeAtlasIndex);
        if (ImGui::Combo("Type", &current, citems.data(), static_cast<int>(citems.size())))
        {
            rendern::renderer_settings_commands::SetDebugShadowCubeMapType(rs, current);
        }

        if (current == 1)
        {
            int reflectiveOwnerCount = 0;
            for (const auto& di : scene.drawItems)
            {
                if (di.material.id == 0)
                    continue;
                const auto& mat = scene.GetMaterial(di.material);
                if (mat.envSource == EnvSource::ReflectionCapture)
                    ++reflectiveOwnerCount;
            }

            ImGui::TextDisabled("Reflection owner index among reflective objects (count: %d)", reflectiveOwnerCount);
            ImGui::TextDisabled("Debug atlas index now selects which reflective owner is captured/shown.");
            if (scene.editorReflectionCaptureOwnerNode >= 0)
            {
                ImGui::TextDisabled("In reflection atlas debug mode, the debug owner index overrides the explicit capture owner.");
            }
        }

        const char* debugIndexLabel = (current == 0) ? "Point cube index" : "Reflection owner index";
        if (ImGui::InputInt(debugIndexLabel, &debugCubeAtlasIndex))
        {
            rendern::renderer_settings_commands::SetDebugCubeAtlasIndex(rs, debugCubeAtlasIndex);
        }

        ImGui::Separator();
        ImGui::Text("Shadow bias (texels)");
        float requestedDirShadowBaseBiasTexels = shadowSettings.dirShadowBaseBiasTexels;
        float requestedSpotShadowBaseBiasTexels = shadowSettings.spotShadowBaseBiasTexels;
        float requestedPointShadowBaseBiasTexels = shadowSettings.pointShadowBaseBiasTexels;
        float requestedShadowSlopeScaleTexels = shadowSettings.shadowSlopeScaleTexels;
        const bool bChangedDirShadowBaseBias = ImGui::SliderFloat("Dir base", &requestedDirShadowBaseBiasTexels, 0.0f, 5.0f, "%.3f");
        const bool bChangedSpotShadowBaseBias = ImGui::SliderFloat("Spot base", &requestedSpotShadowBaseBiasTexels, 0.0f, 10.0f, "%.3f");
        const bool bChangedPointShadowBaseBias = ImGui::SliderFloat("Point base", &requestedPointShadowBaseBiasTexels, 0.0f, 10.0f, "%.3f");
        const bool bChangedShadowSlopeScale = ImGui::SliderFloat("Slope scale", &requestedShadowSlopeScaleTexels, 0.0f, 10.0f, "%.3f");
        if (bChangedDirShadowBaseBias || bChangedSpotShadowBaseBias || bChangedPointShadowBaseBias || bChangedShadowSlopeScale)
        {
            rendern::renderer_settings_commands::SetShadowBiasSettings(
                rs,
                requestedDirShadowBaseBiasTexels,
                requestedSpotShadowBaseBiasTexels,
                requestedPointShadowBaseBiasTexels,
                requestedShadowSlopeScaleTexels);
        }

        ImGui::Separator();
        ImGui::Text("Debug draw");
        ImGui::Checkbox("Light gizmos", &rs.drawLightGizmos);
        ImGui::Checkbox("Gameplay movement", &rs.drawGameplayMovementDebug);
        ImGui::Checkbox("Performance panel", &rs.showPerformancePanel);
        ImGui::Checkbox("Log CPU frame timings", &rs.logCpuFrameTimings);
        ImGui::Checkbox("VSync", &rs.enableVSync);
        ImGui::Checkbox("Render Debug Window Swapchain", &rs.enableDebugWindowRender);
        ImGui::TextDisabled("F7: toggle separate debug window swapchain rendering.");
       
        if (rs.drawGameplayMovementDebug)
        {
            ImGui::Checkbox("Movement debug: controlled only", &rs.drawGameplayMovementDebugOnlyControlled);
            ImGui::Checkbox("Movement labels", &rs.drawGameplayMovementDebugLabels);
            ImGui::Checkbox("Movement speed text", &rs.drawGameplayMovementDebugText);
            ImGui::SliderFloat("Velocity scale", &rs.gameplayMovementVelocityScale, 0.05f, 3.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Target velocity scale", &rs.gameplayMovementTargetVelocityScale, 0.05f, 3.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Desired move scale", &rs.gameplayMovementDesiredMoveScale, 0.05f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Facing scale", &rs.gameplayMovementFacingScale, 0.05f, 4.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Movement lift", &rs.gameplayMovementLift, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Movement label scale", &rs.gameplayMovementLabelScale, 0.5f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Movement text scale", &rs.gameplayMovementTextScale, 0.5f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        }
        ImGui::Checkbox("Animation runtime overlay", &rs.drawAnimationRuntimeOverlay);
        if (rs.drawAnimationRuntimeOverlay)
        {
            ImGui::Checkbox("Animation overlay: controlled only", &rs.drawAnimationRuntimeOverlayOnlyControlled);
            ImGui::SliderFloat("Animation overlay text scale", &rs.animationRuntimeOverlayTextScale, 0.5f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Animation overlay X", &rs.animationRuntimeOverlayAnchorXPx, 0.0f, 4096.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Animation overlay Y", &rs.animationRuntimeOverlayAnchorYPx, 0.0f, 4096.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TextDisabled("Hotkey: F6 toggles overlay in-game.");
        }
        ImGui::Checkbox("Planar mirror normals", &rs.drawPlanarMirrorNormals);
        if (rs.drawPlanarMirrorNormals)
        {
            ImGui::SliderFloat("Planar normal length", &rs.planarMirrorNormalLength, 0.05f, 20.0f, "%.3f");
        }
        ImGui::BeginDisabled(!rs.drawLightGizmos);
        ImGui::Checkbox("Depth test (main view)", &rs.debugDrawDepthTest);
        ImGui::SliderFloat("Gizmo half-size", &rs.lightGizmoHalfSize, 0.01f, 2.0f, "%.3f");
        ImGui::SliderFloat("Arrow length", &rs.lightGizmoArrowLength, 0.05f, 25.0f, "%.3f");
        ImGui::SliderFloat("Arrow thickness (UI only)", &rs.lightGizmoArrowThickness, 0.001f, 2.0f, "%.3f");
        ImGui::EndDisabled();
    }

    static void DrawRendererCoreWindow(
        rendern::RendererSettings& rs,
        rendern::Scene& scene,
        rendern::CameraController& camCtl)
    {
        const RendererSettingsViewModel rendererSettingsViewModel = BuildRendererSettingsViewModel(rs);
        
        ImGui::Begin("Renderer / Shadows");

        bool bShouldEnableDepthPrepass = rendererSettingsViewModel.pipeline.enableDepthPrepass;
        if (ImGui::Checkbox("Depth prepass", &bShouldEnableDepthPrepass))
        {
            rendern::renderer_settings_commands::SetDepthPrepassEnabled(rs, bShouldEnableDepthPrepass);
        }
        bool bShouldEnableDeferred = rendererSettingsViewModel.pipeline.enableDeferred;
        if (ImGui::Checkbox("Deferred (experimental)", &bShouldEnableDeferred))
        {
            rendern::renderer_settings_commands::SetDeferredEnabled(rs, bShouldEnableDeferred);
        }
        bool bShouldEnableFrustumCulling = rendererSettingsViewModel.pipeline.enableFrustumCulling;
        if (ImGui::Checkbox("Frustum culling", &bShouldEnableFrustumCulling))
        {
            rendern::renderer_settings_commands::SetFrustumCullingEnabled(rs, bShouldEnableFrustumCulling);
        }
        bool bShouldDebugPrintDrawCalls = rendererSettingsViewModel.pipeline.debugPrintDrawCalls;
        if (ImGui::Checkbox("Debug print draw calls", &bShouldDebugPrintDrawCalls))
        {
            rendern::renderer_settings_commands::SetDebugPrintDrawCallsEnabled(rs, bShouldDebugPrintDrawCalls);
        }

        DrawSSAOSection(rs, rendererSettingsViewModel.ssao);
        DrawFogSection(rs, rendererSettingsViewModel.fog);
        DrawAntiAliasingSection(rs, rendererSettingsViewModel.antiAliasing);
        DrawHdrBloomSection(rs, rendererSettingsViewModel.hdrBloom);

        DrawCameraDebugSection(scene, camCtl);
        DrawShadowAndDebugSection(rs, scene, rendererSettingsViewModel.shadows);

        ImGui::Separator();
        ImGui::TextDisabled("F1: toggle UI");
        ImGui::End();
    }
}
