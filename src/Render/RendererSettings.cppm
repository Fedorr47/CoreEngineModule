module;

#include <filesystem>

// Shared, backend-agnostic renderer settings.
export module core:renderer_settings;

export namespace rendern
{
	struct RendererSettings
	{
		float dirShadowBaseBiasTexels{ 1.0f };
		float spotShadowBaseBiasTexels{ 1.25f };
		float pointShadowBaseBiasTexels{ 1.5f };
		float shadowSlopeScaleTexels{ 2.0f };

		bool enableDepthPrepass{ false };
		bool debugPrintDrawCalls{ true }; // prints MainPass draw-call count (DX12) once per ~60 frames
		std::filesystem::path modelPath = std::filesystem::path("models") / "cube.obj";
	};
}
