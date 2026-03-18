		// ---------------- Textures ----------------
		TextureHandle CreateTexture2D(Extent2D extent, Format format) override
		{
			GLuint textureId = 0;
			glGenTextures(1, &textureId);
			glBindTexture(GL_TEXTURE_2D, textureId);

			if (format == rhi::Format::D32_FLOAT)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			const GLenum internalFormat = ToGLInternalFormat(format);
			const GLenum baseFormat = ToGLBaseFormat(format);
			const GLenum type = ToGLType(format);

			glTexImage2D(
				GL_TEXTURE_2D
				, 0
				, static_cast<GLint>(internalFormat)
				, static_cast<GLsizei>(extent.width)
				, static_cast<GLsizei>(extent.height)
				, 0
				, baseFormat
				, type
				, nullptr);

			glBindTexture(GL_TEXTURE_2D, 0);
			return TextureHandle{ static_cast<std::uint32_t>(textureId) };
		}


		TextureHandle CreateTextureCube(Extent2D extent, Format format) override
		{
			// Used for point light shadows: R32_FLOAT distance cubemap.
			if (IsDepthFormat(format))
			{
				throw std::runtime_error("OpenGL: CreateTextureCube: depth formats are not supported for cubemaps");
			}

			GLuint tex = 0;
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);

			const GLenum internal = ToGLInternalFormat(format);

			glTextureStorage2D(tex, 1, internal, static_cast<GLsizei>(extent.width), static_cast<GLsizei>(extent.height));

			// No filtering for shadow distance maps
			glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// Clamp to edge to avoid seams.
			glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

			return TextureHandle{ tex };
		}

void DestroyTexture(TextureHandle texture) noexcept override
		{
			GLuint textureId = static_cast<GLuint>(texture.id);
			if (textureId != 0)
			{
				glDeleteTextures(1, &textureId);
			}
		}

		// ---------------- Framebuffers ----------------
		FrameBufferHandle CreateFramebuffer(TextureHandle color, TextureHandle depth) override
		{
			GLuint framebufferId = 0;
			glGenFramebuffers(1, &framebufferId);
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

			if (color.id != 0)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(color.id), 0);
				GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
				glDrawBuffers(1, &drawBuffers);
			}
			else
			{
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}

			if (depth.id != 0)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, static_cast<GLuint>(depth.id), 0);
			}

			const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				glDeleteFramebuffers(1, &framebufferId);
				throw std::runtime_error("Failed to create framebuffer: incomplete framebuffer");
			}

			return FrameBufferHandle{ static_cast<std::uint32_t>(framebufferId) };
		}


		FrameBufferHandle CreateFramebufferCubeFace(TextureHandle colorCube, std::uint32_t faceIndex, TextureHandle depth) override
		{
			GLuint fbo = 0;
			glCreateFramebuffers(1, &fbo);

			if (colorCube.id != 0)
			{
				// Cubemap is a 2D array texture with 6 layers (faces).
				glNamedFramebufferTextureLayer(fbo, GL_COLOR_ATTACHMENT0, colorCube.id, 0, static_cast<GLint>(faceIndex));
			}

			if (depth.id != 0)
			{
				glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth.id, 0);
			}

			const GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				throw std::runtime_error("OpenGL: CreateFramebufferCubeFace: framebuffer is incomplete");
			}

			return FrameBufferHandle{ fbo };
		}

void DestroyFramebuffer(FrameBufferHandle framebuffer) noexcept override
		{
			GLuint framebufferId = static_cast<GLuint>(framebuffer.id);
			if (framebufferId != 0)
			{
				glDeleteFramebuffers(1, &framebufferId);
			}
		}

		// ---------------- Buffers ----------------
		BufferHandle CreateBuffer(const BufferDesc& desc) override
		{
			GLuint bufferId = 0;
			glGenBuffers(1, &bufferId);

			const GLenum target = BufferTargetFor(desc.bindFlag);
			bufferTargets_[bufferId] = target;

			glBindBuffer(target, bufferId);
			glBufferData(target, static_cast<GLsizeiptr>(desc.sizeInBytes), nullptr, BufferUsageFor(desc.usageFlag));
			glBindBuffer(target, 0);

			InvalidateVaoCache();
			return BufferHandle{ static_cast<std::uint32_t>(bufferId) };
		}

		void UpdateBuffer(BufferHandle buffer, std::span<const std::byte> data, std::size_t offsetBytes) override
		{
			const GLuint bufferId = static_cast<GLuint>(buffer.id);
			if (bufferId == 0 || data.empty())
			{
				return;
			}

			const GLenum target = BufferTargetForId(bufferId);
			glBindBuffer(target, bufferId);
			glBufferSubData(target, static_cast<GLintptr>(offsetBytes), static_cast<GLsizeiptr>(data.size()), data.data());
			glBindBuffer(target, 0);
		}

		void DestroyBuffer(BufferHandle buffer) noexcept override
		{
			GLuint bufferId = static_cast<GLuint>(buffer.id);
			if (bufferId != 0)
			{
				glDeleteBuffers(1, &bufferId);
				bufferTargets_.erase(bufferId);
			}
			InvalidateVaoCache();
		}
		// ---------------- Input Layouts ----------------
		InputLayoutHandle CreateInputLayout(const InputLayoutDesc& desc) override
		{
			GLInputLayout glLayout{};
			glLayout.strideBytes = desc.strideBytes;
			glLayout.debugName = desc.debugName;
			glLayout.attribs.reserve(desc.attributes.size());

			for (const auto& attribute : desc.attributes)
			{
				if (attribute.inputSlot != 0)
				{
					throw std::runtime_error("OpenGLRHI: multiple vertex input slots are not supported yet (inputSlot != 0).");
				}

				GLint comps = 0;
				GLenum type = GL_FLOAT;
				VertexFormatToGL(attribute.format, comps, type);

				GLAttrib out{};
				out.location = static_cast<GLuint>(DefaultLocation(attribute.semantic, attribute.semanticIndex));
				out.componentCount = comps;
				out.type = type;
				out.normalized = (attribute.normalized ? GL_TRUE : GL_FALSE);
				out.integerInput = (IsIntegerVertexFormat(attribute.format) ? GL_TRUE : GL_FALSE);
				out.offsetBytes = static_cast<GLuint>(attribute.offsetBytes);
				out.inputSlot = attribute.inputSlot;

				glLayout.attribs.push_back(out);
			}

			const std::uint32_t layoutId = static_cast<std::uint32_t>(inputLayouts_.size()) + 1u;
			inputLayouts_.push_back(std::move(glLayout));

			InvalidateVaoCache();
			return rhi::InputLayoutHandle{ layoutId };
		}

		void DestroyInputLayout(InputLayoutHandle layout) noexcept override
		{
			const std::uint32_t layoutId = layout.id;
			if (layoutId == 0)
			{
				return;
			}

			const std::size_t idx = static_cast<std::size_t>(layoutId - 1);
			if (idx < inputLayouts_.size())
			{
				inputLayouts_[idx] = GLInputLayout{};
			}

			InvalidateVaoCache();
		}

		// ---------------- Shaders / Pipeline ----------------
		ShaderHandle CreateShader(
			ShaderStage stage,
			std::string_view debugName,
			std::string_view sourceOrBytecode) override
		{
			const GLenum shaderType = ToGLShaderStage(stage);
			GLuint shaderId = glCreateShader(shaderType);
			if (shaderId == 0)
			{
				throw std::runtime_error("Failed to create (GL) shader object (" + std::string(debugName) + ")");
			}

			std::string sourceWithVersion = EnsureGLSLVersion(sourceOrBytecode);
			const char* sourceCStr = sourceWithVersion.c_str();
			GLint length = static_cast<GLint>(sourceWithVersion.size());
			glShaderSource(shaderId, 1, &sourceCStr, &length);
			glCompileShader(shaderId);

			RHI_GL_UTILS::ThrowIfShaderCompilationFailed(shaderId, debugName);
			return ShaderHandle{ static_cast<std::uint32_t>(shaderId) };
		}

		void DestroyShader(ShaderHandle shader) noexcept override
		{
			GLuint shaderId = static_cast<GLuint>(shader.id);
			if (shaderId != 0)
			{
				glDeleteShader(shaderId);
			}
		}

		PipelineHandle CreatePipeline(std::string_view debugName, ShaderHandle vertexShader, ShaderHandle pixelShader, PrimitiveTopologyType) override
		{
			GLuint programId = glCreateProgram();
			if (programId == 0)
			{
				throw std::runtime_error("Failed to create (GL) program object (" + std::string(debugName) + ")");
			}

			glAttachShader(programId, static_cast<GLuint>(vertexShader.id));
			glAttachShader(programId, static_cast<GLuint>(pixelShader.id));
			glLinkProgram(programId);

			RHI_GL_UTILS::ThrowIfProgramLinkFailed(programId, debugName);

			glDetachShader(programId, static_cast<GLuint>(vertexShader.id));
			glDetachShader(programId, static_cast<GLuint>(pixelShader.id));

			return PipelineHandle{ static_cast<std::uint32_t>(programId) };
		}

		void DestroyPipeline(PipelineHandle pso) noexcept override
		{
			GLuint programId = static_cast<GLuint>(pso.id);
			if (programId != 0)
			{
				glDeleteProgram(programId);
			}
		}

		// ---------------- Command submission ----------------
		void SubmitCommandList(CommandList&& commandList) override
		{
			for (const auto& command : commandList.commands)
			{
				std::visit([this](auto&& cmd) { ExecuteOnce(cmd); }, command);
			}
		}

		// ---------------- Texture descriptors ----------------
		TextureDescIndex AllocateTextureDesctiptor(TextureHandle texture) override
		{
			if (!freeTextureDescIndices_.empty())
			{
				const TextureDescIndex index = freeTextureDescIndices_.back();
				freeTextureDescIndices_.pop_back();
				textureDescriptions_[index] = texture;
				return index;

			}

			const TextureDescIndex index = static_cast<TextureDescIndex>(textureDescriptions_.size());
			textureDescriptions_.push_back(texture);
			return index;
		}

		void UpdateTextureDescriptor(TextureDescIndex index, TextureHandle texture) override
		{
			if (index == 0)
			{
				return;
			}
			const size_t vecIndex = static_cast<size_t>(index);
			if (vecIndex >= textureDescriptions_.size())
			{
				textureDescriptions_.resize(vecIndex + 1);
			}
			textureDescriptions_[vecIndex] = texture;
		}

		void FreeTextureDescriptor(TextureDescIndex index) noexcept override
		{
			if (index == 0)
			{
				return;
			}
			const size_t vecIndex = static_cast<size_t>(index);
			if (vecIndex < textureDescriptions_.size())
			{
				textureDescriptions_[vecIndex] = TextureHandle{};
				freeTextureDescIndices_.push_back(index);
			}
		}

		// ---------------- Fences ----------------
		FenceHandle CreateFence(bool signaled = false) override
		{
			const std::uint32_t fenceId = ++nextFenceId_;
			GLFence fence;
			fence.signaled = signaled;

			if (!signaled)
			{
				fence.sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				glFlush();
			}

			fences_[fenceId] = fence;
			return FenceHandle{ fenceId };
		}

		void DestroyFence(FenceHandle fence) noexcept override
		{
			const std::uint32_t fenceId = fence.id;
			if (fenceId == 0)
			{
				return;
			}

			if (auto it = fences_.find(fenceId); it != fences_.end())
			{
				if (it->second.sync)
				{
					glDeleteSync(it->second.sync);
				}
				fences_.erase(it);
			}
		}

		void SignalFence(FenceHandle fence) override
		{
			GLFence* ptrFence = GetFence(fence);
			if (!ptrFence)
			{
				return;
			}

			if (ptrFence->sync)
			{
				glDeleteSync(ptrFence->sync);
			}

			ptrFence->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			ptrFence->signaled = false;
			glFlush();
		}

		void WaitFence(FenceHandle fence) override
		{
			GLFence* ptrFence = GetFence(fence);
			if (!ptrFence)
			{
				return;
			}

			if (ptrFence->signaled)
			{
				return;
			}

			if (!ptrFence->sync)
			{
				ptrFence->signaled = true;
				return;
			}

			while (true)
			{
				const GLenum res = glClientWaitSync(ptrFence->sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000);
				if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED)
				{
					break;
				}
			}

			glDeleteSync(ptrFence->sync);
			ptrFence->sync = nullptr;
			ptrFence->signaled = true;
		}

		bool IsFenceSignaled(FenceHandle fence) override
		{
			GLFence* ptrFence = GetFence(fence);
			if (!ptrFence)
			{
				return true;
			}

			if (ptrFence->signaled)
			{
				return true;
			}

			if (!ptrFence->sync)
			{
				return true;
			}

			const GLenum res = glClientWaitSync(ptrFence->sync, 0, 0);
			if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED)
			{
				glDeleteSync(ptrFence->sync);
				ptrFence->sync = nullptr;
				ptrFence->signaled = true;
				return true;
			}
			return false;
		}
