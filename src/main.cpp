#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

import core;
import std;

static rhi::Extent2D GetDrawableExtent(GLFWwindow* wnd)
{
	int w = 0;
	int h = 0;
	glfwGetFramebufferSize(wnd, &w, &h);
	return rhi::Extent2D{ static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h) };
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "GLFW init failed\n";
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* wnd = glfwCreateWindow(1280, 720, "RenderGraph + RHI_GL Demo", nullptr, nullptr);
	if (!wnd)
	{
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(wnd);
	glfwSwapInterval(1);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "GLEW init failed\n";
		glfwDestroyWindow(wnd);
		glfwTerminate();
		return 1;
	}

	auto device = rhi::CreateGLDevice(rhi::GLDeviceDesc{ .enableDebug = false });

	rhi::GLSwapChainDesc sc{};
	sc.base.extent = GetDrawableExtent(wnd);
	sc.base.vsync = true;
	sc.base.backbufferFormat = rhi::Format::BGRA8_UNORM;
	sc.hooks.present = [wnd] { glfwSwapBuffers(wnd); };
	sc.hooks.getDrawableExtent = [wnd] { return GetDrawableExtent(wnd); };
	sc.hooks.setVsync = [](bool vsync) { glfwSwapInterval(vsync ? 1 : 0); };

	auto swapchain = rhi::CreateGLSwapChain(*device, std::move(sc));

	std::cout << "Device: " << device->GetName() << "\n";

	// --- ResourceManager + GL uploader demo ---
	// We create a tiny procedural decoder (checkerboard) and let ResourceManager
	// drive the CPU->GPU upload through IRenderQueue.
	struct CheckerDecoder final : public ITextureDecoder
	{
		std::optional<TextureCPUData> Decode(const TextureProperties& properties, std::string_view) override
		{
			TextureCPUData out;
			out.width = properties.width;
			out.height = properties.height;
			out.channels = 4;
			out.format = TextureFormat::RGBA;
			out.pixels.resize(static_cast<std::size_t>(out.width) * out.height * 4);

			for (std::uint32_t y = 0; y < out.height; ++y)
			{
				for (std::uint32_t x = 0; x < out.width; ++x)
				{
					const bool dark = (((x / 32) ^ (y / 32)) & 1) != 0;
					const unsigned char c = dark ? 40 : 220;
					const std::size_t i = (static_cast<std::size_t>(y) * out.width + x) * 4;
					out.pixels[i + 0] = c;
					out.pixels[i + 1] = c;
					out.pixels[i + 2] = c;
					out.pixels[i + 3] = 255;
				}
			}
			return out;
		}
	};

	CheckerDecoder decoder;
	rendern::JobSystemImmediate jobs;
	rendern::RenderQueueImmediate renderQueue;
	rendern::GLTextureUploader uploader;
	TextureIO io{ decoder, uploader, jobs, renderQueue };

	ResourceManager rm;
	TextureProperties props{};
	props.width = 256;
	props.height = 256;
	props.format = TextureFormat::RGBA;
	props.srgb = true;
	props.generateMips = true;
	props.filePath = "";

	auto tex = rm.LoadAsync<TextureResource>("checker", io, props);

	rendern::Renderer renderer(*device);

	while (!glfwWindowShouldClose(wnd))
	{
		glfwPollEvents();

		// Pump CPU->GPU uploads (runs immediately in this demo).
		rm.ProcessUploads<TextureResource>(io, 8, 8);

		rhi::TextureHandle sampled{};
		if (rm.GetState<TextureResource>("checker") == ResourceState::Loaded)
		{
			sampled = rhi::TextureHandle{ tex->GetResource().id };
		}

		renderer.RenderFrame(*swapchain, sampled);
	}

	renderer.Shutdown();

	// Clean up GL resources created by ResourceManager.
	rm.Clear<TextureResource>();
	rm.ProcessUploads<TextureResource>(io, 0, 64);

	glfwDestroyWindow(wnd);
	glfwTerminate();
	return 0;
}
