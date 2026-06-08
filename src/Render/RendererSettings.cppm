module;

#include <filesystem>
#include <cstdint>
#include <array>
#include <cstddef>

// Shared, backend-agnostic renderer settings.
export module core:renderer_settings;

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
		bool drawGameplayMovementDebug{ false };
		bool drawGameplayMovementDebugOnlyControlled{ false };
		bool drawGameplayMovementDebugLabels{ false };
		bool drawGameplayMovementDebugText{ false };

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
}
