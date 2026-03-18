export namespace RHI_GL_UTILS
{
	inline void ThrowIfShaderCompilationFailed(GLuint shader, std::string_view debugName)
	{
		GLint success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (success == GL_FALSE)
		{
			GLint logLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
			std::string infoLog;
			infoLog.resize(std::max(0, logLength));
			if (logLength > 0)
			{
				glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
			}
			throw std::runtime_error(std::string("OpenGL shader compile failed (") + std::string(debugName) + "): " + infoLog);
		}
	}

	inline void ThrowIfProgramLinkFailed(GLuint program, std::string_view debugName)
	{
		GLint success = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (success == GL_FALSE)
		{
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
			std::string infoLog;
			infoLog.resize(std::max(0, logLength));
			if (logLength > 0)
			{
				glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
			}
			throw std::runtime_error(std::string("OpenGL program link failed (") + std::string(debugName) + "): " + infoLog);
		}
	}
}

namespace
{
	static std::uint32_t DefaultLocation(rhi::VertexSemantic sem, std::uint8_t semIndex) noexcept
	{
		switch (sem)
		{
		case rhi::VertexSemantic::Position:
			return 0;
		case rhi::VertexSemantic::Normal:
			return 1;
		case rhi::VertexSemantic::TexCoord:
			return 2 + static_cast<std::uint32_t>(semIndex) * 4u;
		case rhi::VertexSemantic::Color:
			return 3;
		case rhi::VertexSemantic::Tangent:
			return 4;
		case rhi::VertexSemantic::BoneIndices:
			return 5;
		case rhi::VertexSemantic::BoneWeights:
			return 6;
		default:
			return 0;
		}
	}

	static bool IsDepthFormat(rhi::Format format) noexcept
	{
		return format == rhi::Format::D32_FLOAT || format == rhi::Format::D24_UNORM_S8_UINT;
	}

	static GLenum ToGLInternalFormat(rhi::Format format)
	{
		switch (format)
		{
		case rhi::Format::RGBA8_UNORM:
			return GL_RGBA8;
		case rhi::Format::RGBA16_FLOAT:
			return GL_RGBA16F;
		case rhi::Format::R32_FLOAT:
			return GL_R32F;
		case rhi::Format::BGRA8_UNORM:
			return GL_RGBA8;
		case rhi::Format::D32_FLOAT:
			return GL_DEPTH_COMPONENT32F;
		case rhi::Format::D24_UNORM_S8_UINT:
			return GL_DEPTH24_STENCIL8;
		default:
			return GL_RGBA8;
		}
	}

	static GLenum ToGLBaseFormat(rhi::Format format)
	{
		switch (format)
		{
		case rhi::Format::RGBA8_UNORM:
		case rhi::Format::RGBA16_FLOAT:
			return GL_RGBA;
		case rhi::Format::BGRA8_UNORM:
			return GL_BGRA;
		case rhi::Format::R32_FLOAT:
			return GL_RED;
		case rhi::Format::D32_FLOAT:
			return GL_DEPTH_COMPONENT;
		case rhi::Format::D24_UNORM_S8_UINT:
			return GL_DEPTH_STENCIL;
		default:
			return GL_RGBA;
		}
	}

	static GLenum ToGLType(rhi::Format format)
	{
		switch (format)
		{
		case rhi::Format::RGBA8_UNORM:
		case rhi::Format::BGRA8_UNORM:
			return GL_UNSIGNED_BYTE;
		case rhi::Format::RGBA16_FLOAT:
		case rhi::Format::R32_FLOAT:
		case rhi::Format::D32_FLOAT:
			return GL_FLOAT;
		case rhi::Format::D24_UNORM_S8_UINT:
			return GL_UNSIGNED_INT_24_8;
		default:
			return GL_UNSIGNED_BYTE;
		}
	}

	static GLenum ToGLCompareOp(rhi::CompareOp op)
	{
		switch (op)
		{
		case rhi::CompareOp::Never:
			return GL_NEVER;
		case rhi::CompareOp::Less:
			return GL_LESS;
		case rhi::CompareOp::Equal:
			return GL_EQUAL;
		case rhi::CompareOp::LessEqual:
			return GL_LEQUAL;
		case rhi::CompareOp::Greater:
			return GL_GREATER;
		case rhi::CompareOp::NotEqual:
			return GL_NOTEQUAL;
		case rhi::CompareOp::GreaterEqual:
			return GL_GEQUAL;
		case rhi::CompareOp::Always:
			return GL_ALWAYS;
		default:

			return GL_LEQUAL;
		}
	}

	static GLenum ToGLStencilOp(rhi::StencilOp op)
	{
		switch (op)
		{
		case rhi::StencilOp::Keep:
			return GL_KEEP;
		case rhi::StencilOp::Zero:
			return GL_ZERO;
		case rhi::StencilOp::Replace:
			return GL_REPLACE;
		case rhi::StencilOp::IncrementClamp:
			return GL_INCR;
		case rhi::StencilOp::DecrementClamp:
			return GL_DECR;
		case rhi::StencilOp::Invert:
			return GL_INVERT;
		case rhi::StencilOp::IncrementWrap:
			return GL_INCR_WRAP;
		case rhi::StencilOp::DecrementWrap:
			return GL_DECR_WRAP;
		default:
			return GL_KEEP;
		}
	}

	static GLenum ToGLCullMode(rhi::CullMode mode)
	{
		switch (mode)
		{

		case rhi::CullMode::Front:
			return GL_FRONT;
		case rhi::CullMode::Back:
			return GL_BACK;
		default:
			return GL_BACK;
		}
	}

	static GLenum ToGLFrontFace(rhi::FrontFace face)
	{
		return (face == rhi::FrontFace::Clockwise) ? GL_CW : GL_CCW;
	}

	static GLenum BufferTargetFor(rhi::BufferBindFlag bindFlag)
	{
		switch (bindFlag)
		{
		case rhi::BufferBindFlag::VertexBuffer:
			return GL_ARRAY_BUFFER;
		case rhi::BufferBindFlag::IndexBuffer:
			return GL_ELEMENT_ARRAY_BUFFER;
		case rhi::BufferBindFlag::ConstantBuffer:
			return GL_UNIFORM_BUFFER;
		case rhi::BufferBindFlag::UniformBuffer:
			return GL_UNIFORM_BUFFER;
		case rhi::BufferBindFlag::StructuredBuffer:
			return GL_SHADER_STORAGE_BUFFER;
		default:
			return GL_ARRAY_BUFFER;
		}
	}

	static GLenum BufferUsageFor(rhi::BufferUsageFlag usage)
	{
		switch (usage)
		{
		case rhi::BufferUsageFlag::Static:
			return GL_STATIC_DRAW;
		case rhi::BufferUsageFlag::Dynamic:
			return GL_DYNAMIC_DRAW;
		case rhi::BufferUsageFlag::Stream:
			return GL_STREAM_DRAW;
		default:
			return GL_STATIC_DRAW;
		}
	}

	static GLenum ToGLIndexType(rhi::IndexType t)
	{
		return (t == rhi::IndexType::UINT16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	}

	static std::uint32_t IndexSizeBytes(rhi::IndexType t)
	{
		return (t == rhi::IndexType::UINT16) ? 2u : 4u;
	}

	static std::string EnsureGLSLVersion(std::string_view source)
	{
		constexpr std::string_view kVersion = "#version";
		std::string_view vSource = source;
		while (!vSource.empty() && (
			vSource.front() == ' '
			|| vSource.front() == '\t'
			|| vSource.front() == '\r'
			|| vSource.front() == '\n')
			)
		{
			vSource.remove_prefix(1);
		}

		if (vSource.rfind(kVersion, 0) == 0)
		{
			return std::string(source);
		}

		std::string outStr;
		outStr.reserve(source.size() + 32);
		outStr += "#version 330 core\n";
		outStr += source;
		return outStr;
	}

	struct GLAttrib
	{
		GLuint location{};
		GLint componentCount{};
		GLenum type{};
		GLboolean normalized{};
		GLboolean integerInput{};
		GLuint offsetBytes{};
		std::uint32_t inputSlot{};
	};

	struct GLInputLayout
	{
		std::uint32_t strideBytes{};
		std::vector<GLAttrib> attribs;
		std::string debugName;
	};

	static GLenum ToGLShaderStage(rhi::ShaderStage stage)
	{
		switch (stage)
		{
		case rhi::ShaderStage::Vertex:
			return GL_VERTEX_SHADER;
		case rhi::ShaderStage::Pixel:
			return GL_FRAGMENT_SHADER;
		case rhi::ShaderStage::Geometry:
			return GL_GEOMETRY_SHADER;
		case rhi::ShaderStage::Compute:
			return GL_COMPUTE_SHADER;
		default:
			return GL_FRAGMENT_SHADER;
		}
	}

	static bool IsIntegerVertexFormat(rhi::VertexFormat format) noexcept
	{
		switch (format)
		{
		case rhi::VertexFormat::R16G16B16A16_UINT:
		case rhi::VertexFormat::R32G32B32A32_UINT:
			return true;
		default:
			return false;
		}
	}

	static void VertexFormatToGL(rhi::VertexFormat format, GLint& outComponents, GLenum& outType)
	{
		switch (format)
		{
		case rhi::VertexFormat::R32G32B32_FLOAT:
			outComponents = 3;
			outType = GL_FLOAT;
			break;
		case rhi::VertexFormat::R32G32_FLOAT:
			outComponents = 2;
			outType = GL_FLOAT;
			break;
		case rhi::VertexFormat::R32G32B32A32_FLOAT:
			outComponents = 4;
			outType = GL_FLOAT;
			break;
		case rhi::VertexFormat::R8G8B8A8_UNORM:
			outComponents = 4;
			outType = GL_UNSIGNED_BYTE;
			break;
		case rhi::VertexFormat::R16G16B16A16_UINT:
			outComponents = 4;
			outType = GL_UNSIGNED_SHORT;
			break;
		case rhi::VertexFormat::R16G16B16A16_UNORM:
			outComponents = 4;
			outType = GL_UNSIGNED_SHORT;
			break;
		case rhi::VertexFormat::R32G32B32A32_UINT:
			outComponents = 4;
			outType = GL_UNSIGNED_INT;
			break;
		default:
			outComponents = 4;
			outType = GL_FLOAT;
			break;
		}
	}

	struct VaoKey
	{
		std::uint32_t layoutId{};
		std::uint32_t vbId{};
		std::uint32_t vbOffset{};
		std::uint32_t vbStride{};
		std::uint32_t ibId{};
		std::uint32_t ibOffset{};
		rhi::IndexType indexType{ rhi::IndexType::UINT16 };

		friend bool operator==(const VaoKey& a, const VaoKey& b) noexcept
		{
			return a.layoutId == b.layoutId
				&& a.vbId == b.vbId
				&& a.vbOffset == b.vbOffset
				&& a.vbStride == b.vbStride
				&& a.ibId == b.ibId
				&& a.ibOffset == b.ibOffset
				&& a.indexType == b.indexType;
		}
	};

	struct VaoKeyHash
	{
		std::size_t operator()(const VaoKey& key) const noexcept
		{
			std::size_t hash = 1469598103934665603ull;
			auto mix = [&](std::uint64_t val)
				{
					hash ^= static_cast<std::size_t>(val) + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
				};
			mix(key.layoutId);
			mix(key.vbId);
			mix(key.vbOffset);
			mix(key.vbStride);
			mix(key.ibId);
			mix(key.ibOffset);
			mix(static_cast<std::uint32_t>(key.indexType));
			return hash;
		}
	};

	struct GLFence
	{
		GLsync sync{ nullptr };
		bool signaled{ false };
	};

	struct VertexBufferState
	{
		rhi::BufferHandle buffer{};
		std::uint32_t strideBytes{ 0 };
		std::uint32_t offsetBytes{ 0 };
	};
}
