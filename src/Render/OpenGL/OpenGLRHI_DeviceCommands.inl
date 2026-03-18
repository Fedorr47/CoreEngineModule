		void InvalidateVaoCache()
		{
			for (auto& [_, vao] : vaoCache_)
			{
				if (vao != 0)
				{
					glDeleteVertexArrays(1, &vao);
				}
			}
			vaoCache_.clear();
			boundVao_ = 0;
		}

		GLuint GetOrCreateVAO(bool requireIndexBuffer)
		{
			// Currently: support only slot 0.
			if (currentLayout_.id == 0)
			{
				throw std::runtime_error("OpenGLRHI: Draw called without InputLayout bound.");
			}
			if (vertexBuffer_[0].buffer.id == 0)
			{
				throw std::runtime_error("OpenGLRHI: Draw called without VertexBuffer bound.");
			}
			if (requireIndexBuffer && indexBuffer_.buffer.id == 0)
			{
				throw std::runtime_error("OpenGLRHI: DrawIndexed called without IndexBuffer bound.");
			}

			const GLInputLayout* layout = GetLayout(currentLayout_);
			if (!layout)
			{
				throw std::runtime_error("OpenGLRHI: invalid InputLayout handle.");
			}

			const GLuint vbId = static_cast<GLuint>(vertexBuffer_[0].buffer.id);
			const GLuint ibId = static_cast<GLuint>(indexBuffer_.buffer.id);

			VaoKey key{};
			key.layoutId = currentLayout_.id;
			key.vbId = vbId;
			key.vbOffset = vertexBuffer_[0].offsetBytes;
			key.vbStride = (vertexBuffer_[0].strideBytes != 0) ? vertexBuffer_[0].strideBytes : layout->strideBytes;
			key.ibId = requireIndexBuffer ? ibId : 0u;
			key.ibOffset = requireIndexBuffer ? indexBuffer_.offsetBytes : 0u;
			key.indexType = indexBuffer_.indexType;

			if (auto it = vaoCache_.find(key); it != vaoCache_.end())
			{
				return it->second;
			}

			GLuint vao = 0;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vbId);
			if (requireIndexBuffer)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibId);
			}

			const GLsizei stride = static_cast<GLsizei>(key.vbStride);

			for (const auto& attribute : layout->attribs)
			{
				const GLuint loc = attribute.location;
				glEnableVertexAttribArray(loc);

				const std::uintptr_t ptrOffset = static_cast<std::uintptr_t>(key.vbOffset) + static_cast<std::uintptr_t>(attribute.offsetBytes);
				if (attribute.integerInput == GL_TRUE)
				{
					glVertexAttribIPointer(
						loc,
						attribute.componentCount,
						attribute.type,
						stride,
						reinterpret_cast<const void*>(ptrOffset));
				}
				else
				{
					glVertexAttribPointer(
						loc,
						attribute.componentCount,
						attribute.type,
						attribute.normalized,
						stride,
						reinterpret_cast<const void*>(ptrOffset));
				}
			}

			glBindVertexArray(0);

			vaoCache_.emplace(key, vao);
			return vao;
		}

		// -------- State / uniforms --------
		void ApplyState(const GraphicsState& state)
		{
			// Depth state
			if (state.depth.testEnable)
			{
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDisable(GL_DEPTH_TEST);
			}

			glDepthMask(state.depth.writeEnable ? GL_TRUE : GL_FALSE);
			glDepthFunc(ToGLCompareOp(state.depth.depthCompareOp));

			// Stencil state
			if (state.depth.stencil.enable)
			{
				glEnable(GL_STENCIL_TEST);
				glStencilMaskSeparate(GL_FRONT, state.depth.stencil.writeMask);
				glStencilMaskSeparate(GL_BACK, state.depth.stencil.writeMask);
				glStencilFuncSeparate(GL_FRONT, ToGLCompareOp(state.depth.stencil.front.compareOp), static_cast<GLint>(currentStencilRef_ & 0xFFu), state.depth.stencil.readMask);
				glStencilFuncSeparate(GL_BACK, ToGLCompareOp(state.depth.stencil.back.compareOp), static_cast<GLint>(currentStencilRef_ & 0xFFu), state.depth.stencil.readMask);
				glStencilOpSeparate(GL_FRONT, ToGLStencilOp(state.depth.stencil.front.failOp), ToGLStencilOp(state.depth.stencil.front.depthFailOp), ToGLStencilOp(state.depth.stencil.front.passOp));
				glStencilOpSeparate(GL_BACK, ToGLStencilOp(state.depth.stencil.back.failOp), ToGLStencilOp(state.depth.stencil.back.depthFailOp), ToGLStencilOp(state.depth.stencil.back.passOp));
			}
			else
			{
				glDisable(GL_STENCIL_TEST);
			}

			// Raster
			if (state.rasterizer.cullMode != rhi::CullMode::None)
			{
				glEnable(GL_CULL_FACE);
				glCullFace(ToGLCullMode(state.rasterizer.cullMode));
			}
			else
			{
				glDisable(GL_CULL_FACE);
			}
			glFrontFace(ToGLFrontFace(state.rasterizer.frontFace));

			// Blend
			if (state.blend.enable)
			{
				glEnable(GL_BLEND);
				if (state.blend.mode == BlendMode::Additive)
				{
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				}
				else
				{
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				}
				glBlendEquation(GL_FUNC_ADD);
			}
			else
			{
				glDisable(GL_BLEND);
			}
		}

		GLint GetUniformLocationCached(const std::string& name)
		{
			if (currentProgram_ == 0)
			{
				return -1;
			}

			auto& uCache = uniformLocationCache_[currentProgram_];
			if (auto it = uCache.find(name); it != uCache.end())
			{
				return it->second;
			}

			GLint loc = glGetUniformLocation(currentProgram_, name.c_str());
			uCache.emplace(name, loc);
			return loc;
		}

		void SetUniformIntImpl(const std::string& name, int value)
		{
			if (currentProgram_ == 0)
			{
				return;
			}
			GLint location = glGetUniformLocation(currentProgram_, name.c_str());
			if (location != -1)
			{
				glUniform1i(location, value);
			}
		}

		void SetUniformFloat4Impl(const std::string& name, const std::array<float, 4>& value)
		{
			if (currentProgram_ == 0)
			{
				return;
			}
			GLint location = glGetUniformLocation(currentProgram_, name.c_str());
			if (location != -1)
			{
				glUniform4f(location, value[0], value[1], value[2], value[3]);
			}
		}

		void SetUniformMat4Impl(const std::string& name, const std::array<float, 16>& value)
		{
			if (currentProgram_ == 0)
			{
				return;
			}
			GLint location = glGetUniformLocation(currentProgram_, name.c_str());
			if (location != -1)
			{
				glUniformMatrix4fv(location, 1, GL_FALSE, value.data());
			}
		}

		//----------------------   ExecuteOnce section -------------------------------//
		void ExecuteOnce(const CommandBeginPass& cmd)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(cmd.desc.frameBuffer.id));
			glViewport(0, 0, static_cast<GLsizei>(cmd.desc.extent.width), static_cast<GLsizei>(cmd.desc.extent.height));

			GLbitfield clearMask = 0;
			if (cmd.desc.clearDesc.clearColor)
			{
				clearMask |= GL_COLOR_BUFFER_BIT;
				glClearColor(
					cmd.desc.clearDesc.color[0],
					cmd.desc.clearDesc.color[1],
					cmd.desc.clearDesc.color[2],
					cmd.desc.clearDesc.color[3]);
			}
			if (cmd.desc.clearDesc.clearDepth)
			{
				clearMask |= GL_DEPTH_BUFFER_BIT;
				glClearDepth(static_cast<GLdouble>(cmd.desc.clearDesc.depth));
			}
			if (cmd.desc.clearDesc.clearStencil)
			{
				clearMask |= GL_STENCIL_BUFFER_BIT;
				glClearStencil(static_cast<GLint>(cmd.desc.clearDesc.stencil));
			}
			if (clearMask != 0)
			{
				glClear(clearMask);
			}
		}

		void ExecuteOnce(const CommandEndPass& /*cmd*/) {}

		void ExecuteOnce(const CommandSetViewport& cmd)
		{
			glViewport(cmd.x, cmd.y, cmd.width, cmd.height);
		}

		void ExecuteOnce(const CommandSetState& cmd)
		{
			currentGraphicsState_ = cmd.state;
			ApplyState(cmd.state);
		}

		void ExecuteOnce(const CommandSetStencilRef& cmd)
		{
			currentStencilRef_ = (cmd.ref & 0xFFu);
			if (currentGraphicsState_.depth.stencil.enable)
			{
				ApplyState(currentGraphicsState_);
			}
		}

		void ExecuteOnce(const CommandBindPipeline& cmd)
		{
			const GLuint programId = static_cast<GLuint>(cmd.pso.id);
			if (currentProgram_ != programId)
			{
				glUseProgram(programId);
				currentProgram_ = programId;
			}
		}

		void ExecuteOnce(const rhi::CommandBindInputLayout& cmd)
		{
			currentLayout_ = cmd.layout;
		}

		void ExecuteOnce(const rhi::CommandBindVertexBuffer& cmd)
		{
			if (cmd.slot != 0)
			{
				throw std::runtime_error("OpenGLRHI: only vertex buffer slot 0 is supported right now.");
			}
			vertexBuffer_[0].buffer = cmd.buffer;
			vertexBuffer_[0].strideBytes = cmd.strideBytes;
			vertexBuffer_[0].offsetBytes = cmd.offsetBytes;
		}

		void ExecuteOnce(const rhi::CommandBindIndexBuffer& cmd)
		{
			indexBuffer_.buffer = cmd.buffer;
			indexBuffer_.indexType = cmd.indexType;
			indexBuffer_.offsetBytes = cmd.offsetBytes;
		}

		void ExecuteOnce(const CommnadBindTexture2D& cmd)
		{
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(cmd.slot));
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(cmd.texture.id));
		}

		void ExecuteOnce(const CommandBindTextureCube& cmd)
		{
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(cmd.slot));
			glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<GLuint>(cmd.texture.id));
		}

		void ExecuteOnce(const CommandTextureDesc& cmd)
		{
			const TextureHandle& texture = ResolveTextureDesc(cmd.texture);
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(cmd.slot));
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture.id));
		}

		 		void ExecuteOnce(const CommandBindStructuredBufferSRV& /*cmd*/)
		{
			// Stage-1: OpenGL backend ignores structured-buffer SRVs.
		}

		void ExecuteOnce(const CommandSetUniformInt& cmd)
		{
			SetUniformIntImpl(cmd.name, cmd.value);
		}

		void ExecuteOnce(const CommandUniformFloat4& cmd)
		{
			SetUniformFloat4Impl(cmd.name, cmd.value);
		}

		void ExecuteOnce(const CommandUniformMat4& cmd)
		{
			SetUniformMat4Impl(cmd.name, cmd.value);
		}

		void ExecuteOnce(const CommandSetConstants& /*cmd*/)
		{
			// OpenGL backend currently uses the name-based uniform path.
			// The constant-block command is primarily for DX12 (root CBV update).
		}

		void ExecuteOnce(const CommandDrawIndexed& cmd)
		{
			const GLuint vao = GetOrCreateVAO(true);
			if (boundVao_ != vao)
			{
				glBindVertexArray(vao);
				boundVao_ = vao;
			}

			const GLenum indexType = ToGLIndexType(cmd.indexType);
			const std::uintptr_t start = static_cast<std::uintptr_t>(indexBuffer_.offsetBytes)
				+ static_cast<std::uintptr_t>(cmd.firstIndex) * static_cast<std::uintptr_t>(IndexSizeBytes(cmd.indexType));

			if (cmd.baseVertex != 0)
			{
				glDrawElementsBaseVertex(
					currentTopology_,
					static_cast<GLsizei>(cmd.indexCount),
					indexType,
					reinterpret_cast<const void*>(start),
					cmd.baseVertex);
			}
			else
			{
				glDrawElements(
					currentTopology_,
					static_cast<GLsizei>(cmd.indexCount),
					indexType,
					reinterpret_cast<const void*>(start));
			}
		}

		void ExecuteOnce(const CommandDraw& cmd)
		{
			const GLuint vao = GetOrCreateVAO(false);
			if (boundVao_ != vao)
			{
				glBindVertexArray(vao);
				boundVao_ = vao;
			}

			glDrawArrays(currentTopology_, static_cast<GLint>(cmd.firstVertex), static_cast<GLsizei>(cmd.vertexCount));
		}

		void ExecuteOnce(const CommandBindTexture2DArray& cmd)
		{
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(cmd.slot));
			glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<GLuint>(cmd.texture.id));
		}

		void ExecuteOnce(const CommandSetPrimitiveTopology& cmd)
		{
			// Cached GLenum used by Draw/DrawIndexed.
			switch (cmd.topology)
			{
			case rhi::PrimitiveTopology::TriangleList:
				currentTopology_ = GL_TRIANGLES;
				break;
			case rhi::PrimitiveTopology::LineList:
				currentTopology_ = GL_LINES;
				break;
			default:
				currentTopology_ = GL_TRIANGLES;
				break;
			}
		}

		void ExecuteOnce(const CommandDX12ImGuiRender& /*cmd*/)
		{
			// DX12-only command. Other backends intentionally ignore it.
		}

