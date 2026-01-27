module;

#include <filesystem>

// Shared, backend-agnostic renderer settings.
export module core:renderer_settings;

export namespace rendern
{
	struct RendererSettings
	{
		bool enableDepthPrepass{ false };
		std::filesystem::path modelPath = std::filesystem::path("models") / "cube.obj";
	};
}
