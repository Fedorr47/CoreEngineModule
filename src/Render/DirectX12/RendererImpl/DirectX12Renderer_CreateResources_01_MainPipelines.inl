			// Main pipeline permutations (UseTex / UseShadow)
			{
				auto MakeDefines = [](bool useTex, bool useShadow) -> std::vector<std::string>
					{
						std::vector<std::string> defines;
						if (useTex)
						{
							defines.push_back("USE_TEX=1");
						}
						if (useShadow)
						{
							defines.push_back("USE_SHADOW=1");
						}
						return defines;
					};
			
				for (std::uint32_t idx = 0; idx < 4; ++idx)
				{
					const bool useTex = (idx & 1u) != 0;
					const bool useShadow = (idx & 2u) != 0;
					const auto defs = MakeDefines(useTex, useShadow);
			
					const auto vs = shaderLibrary_.GetOrCreateShader(ShaderKey{
						.stage = rhi::ShaderStage::Vertex,
						.name = "VSMain",
						.filePath = shaderPath.string(),
						.defines = defs
						});
					const auto ps = shaderLibrary_.GetOrCreateShader(ShaderKey{
						.stage = rhi::ShaderStage::Pixel,
						.name = "PSMain",
						.filePath = shaderPath.string(),
						.defines = defs
						});
			
					std::string psoName = "PSO_Mesh";
					if (useTex)
					{
						psoName += "_Tex";
					}
					if (useShadow)
					{
						psoName += "_Shadow";
					}
			
					psoMain_[idx] = psoCache_.GetOrCreate(psoName, vs, ps);
				}
			
				state_.depth.testEnable = true;
				state_.depth.writeEnable = true;
				state_.depth.depthCompareOp = rhi::CompareOp::LessEqual;
			
				state_.rasterizer.cullMode = rhi::CullMode::Back;
				state_.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
			
				state_.blend.enable = false;
			
				transparentState_ = state_;
				transparentState_.depth.writeEnable = false;
				transparentState_.blend.enable = true;
				transparentState_.rasterizer.cullMode = rhi::CullMode::None;
			
				// Depth pre-pass state: same raster as opaque, depth test+write enabled.
				preDepthState_ = state_;
			
				// Main pass state when running after a depth pre-pass: keep depth read-only.
				mainAfterPreDepthState_ = state_;
				mainAfterPreDepthState_.depth.writeEnable = false;

				// Planar reflection stencil mask (writes stencil, keeps color untouched via depth-only PSO).
				planarMaskState_ = preDepthState_;
				planarMaskState_.depth.testEnable = true;
				planarMaskState_.depth.writeEnable = false;
				planarMaskState_.depth.depthCompareOp = rhi::CompareOp::LessEqual;
				// Reflection across a plane flips triangle winding (det(R) < 0). Keep backface culling
				// but invert the front-face definition to avoid drawing backfaces (which look black
				// with one-sided lighting).
				planarReflectedState_.rasterizer.cullMode = rhi::CullMode::Back;
				planarReflectedState_.rasterizer.frontFace = rhi::FrontFace::Clockwise;
				planarMaskState_.blend.enable = false;
				planarMaskState_.depth.stencil.enable = true;
				planarMaskState_.depth.stencil.readMask = 0xFFu;
				planarMaskState_.depth.stencil.writeMask = 0xFFu;
				planarMaskState_.depth.stencil.front.failOp = rhi::StencilOp::Keep;
				planarMaskState_.depth.stencil.front.depthFailOp = rhi::StencilOp::Keep;
				planarMaskState_.depth.stencil.front.passOp = rhi::StencilOp::Replace;
				planarMaskState_.depth.stencil.front.compareOp = rhi::CompareOp::Always;
				planarMaskState_.depth.stencil.back = planarMaskState_.depth.stencil.front;

				// Reflected scene pass: stencil-gated overlay inside visible mirror pixels (MVP path).
				planarReflectedState_ = state_;
				planarReflectedState_.depth.testEnable = false;
				planarReflectedState_.depth.writeEnable = false;
				// We reflect the *camera* (LookAtRH), not the *world*, so winding does NOT need flipping.
				// Keep the engine's default FrontFace (CCW) and just cull backfaces to avoid dark backface lighting.
				planarReflectedState_.rasterizer.cullMode = rhi::CullMode::Back;
				planarReflectedState_.blend.enable = false;
				planarReflectedState_.depth.stencil.enable = true;
				planarReflectedState_.depth.stencil.readMask = 0xFFu;
				planarReflectedState_.depth.stencil.writeMask = 0x00u;
				planarReflectedState_.depth.stencil.front.failOp = rhi::StencilOp::Keep;
				planarReflectedState_.depth.stencil.front.depthFailOp = rhi::StencilOp::Keep;
				planarReflectedState_.depth.stencil.front.passOp = rhi::StencilOp::Keep;
				planarReflectedState_.depth.stencil.front.compareOp = rhi::CompareOp::Equal;
				planarReflectedState_.depth.stencil.back = planarReflectedState_.depth.stencil.front;
			}
			
			// Debug cubemap atlas pipeline (fullscreen triangle with a tiny VB).
			{
				const auto dbgPath = corefs::ResolveAsset("shaders\\DebugCubeAtlas_dx12.hlsl");
			
				const auto vsDbg = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Vertex,
					.name = "VSMain",
					.filePath = dbgPath.string(),
					.defines = {}
					});
				const auto psDbg = shaderLibrary_.GetOrCreateShader(ShaderKey{
					.stage = rhi::ShaderStage::Pixel,
					.name = "PSMain",
					.filePath = dbgPath.string(),
					.defines = {}
					});
			
				psoDebugCubeAtlas_ = psoCache_.GetOrCreate("PSO_DebugCubeAtlas", vsDbg, psDbg);
			
				debugCubeAtlasState_ = {};
				debugCubeAtlasState_.depth.testEnable = false;
				debugCubeAtlasState_.depth.writeEnable = false;
				debugCubeAtlasState_.blend.enable = false;
				debugCubeAtlasState_.rasterizer.cullMode = rhi::CullMode::None;
				debugCubeAtlasState_.rasterizer.frontFace = rhi::FrontFace::CounterClockwise;
			
				// Input layout: POSITION.xy + TEXCOORD0.xy
				rhi::InputLayoutDesc il{};
				il.strideBytes = 16;
				il.attributes = {
					rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::Position, .semanticIndex = 0, .format = rhi::VertexFormat::R32G32_FLOAT, .inputSlot = 0, .offsetBytes = 0 },
					rhi::VertexAttributeDesc{.semantic = rhi::VertexSemantic::TexCoord, .semanticIndex = 0, .format = rhi::VertexFormat::R32G32_FLOAT, .inputSlot = 0, .offsetBytes = 8 },
				};
				debugCubeAtlasLayout_ = device_.CreateInputLayout(il);
			
				struct DebugFSVertex { float px, py; float ux, uy; };
				const DebugFSVertex tri[3] = {
					{ -1.0f, -1.0f, 0.0f, 0.0f },
					{ -1.0f,  3.0f, 0.0f, 2.0f },
					{  3.0f, -1.0f, 2.0f, 0.0f },
				};
			
				rhi::BufferDesc vbDesc{};
				vbDesc.bindFlag = rhi::BufferBindFlag::VertexBuffer;
				vbDesc.usageFlag = rhi::BufferUsageFlag::Default;
				vbDesc.sizeInBytes = sizeof(tri);
				vbDesc.debugName = "DebugCubeAtlasVB";
				debugCubeAtlasVB_ = device_.CreateBuffer(vbDesc);
				if (debugCubeAtlasVB_)
				{
					device_.UpdateBuffer(debugCubeAtlasVB_, std::as_bytes(std::span{ tri, 3 }));
					debugCubeAtlasVBStrideBytes_ = 16;
				}
			}