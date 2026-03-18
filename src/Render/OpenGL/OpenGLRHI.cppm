module;

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

export module core:rhi_gl;

import :rhi;

export namespace rhi
{
	// This keeps the GL backend independent from a specific windowing library (GLFW/SDL/etc.).
	struct GLSwapChainHooks
	{
		std::function<void()> present;
		std::function<Extent2D()> getDrawableExtent;
		std::function<void(bool)> setVsync;
	};

	struct GLDeviceDesc
	{
		bool enableDebug{ false };
	};

	struct GLSwapChainDesc
	{
		SwapChainDesc base{};
		GLSwapChainHooks hooks{};
	};

	std::unique_ptr<IRHIDevice> CreateGLDevice(GLDeviceDesc desc = {});
	std::unique_ptr<IRHISwapChain> CreateGLSwapChain(IRHIDevice& device, GLSwapChainDesc desc);
}

#include "OpenGLRHI_GlUtils.inl"


namespace rhi
{
	class GLSwapChain final : public IRHISwapChain
	{
	public:
		GLSwapChain(GLSwapChainDesc desc)
			: desc_(std::move(desc))
		{
			if (desc_.hooks.setVsync)
			{
				desc_.hooks.setVsync(desc_.base.vsync);
			}
		}
		~GLSwapChain() override = default;

		SwapChainDesc GetDesc() const override
		{
			SwapChainDesc outDesc = desc_.base;
			if (desc_.hooks.getDrawableExtent)
			{
				outDesc.extent = desc_.hooks.getDrawableExtent();
			}
			return outDesc;
		}

		FrameBufferHandle GetCurrentBackBuffer() const override
		{
			return FrameBufferHandle{ 0 };
		}

		TextureHandle GetDepthTexture() const override
		{
			return TextureHandle{};
		}

		void Present() override
		{
			if (desc_.hooks.present)
			{
				desc_.hooks.present();
			}
			else
			{
				glFlush();
			}
		}

		void Resize(Extent2D newExtent) override
		{
			desc_.base.extent = newExtent;
		}

	private:
		GLSwapChainDesc desc_{};
	};

	class GLDevice final : public IRHIDevice
	{
	public:
		explicit GLDevice([[maybe_unused]] GLDeviceDesc desc)
			: desc_(std::move(desc))
		{
			const GLubyte* vendor = glGetString(GL_VENDOR);
			const GLubyte* renderer = glGetString(GL_RENDERER);
			const GLubyte* version = glGetString(GL_VERSION);

			name_.reserve(256);
			name_ += "OpenGL";
			if (vendor)
			{
				name_ += " | ";
				name_ += reinterpret_cast<const char*>(vendor);
			}
			if (renderer)
			{
				name_ += " | ";
				name_ += reinterpret_cast<const char*>(renderer);
			}
			if (version)
			{
				name_ += " | ";
				name_ += reinterpret_cast<const char*>(version);
			}
		}

		~GLDevice()
		{
			InvalidateVaoCache();

			for (auto& [_, fence] : fences_)
			{
				if (fence.sync)
				{
					glDeleteSync(fence.sync);
				}
			}
			fences_.clear();
		}

		Backend GetBackend() const noexcept override
		{
			return Backend::OpenGL;
		}

		std::string_view GetName() const override
		{
			return name_;
		}

		#include "OpenGLRHI_DeviceApi.inl"

	private:
		#include "OpenGLRHI_DevicePrivate.inl"

		//---------------------------------------------------------------------//
		GLDeviceDesc desc_{};
		std::string name_;

		// Buffer targets: buffer id -> GL target
		std::unordered_map<GLuint, GLenum> bufferTargets_{};
		// Input layouts (1-based handle id -> vector[id-1])
		std::vector<GLInputLayout> inputLayouts_{};

		// Descriptor indices (0 invalid)
		std::vector<TextureHandle> textureDescriptions_{ TextureHandle{} };
		std::vector<TextureDescIndex> freeTextureDescIndices_;

		// Fence storage
		std::uint32_t nextFenceId_{ 0 };
		std::unordered_map<std::uint32_t, GLFence> fences_{};

		// Current program + uniform location cache per program
		std::unordered_map<GLuint, std::unordered_map<std::string, GLint>> uniformLocationCache_{};
		GLuint currentProgram_{ 0 };

		GLenum currentTopology_{ GL_TRIANGLES };

		rhi::GraphicsState currentGraphicsState_{};
		std::uint32_t currentStencilRef_{ 0 };

		// Current bindings for VAO build
		rhi::InputLayoutHandle currentLayout_{};
		std::array<VertexBufferState, 1> vertexBuffer_{};
		struct {
			rhi::BufferHandle buffer{};
			rhi::IndexType indexType{ rhi::IndexType::UINT16 };
			std::uint32_t offsetBytes{ 0 };
		} indexBuffer_{};

		// VAO cache
		std::unordered_map<VaoKey, GLuint, VaoKeyHash> vaoCache_{};
		GLuint boundVao_{ 0 };
	};

	std::unique_ptr<IRHIDevice> CreateGLDevice(GLDeviceDesc desc)
	{
		return std::make_unique<GLDevice>(std::move(desc));
	}

	std::unique_ptr<IRHISwapChain> CreateGLSwapChain([[maybe_unused]] IRHIDevice& device, GLSwapChainDesc desc)
	{
		return std::make_unique<GLSwapChain>(std::move(desc));
	}
}
