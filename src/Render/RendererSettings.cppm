module;

#include <filesystem>
#include <cstdint>
#include <array>
#include <cstddef>
#include <algorithm>
#include <utility>

// Shared, backend-agnostic renderer settings.
export module core:renderer_settings;

import :rhi;

export namespace rendern
{
	struct CpuFrameStageTimingSnapshot
	{
		float totalBeforeSleepMs{ 0.0f };
		float totalWithSleepMs{ 0.0f };
		float updateFrameTimingMs{ 0.0f };
		float inputMs{ 0.0f };
		float editorInteractionMs{ 0.0f };
		float streamingMs{ 0.0f };
		float gameplayAndAnimationMs{ 0.0f };
		float buildImGuiMs{ 0.0f };
		float renderMainViewportMs{ 0.0f };
		float renderDebugWindowMs{ 0.0f };
		float tinySleepMs{ 0.0f };
	};

	struct RendererCpuTimingSnapshot
	{
		double renderGraphBuildMs{ 0.0 };
		double renderGraphExecuteTotalMs{ 0.0 };
		double renderGraphSubmitCommandListMs{ 0.0 };
		double presentMs{ 0.0 };
		double setupCsmMs{ 0.0 };
		double shadowPassBuildMs{ 0.0 };
		double reflectionCaptureBuildMs{ 0.0 };
		double mainPassBuildMs{ 0.0 };
		rhi::PresentDiagnostics presentDiagnostics{};
	};

	struct PerformanceSnapshot
	{
		static constexpr std::size_t FrameTimeHistoryCapacity = 128u;

		float fps{ 0.0f };
		float frameTimeMs{ 0.0f };
		float rawFrameTimeMs{ 0.0f };
		CpuFrameStageTimingSnapshot cpuFrameStages{};
		RendererCpuTimingSnapshot rendererCpuTimings{};
		bool hasFrameSample{ false };
		bool hasGpuTimings{ false };
		std::array<float, FrameTimeHistoryCapacity> frameTimeHistoryMs{};
		std::size_t frameTimeHistoryCount{ 0u };
		std::size_t nextFrameTimeHistoryIndex{ 0u };

		void PushFrameTimeSample(float sampleFrameTimeMs) noexcept
		{
			if (sampleFrameTimeMs <= 0.0f)
			{
				return;
			}

			frameTimeHistoryMs[nextFrameTimeHistoryIndex] = sampleFrameTimeMs;
			nextFrameTimeHistoryIndex = (nextFrameTimeHistoryIndex + 1u) % FrameTimeHistoryCapacity;
			if (frameTimeHistoryCount < FrameTimeHistoryCapacity)
			{
				++frameTimeHistoryCount;
			}
			hasFrameSample = true;
		}
	};

	struct RendererSettings
	{
		float dirShadowBaseBiasTexels{ 0.6f };
		float spotShadowBaseBiasTexels{ 1.0f };
		float pointShadowBaseBiasTexels{ 0.4f };
		float shadowSlopeScaleTexels{ 2.0f };

		// Directional shadow cascade settings (DX12-only usage; safe to ignore in other backends)
		float dirShadowDistance{ 200.0f };
		std::uint32_t dirShadowCascadeCount{ 3 };
		float dirShadowSplitLambda{ 0.7f };
		bool enableDepthPrepass{ false };
		bool enableDeferred{ false }; // DX12-only (currently): GBuffer + fullscreen resolve
		bool enableFrustumCulling{ true };
		bool debugPrintDrawCalls{ false }; // prints MainPass draw-call count (DX12) once per ~60 frames

		// SSAO (DX12 deferred path). Applied as a multiplicative factor to AO/ambient.
		bool enableSSAO{ true };
		float ssaoRadius{ 1.0f };               // world units (meters in your convention)
		float ssaoBias{ 0.02f };                // world units
		float ssaoStrength{ 1.25f };            // intensity multiplier
		float ssaoPower{ 1.5f };                // contrast curve
		float ssaoBlurDepthThreshold{ 0.0025f };// depth compare threshold in 0..1 depth space

		// Fog (post effect). Applied after Skybox/Planar, before editor selection + transparents.
		bool enableFog{ false };
		std::uint32_t fogMode{ 0u }; // 0=Linear, 1=Exp, 2=Exp2
		float fogStart{ 15.0f };
		float fogEnd{ 80.0f };
		float fogDensity{ 0.02f };
		std::array<float, 3> fogColor{ 0.60f, 0.70f, 0.80f };

		// HDR / tone mapping / bloom.
		std::uint32_t antiAliasingMode{ 0u }; // 0=None, 1=FXAA
		float fxaaSubpix{ 0.75f };
		float fxaaEdgeThreshold{ 0.166f };
		float fxaaEdgeThresholdMin{ 0.0833f };
		bool enableHDR{ true };
		std::uint32_t toneMapMode{ 2u }; // 0=None, 1=Reinhard, 2=ACES
		float hdrExposure{ 1.0f };
		bool enableBloom{ true };
		float bloomThreshold{ 1.0f };
		float bloomSoftKnee{ 0.5f };
		float bloomIntensity{ 0.08f };
		float bloomClamp{ 16.0f };
		float bloomRadius{ 1.0f };

		// Reflection capture (cubemap). Currently used by DX12 backend.
		bool enableReflectionCapture{ true };
		bool reflectionCaptureUpdateEveryFrame{ true };
		bool reflectionCaptureFollowSelectedObject{ false };
		std::uint32_t reflectionCaptureResolution{ 1024 }; // cube face size (px)
		float reflectionCaptureNearZ{ 0.05f };
		float reflectionCaptureFarZ{ 200.0f };

		// Parallax-corrected (box-projected) reflection probes: box half-extent in world units.
		// 0 disables box projection (falls back to direction-only env sampling).
		float reflectionProbeBoxHalfExtent{ 1.5f };

		// Planar reflections (DX12 MVP): mark mirror materials with MaterialPerm::PlanarMirror.
		bool enablePlanarReflections{ true };
		std::uint32_t planarReflectionMaxMirrors{ 5 };

		bool drawLightGizmos{ false };
		bool debugDrawDepthTest{ true };
		bool drawNavigationMesh{ false };
		bool drawAIPlannedPathDebug{ false };
		bool drawGameplayMovementDebug{ false };
		bool drawGameplayMovementDebugOnlyControlled{ false };
		bool drawGameplayMovementDebugLabels{ false };
		bool drawGameplayMovementDebugText{ false };
		bool drawPhysicsCharacters{ false };
		bool drawPhysicsCharacterGround{ false };
		bool drawPhysicsCharacterVelocity{ false };
		bool drawPhysicsCharacterBlocked{ false };
		float physicsCharacterVelocityScale{ 0.35f };
		bool drawPhysicsBodies{ false };
		bool drawPhysicsBodyAabbs{ false };
		bool drawPhysicsBodyVelocity{ false };
		float physicsBodyVelocityScale{ 0.35f };

		bool drawAnimationRuntimeOverlay{ false };
		bool drawAnimationRuntimeOverlayOnlyControlled{ false };
		float animationRuntimeOverlayTextScale{ 1.10f };
		float animationRuntimeOverlayAnchorXPx{ 12.0f };
		float animationRuntimeOverlayAnchorYPx{ 54.0f };
		
		bool logCpuFrameTimings{ false };
		std::uint32_t performanceLogFrameInterval{ 120u };
		
		bool enableDebugWindowRender{ true };
		// Keep the legacy DX12 startup behavior: the main swapchain was created in immediate mode.
		bool enableVSync{ false };
		bool showPerformancePanel{ true };
		PerformanceSnapshot performanceSnapshot{};
		bool enableTinySleep{ false };

		float gameplayMovementVelocityScale{ 0.35f };
		float gameplayMovementTargetVelocityScale{ 0.35f };
		float gameplayMovementDesiredMoveScale{ 1.4f };
		float gameplayMovementFacingScale{ 1.1f };
		float gameplayMovementLift{ 0.08f };
		float gameplayMovementLabelScale{ 1.45f };
		float gameplayMovementTextScale{ 1.30f };
		float lightGizmoHalfSize{ 0.15f };
		float debugLightGizmoScale = 1.0f;
		float lightGizmoArrowLength{ 1.5f };
		float lightGizmoArrowThickness{ 0.05f };

		bool ShowCubeAtlas{ false }; // debug: visualize point shadow cubemap atlas on the swapchain (DX12-only)
		std::uint32_t debugCubeAtlasIndex{ 0 };
		std::uint32_t debugShadowCubeMapType{ 1 }; // point shadow: cube/light index; reflection mode: reflective-owner index

		bool drawPlanarMirrorNormals{ false };
		float planarMirrorNormalLength{ 2.0f };

		bool loadingOverlayVisible{ false };
		float loadingOverlayProgressBar{ 0.0f };

		std::uint32_t loadingOverlayTotalUnits{ 0u };
		std::uint32_t loadingOverlayCompletedUnits{ 0u };

		float reflectionCaptureFovPadDeg{ 0.0f };
		std::filesystem::path modelPath = std::filesystem::path("models") / "cube.obj";
	};
	
	enum class RendererSettingEditPolicy
	{
		Instant,
		Deferred,
		RequiresResourceRebuild,
		NotEditableAtRuntime
	};
	
	struct RendererSettingEditState
	{
		RendererSettingEditPolicy editPolicy{ RendererSettingEditPolicy::Instant };
		bool bCanEditNow{ true };
		const char* warning{ "" };
	};
	
	[[nodiscard]] RendererSettingEditState GetDeferredRendererEditState() noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::RequiresResourceRebuild,
			false,
			"Changing deferred rendering at runtime requires renderer resource rebuild. "
		};
	}
	
	[[nodiscard]] RendererSettingEditState GetSSAOSectionEditState(const RendererSettings& rendererSettings) noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::Instant,
			rendererSettings.enableDeferred,
			"SSAO controls require deferred rendering."
		};
	}

	[[nodiscard]] RendererSettingEditState GetFxaaControlsEditState(const RendererSettings& rendererSettings) noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::Instant,
			rendererSettings.antiAliasingMode == 1u,
			"FXAA controls are available only when FXAA is selected."
		};
	}

	[[nodiscard]] RendererSettingEditState GetHDRToggleEditState() noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::RequiresResourceRebuild,
			false,
			"Changing HDR at runtime can change render target formats and requires renderer resource rebuild."
		};
	}

	[[nodiscard]] RendererSettingEditState GetHDRControlsEditState(const RendererSettings& rendererSettings) noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::Instant,
			rendererSettings.enableHDR,
			"HDR controls are available only when HDR is enabled."
		};
	}

	[[nodiscard]] RendererSettingEditState GetBloomControlsEditState(const RendererSettings& rendererSettings) noexcept
	{
		return RendererSettingEditState{
			RendererSettingEditPolicy::Instant,
			rendererSettings.enableHDR && rendererSettings.enableBloom,
			"Bloom controls are available only when HDR and bloom are enabled."
		};
	}
}

export namespace rendern::renderer_settings_commands
{
	void SetDepthPrepassEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableDepthPrepass = bEnabled;
	}

	void SetDeferredEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableDeferred = bEnabled;
	}

	void SetFrustumCullingEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableFrustumCulling = bEnabled;
	}

	void SetDebugPrintDrawCallsEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.debugPrintDrawCalls = bEnabled;
	}

	void SetSSAOEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableSSAO = bEnabled;
	}

	void SetSSAORadius(RendererSettings& rendererSettings, const float radius) noexcept
	{
		rendererSettings.ssaoRadius = std::clamp(radius, 0.05f, 5.0f);
	}

	void SetSSAOBias(RendererSettings& rendererSettings, const float bias) noexcept
	{
		rendererSettings.ssaoBias = std::clamp(bias, 0.0f, 0.25f);
	}

	void SetSSAOStrength(RendererSettings& rendererSettings, const float strength) noexcept
	{
		rendererSettings.ssaoStrength = std::clamp(strength, 0.0f, 4.0f);
	}

	void SetSSAOPower(RendererSettings& rendererSettings, const float power) noexcept
	{
		rendererSettings.ssaoPower = std::clamp(power, 0.5f, 4.0f);
	}

	void SetSSAOBlurDepthThreshold(RendererSettings& rendererSettings, const float threshold) noexcept
	{
		rendererSettings.ssaoBlurDepthThreshold = std::clamp(threshold, 0.0f, 0.02f);
	}

	void ResetSSAODefaults(RendererSettings& rendererSettings) noexcept
	{
		rendererSettings.ssaoRadius = 1.0f;
		rendererSettings.ssaoBias = 0.02f;
		rendererSettings.ssaoStrength = 1.25f;
		rendererSettings.ssaoPower = 1.5f;
		rendererSettings.ssaoBlurDepthThreshold = 0.0025f;
	}

	void SetFogEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableFog = bEnabled;
	}

	void SetFogMode(RendererSettings& rendererSettings, const int mode) noexcept
	{
		rendererSettings.fogMode = static_cast<std::uint32_t>(std::clamp(mode, 0, 2));
	}

	void SetFogRange(RendererSettings& rendererSettings, const float start, const float end) noexcept
	{
		rendererSettings.fogStart = std::clamp(start, 0.0f, 500.0f);
		rendererSettings.fogEnd = std::clamp(end, 0.0f, 500.0f);
		if (rendererSettings.fogEnd < rendererSettings.fogStart)
		{
			std::swap(rendererSettings.fogEnd, rendererSettings.fogStart);
		}
	}

	void SetFogDensity(RendererSettings& rendererSettings, const float density) noexcept
	{
		rendererSettings.fogDensity = std::clamp(density, 0.0f, 0.25f);
	}

	void SetFogColor(RendererSettings& rendererSettings, const std::array<float, 3>& color) noexcept
	{
		rendererSettings.fogColor = color;
	}

	void ResetFogDefaults(RendererSettings& rendererSettings) noexcept
	{
		rendererSettings.fogMode = 0u;
		rendererSettings.fogStart = 15.0f;
		rendererSettings.fogEnd = 80.0f;
		rendererSettings.fogDensity = 0.02f;
		rendererSettings.fogColor = { 0.60f, 0.70f, 0.80f };
	}

	void SetAntiAliasingMode(RendererSettings& rendererSettings, const int mode) noexcept
	{
		rendererSettings.antiAliasingMode = static_cast<std::uint32_t>(std::clamp(mode, 0, 1));
	}

	void SetFxaaSubpix(RendererSettings& rendererSettings, const float subpix) noexcept
	{
		rendererSettings.fxaaSubpix = std::clamp(subpix, 0.0f, 1.0f);
	}

	void SetFxaaEdgeThreshold(RendererSettings& rendererSettings, const float threshold) noexcept
	{
		rendererSettings.fxaaEdgeThreshold = std::clamp(threshold, 0.0312f, 0.333f);
	}

	void SetFxaaEdgeThresholdMin(RendererSettings& rendererSettings, const float thresholdMin) noexcept
	{
		rendererSettings.fxaaEdgeThresholdMin = std::clamp(thresholdMin, 0.0f, 0.125f);
	}

	void ResetFxaaDefaults(RendererSettings& rendererSettings) noexcept
	{
		rendererSettings.fxaaSubpix = 0.75f;
		rendererSettings.fxaaEdgeThreshold = 0.166f;
		rendererSettings.fxaaEdgeThresholdMin = 0.0833f;
	}

	void SetHDREnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableHDR = bEnabled;
	}

	void SetToneMapMode(RendererSettings& rendererSettings, const int mode) noexcept
	{
		rendererSettings.toneMapMode = static_cast<std::uint32_t>(std::clamp(mode, 0, 2));
	}

	void SetHDRExposure(RendererSettings& rendererSettings, const float exposure) noexcept
	{
		rendererSettings.hdrExposure = std::clamp(exposure, 0.1f, 8.0f);
	}

	void SetBloomEnabled(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.enableBloom = bEnabled;
	}

	void SetBloomThreshold(RendererSettings& rendererSettings, const float threshold) noexcept
	{
		rendererSettings.bloomThreshold = std::clamp(threshold, 0.1f, 8.0f);
	}

	void SetBloomSoftKnee(RendererSettings& rendererSettings, const float softKnee) noexcept
	{
		rendererSettings.bloomSoftKnee = std::clamp(softKnee, 0.0f, 2.0f);
	}

	void SetBloomIntensity(RendererSettings& rendererSettings, const float intensity) noexcept
	{
		rendererSettings.bloomIntensity = std::clamp(intensity, 0.0f, 1.5f);
	}

	void SetBloomClamp(RendererSettings& rendererSettings, const float clampValue) noexcept
	{
		rendererSettings.bloomClamp = std::clamp(clampValue, 1.0f, 64.0f);
	}

	void SetBloomRadius(RendererSettings& rendererSettings, const float radius) noexcept
	{
		rendererSettings.bloomRadius = std::clamp(radius, 0.25f, 4.0f);
	}

	void ResetHDRBloomDefaults(RendererSettings& rendererSettings) noexcept
	{
		rendererSettings.enableHDR = true;
		rendererSettings.toneMapMode = 2u;
		rendererSettings.hdrExposure = 1.0f;
		rendererSettings.enableBloom = true;
		rendererSettings.bloomThreshold = 1.0f;
		rendererSettings.bloomSoftKnee = 0.5f;
		rendererSettings.bloomIntensity = 0.08f;
		rendererSettings.bloomClamp = 16.0f;
		rendererSettings.bloomRadius = 1.0f;
	}

	void SetShowCubeAtlas(RendererSettings& rendererSettings, const bool bEnabled) noexcept
	{
		rendererSettings.ShowCubeAtlas = bEnabled;
	}

	void SetDebugShadowCubeMapType(RendererSettings& rendererSettings, const int type) noexcept
	{
		rendererSettings.debugShadowCubeMapType = static_cast<std::uint32_t>(std::clamp(type, 0, 1));
	}

	void SetDebugCubeAtlasIndex(RendererSettings& rendererSettings, const int index) noexcept
	{
		rendererSettings.debugCubeAtlasIndex = static_cast<std::uint32_t>(std::max(index, 0));
	}

	void SetShadowBiasSettings(
		RendererSettings& rendererSettings,
		const float dirBaseBiasTexels,
		const float spotBaseBiasTexels,
		const float pointBaseBiasTexels,
		const float slopeScaleTexels) noexcept
	{
		rendererSettings.dirShadowBaseBiasTexels = std::clamp(dirBaseBiasTexels, 0.0f, 5.0f);
		rendererSettings.spotShadowBaseBiasTexels = std::clamp(spotBaseBiasTexels, 0.0f, 10.0f);
		rendererSettings.pointShadowBaseBiasTexels = std::clamp(pointBaseBiasTexels, 0.0f, 10.0f);
		rendererSettings.shadowSlopeScaleTexels = std::clamp(slopeScaleTexels, 0.0f, 10.0f);
	}
}
