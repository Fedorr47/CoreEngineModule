
						// Debug primitives (no ImGui dependency) - rendered in the main view.
						debugDraw::DebugDrawList debugList;
						if (settings_.drawLightGizmos)
						{
							const float scale = settings_.debugLightGizmoScale;
							const float halfSize = settings_.lightGizmoHalfSize * scale;
							const float arrowLen = settings_.lightGizmoArrowLength * scale;
						
							for (const auto& light : scene.lights)
							{
								const std::uint32_t colDir = debugDraw::PackRGBA8(255, 255, 255, 255);
								const std::uint32_t colPoint = debugDraw::PackRGBA8(255, 220, 80, 255);
								const std::uint32_t colSpot = debugDraw::PackRGBA8(80, 220, 255, 255);
						
								switch (light.type)
								{
								case LightType::Directional:
								{
									const mathUtils::Vec3 dir = mathUtils::Normalize(light.direction);
									const mathUtils::Vec3 anchor = scene.camera.target;
									debugList.AddArrow(anchor, anchor + dir * arrowLen, colDir);
									break;
								}
								case LightType::Point:
								{
									const mathUtils::Vec3 p = light.position;
									debugList.AddLine(p - mathUtils::Vec3(halfSize, 0.0f, 0.0f), p + mathUtils::Vec3(halfSize, 0.0f, 0.0f), colPoint);
									debugList.AddLine(p - mathUtils::Vec3(0.0f, halfSize, 0.0f), p + mathUtils::Vec3(0.0f, halfSize, 0.0f), colPoint);
									debugList.AddLine(p - mathUtils::Vec3(0.0f, 0.0f, halfSize), p + mathUtils::Vec3(0.0f, 0.0f, halfSize), colPoint);
									debugList.AddWireSphere(p, halfSize, colPoint, 16);
									break;
								}
								case LightType::Spot:
								{
									const mathUtils::Vec3 p = light.position;
									const mathUtils::Vec3 dir = mathUtils::Normalize(light.direction);
									debugList.AddArrow(p, p + dir * arrowLen, colSpot);
									const float outerRad = mathUtils::DegToRad(light.outerHalfAngleDeg);
									debugList.AddWireCone(p, dir, arrowLen, outerRad, colSpot, 24);
									break;
								}
								default:
									break;
								}
							}
						}
						
						// Pick ray (from the editor UI) visualized in the main view via DebugDraw.
						if (scene.debugPickRay.enabled)
						{
							const std::uint32_t colHit = debugDraw::PackRGBA8(80, 255, 80, 255);
							const std::uint32_t colMiss = debugDraw::PackRGBA8(255, 80, 80, 255);
							const std::uint32_t col = scene.debugPickRay.hit ? colHit : colMiss;
						
							mathUtils::Vec3 dir = scene.debugPickRay.direction;
							const float dirLen = mathUtils::Length(dir);
							if (dirLen > 1e-5f)
							{
								dir = dir / dirLen;
							}
							else
							{
								dir = mathUtils::Vec3(0.0f, 0.0f, 1.0f);
							}
						
							const mathUtils::Vec3 a = scene.debugPickRay.origin;
							const mathUtils::Vec3 b = a + dir * scene.debugPickRay.length;
							debugList.AddLine(a, b, col);
							if (scene.debugPickRay.hit)
							{
								const float cross = settings_.lightGizmoHalfSize * 0.25f;
								debugList.AddAxesCross(b, cross, col);
							}
						}
						
						if (settings_.ShowCubeAtlas)
						{
							// Debug: visualize a cubemap as a 3x2 atlas inset in the main view (bottom-right)
							//
							// Priority:
							//  1) First point shadow cube (distance map, grayscale)
							//  2) Reflection capture cube (color), even if there is no skybox
							std::optional<renderGraph::RGTexture> debugCubeRG{};
							float debugInvRange = 1.0f;
							std::uint32_t debugInvert = 1u;
							std::uint32_t debugMode = 0u; // 0 = depth grayscale, 1 = color

							if (settings_.debugShadowCubeMapType == 0 && !pointShadows.empty())
							{
								const std::uint32_t maxIdx = static_cast<std::uint32_t>(pointShadows.size() - 1u);
								const std::uint32_t idx = std::min(settings_.debugCubeAtlasIndex, maxIdx);
								debugCubeRG = pointShadows[idx].cube;
								// Point shadow map stores normalized distance [0..1] by default.
								debugInvRange = 1.0f;
								debugInvert = 1u;
								debugMode = 0u;
							}
							else if (settings_.debugShadowCubeMapType == 1
								&& settings_.enableReflectionCapture 
								&& reflectionCube_)
							{
								debugCubeRG = graph.ImportTexture(reflectionCube_, renderGraph::RGTextureDesc{
									.extent = reflectionCubeExtent_,
									.format = rhi::Format::RGBA8_UNORM,
									.usage = renderGraph::ResourceUsage::Sampled,
									.type = renderGraph::TextureType::Cube,
									.debugName = "ReflectionCaptureCube_Debug"
									});
								debugInvRange = 1.0f;
								debugInvert = 0u;
								debugMode = 1u;
							}

							if (debugCubeRG && psoDebugCubeAtlas_ && debugCubeAtlasLayout_ && debugCubeAtlasVB_)
							{
								rhi::ClearDesc clear{};
								clear.clearColor = false;
								clear.clearDepth = false;

								const auto cubeRG = *debugCubeRG;

								graph.AddSwapChainPass("DebugPointShadowAtlas", clear,
									[this, cubeRG, debugInvRange, debugInvert, debugMode](renderGraph::PassContext& ctx)
									{
										struct alignas(16) DebugCubeAtlasCB
										{
											float uInvRange;
											float uGamma;
											std::uint32_t uInvert;
											std::uint32_t uShowGrid;
											std::uint32_t uMode;
											std::uint32_t _pad0;
											float uViewportOriginX;
											float uViewportOriginY;
											float uInvViewportSizeX;
											float uInvViewportSizeY;
											float _pad1;
											float _pad2;
										};

										DebugCubeAtlasCB cb{};
										cb.uInvRange = debugInvRange;
										cb.uGamma = 1.0f;
										cb.uInvert = debugInvert;

										cb.uShowGrid = 1u;
										cb.uMode = debugMode;
										cb._pad0 = 0u;

										const std::uint32_t W = std::max(1u, ctx.passExtent.width);
										const std::uint32_t H = std::max(1u, ctx.passExtent.height);
										const std::uint32_t margin = 16u;

										// Keep 3:2 aspect (3 tiles wide, 2 tiles tall)
										std::uint32_t insetW = std::min(512u, (W > margin * 2u) ? (W - margin * 2u) : 128u);
										insetW = std::max(128u, insetW);
										std::uint32_t insetH = (insetW * 2u) / 3u;
										if (insetH + margin * 2u > H)
										{
											insetH = (H > margin * 2u) ? (H - margin * 2u) : 128u;
											insetW = (insetH * 3u) / 2u;
										}

										const std::uint32_t x0 = (W > (margin + insetW)) ? (W - margin - insetW) : 0u;
										const std::uint32_t y0 = (H > (margin + insetH)) ? (H - margin - insetH) : 0u;

										cb.uViewportOriginX = float(x0);
										cb.uViewportOriginY = float(y0);

										cb.uInvViewportSizeX = 1.0f / float(std::max(1u, insetW));
										cb.uInvViewportSizeY = 1.0f / float(std::max(1u, insetH));
										cb._pad1 = 0.0f;
										cb._pad2 = 0.0f;

										ctx.commandList.SetViewport(
											static_cast<int>(x0), static_cast<int>(y0),
											static_cast<int>(insetW), static_cast<int>(insetH));

										ctx.commandList.SetState(debugCubeAtlasState_);
										ctx.commandList.BindPipeline(psoDebugCubeAtlas_);
										ctx.commandList.BindInputLayout(debugCubeAtlasLayout_);
										ctx.commandList.BindVertexBuffer(0, debugCubeAtlasVB_, debugCubeAtlasVBStrideBytes_, 0);
										ctx.commandList.SetPrimitiveTopology(rhi::PrimitiveTopology::TriangleList);

										const auto tex = ctx.resources.GetTexture(cubeRG);
										ctx.commandList.BindTexture2DArray(0, tex); // t0 (Texture2DArray<float4>)

										ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &cb, 1 })); // b0
										ctx.commandList.Draw(3);

										// Restore full viewport for any following swapchain passes.
										ctx.commandList.SetViewport(0, 0, static_cast<int>(W), static_cast<int>(H));
									});
							}
						}
						
						debugDrawRenderer_.Upload(debugList);
						if (debugList.VertexCount() > 0)
						{
							rhi::ClearDesc clear{};
							clear.clearColor = false;
							clear.clearDepth = false;
						
							const mathUtils::Mat4 proj = mathUtils::PerspectiveRH_ZO(mathUtils::DegToRad(scene.camera.fovYDeg), aspect, scene.camera.nearZ, scene.camera.farZ);
							const mathUtils::Mat4 view = mathUtils::LookAt(scene.camera.position, scene.camera.target, scene.camera.up);
							const mathUtils::Mat4 viewProj = proj * view;
						
							graph.AddSwapChainPass("DebugPrimitivesPass", clear, [this, viewProj](renderGraph::PassContext& ctx)
								{
									debugDrawRenderer_.Draw(ctx.commandList, viewProj, settings_.debugDrawDepthTest);
								});
						}
						
						graph.Execute(device_, swapChain);
						swapChain.Present();