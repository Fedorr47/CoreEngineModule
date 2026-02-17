			// ---------------- Optional depth pre-pass (swapchain depth) ----------------
			const bool doDepthPrepass = settings_.enableDepthPrepass;
			if (doDepthPrepass && psoShadow_)
			{
				// We use the existing depth-only shadow shader (writes SV_Depth, no color outputs).
				// It expects a single matrix (uLightViewProj), so we feed it the camera view-projection.
				rhi::ClearDesc preClear{};
				preClear.clearColor = false; // keep backbuffer untouched
				preClear.clearDepth = true;
				preClear.depth = 1.0f;

				graph.AddSwapChainPass("PreDepthPass", preClear,
					[this, &scene, shadowBatches, instStride](renderGraph::PassContext& ctx) mutable
					{
						const auto extent = ctx.passExtent;
						ctx.commandList.SetViewport(0, 0,
							static_cast<int>(extent.width),
							static_cast<int>(extent.height));

						// Pre-depth state: depth test+write, opaque raster.
						ctx.commandList.SetState(preDepthState_);
						ctx.commandList.BindPipeline(psoShadow_);

						const float aspect = extent.height
							? (static_cast<float>(extent.width) / static_cast<float>(extent.height))
							: 1.0f;

						const mathUtils::Mat4 proj = mathUtils::PerspectiveRH_ZO(
							mathUtils::DegToRad(scene.camera.fovYDeg),
							aspect,
							scene.camera.nearZ,
							scene.camera.farZ);
						const mathUtils::Mat4 view = mathUtils::LookAt(scene.camera.position, scene.camera.target, scene.camera.up);
						const mathUtils::Mat4 viewProj = proj * view;

						struct alignas(16) PreDepthConstants
						{
							std::array<float, 16> uLightViewProj{};
						};
						PreDepthConstants c{};
						const mathUtils::Mat4 vpT = mathUtils::Transpose(viewProj);
						std::memcpy(c.uLightViewProj.data(), mathUtils::ValuePtr(vpT), sizeof(float) * 16);
						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

						this->DrawInstancedShadowBatches(ctx.commandList, shadowBatches, instStride);
					});
			}

