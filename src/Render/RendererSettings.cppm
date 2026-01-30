module;

#include <filesystem>

// Shared, backend-agnostic renderer settings.
export module core:renderer_settings;

export namespace rendern
{
	struct RendererSettings
	{
		bool enableDepthPrepass{ false };
		bool debugPrintDrawCalls{ true }; // prints MainPass draw-call count (DX12) once per ~60 frames
		std::filesystem::path modelPath = std::filesystem::path("models") / "cube.obj";
	};
}
