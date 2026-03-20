module;

#include <GL/glew.h>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

export module core:render_core_gl;

import :render_core;
import :resource_manager_core;

namespace
{
	static GLenum ToGLExternalFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::RGB:
			return GL_RGB;
		case TextureFormat::RGBA:
			return GL_RGBA;
		case TextureFormat::GRAYSCALE:
			return GL_RED;
		default:
			return GL_RGBA;
		}
	}

	static GLint ToGLInternalFormat(TextureFormat format, bool srgb)
	{
		switch (format)
		{
		case TextureFormat::RGB:
			return srgb ? GL_SRGB8 : GL_RGB8;
		case TextureFormat::RGBA:
			return srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		case TextureFormat::GRAYSCALE:
			return GL_R8;
		default:
			return GL_RGBA8;
		}
	}

	static void SetTextureAnisotropy(GLenum target) noexcept
	{
	#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
		if (GLEW_EXT_texture_filter_anisotropic)
		{
			GLfloat maxAniso = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
			const GLfloat wanted = std::min<GLfloat>(16.0f, std::max<GLfloat>(1.0f, maxAniso));
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, wanted);
		}
	#endif
	}

	static void SetDefaultTextureParameters(bool generateMips)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (generateMips)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			SetTextureAnisotropy(GL_TEXTURE_2D);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	static void SetDefaultCubemapParameters(bool generateMips)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		if (generateMips)
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			SetTextureAnisotropy(GL_TEXTURE_CUBE_MAP);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
}

export namespace rendern
{
	class GLTextureUploader final : public ITextureUploader
	{
	public:
		explicit GLTextureUploader(rhi::IRHIDevice& device) noexcept
		{
			//assert(device.GetBackend() == rhi::Backend::OpenGL);
		}
		std::optional<GPUTexture> CreateAndUpload(const TextureCPUData& cpuData, const TextureProperties& properties) override
		{
			// ---------------------- Cubemap ----------------------
			if (properties.dimension == TextureDimension::Cube)
			{
				const GLsizei width = static_cast<GLsizei>(cpuData.width ? cpuData.width : properties.width);
				const GLsizei height = static_cast<GLsizei>(cpuData.height ? cpuData.height : properties.height);

				if (width <= 0 || height <= 0)
				{
					return std::nullopt;
				}

				if (cpuData.format != TextureFormat::RGBA || cpuData.channels != 4)
				{
					return std::nullopt;
				}

				const auto& face0 = cpuData.cubeMips[0];
				if (face0.empty())
				{
					return std::nullopt;
				}

				const std::size_t mipLevels = face0.size();
				for (int face = 0; face < 6; ++face)
				{
					const auto& fm = cpuData.cubeMips[static_cast<std::size_t>(face)];
					if (fm.size() != mipLevels)
					{
						return std::nullopt;
					}
					if (fm.empty() || static_cast<GLsizei>(fm[0].width) != width || static_cast<GLsizei>(fm[0].height) != height)
					{
						return std::nullopt;
					}
				}

				GLuint textureId = 0;
				glGenTextures(1, &textureId);
				if (textureId == 0)
				{
					return std::nullopt;
				}

				glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
				SetDefaultCubemapParameters(mipLevels > 1);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipLevels > 0 ? (mipLevels - 1) : 0));

				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				const GLenum externalFormat = ToGLExternalFormat(TextureFormat::RGBA);
				const GLenum internalFormat = ToGLInternalFormat(TextureFormat::RGBA, properties.srgb);

				for (GLuint face = 0; face < 6; ++face)
				{
					const auto& fm = cpuData.cubeMips[face];
					for (std::size_t mip = 0; mip < mipLevels; ++mip)
					{
						const auto& ml = fm[mip];
						glTexImage2D(
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
							static_cast<GLint>(mip),
							internalFormat,
							static_cast<GLsizei>(ml.width),
							static_cast<GLsizei>(ml.height),
							0,
							externalFormat,
							GL_UNSIGNED_BYTE,
							ml.pixels.data());
					}
				}

				glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

				if (glGetError() != GL_NO_ERROR)
				{
					glDeleteTextures(1, &textureId);
					return std::nullopt;
				}

				return GPUTexture{ static_cast<unsigned int>(textureId) };
			}

			// ---------------------- Tex2D ----------------------
			if (cpuData.mips.empty())
			{
				return std::nullopt;
			}

			if (cpuData.format != TextureFormat::RGBA || cpuData.channels != 4)
			{
				return std::nullopt;
			}

			const std::size_t mipLevels = cpuData.mips.size();

			GLuint textureId = 0;
			glGenTextures(1, &textureId);
			if (textureId == 0)
			{
				return std::nullopt;
			}

			glBindTexture(GL_TEXTURE_2D, textureId);
			SetDefaultTextureParameters(mipLevels > 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mipLevels > 0 ? (mipLevels - 1) : 0));

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			const GLenum externalFormat = ToGLExternalFormat(TextureFormat::RGBA);
			const GLenum internalFormat = ToGLInternalFormat(TextureFormat::RGBA, properties.srgb);

			for (std::size_t mip = 0; mip < mipLevels; ++mip)
			{
				const auto& ml = cpuData.mips[mip];
				glTexImage2D(
					GL_TEXTURE_2D,
					static_cast<GLint>(mip),
					internalFormat,
					static_cast<GLsizei>(ml.width),
					static_cast<GLsizei>(ml.height),
					0,
					externalFormat,
					GL_UNSIGNED_BYTE,
					ml.pixels.data());
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			glBindTexture(GL_TEXTURE_2D, 0);

			if (glGetError() != GL_NO_ERROR)
			{
				glDeleteTextures(1, &textureId);
				return std::nullopt;
			}

			return GPUTexture{ static_cast<unsigned int>(textureId) };
		}

		void Destroy(GPUTexture texture) noexcept
		{
			const GLuint textureId = static_cast<GLuint>(texture.id);
			if (textureId != 0)
			{
				glDeleteTextures(1, &textureId);
			}
		}
	};
}