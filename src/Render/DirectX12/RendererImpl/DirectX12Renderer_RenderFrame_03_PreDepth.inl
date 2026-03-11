// ---------------- Optional depth pre-pass (swapchain depth) ----------------
const bool doDepthPrepass = settings_.enableDepthPrepass && !settings_.enableDeferred;
if (doDepthPrepass && psoShadow_)
{
	// We use the existing depth-only shadow shader (writes SV_Depth, no color outputs).
	// It expects a single matrix (uLightViewProj), so we feed it the camera view-projection.
	rhi::ClearDesc preClear{};
	preClear.clearColor = false; // keep backbuffer untouched
	preClear.clearDepth = true;
	preClear.depth = 1.0f;

	graph.AddSwapChainPass("PreDepthPass", preClear,
		[this, &scene, shadowBatches, skinnedOpaqueDraws, instStride](renderGraph::PassContext& ctx) mutable
		{
			const auto extent = ctx.passExtent;
			ctx.commandList.SetViewport(0, 0,
				static_cast<int>(extent.width),
				static_cast<int>(extent.height));

			// Pre-depth state: depth test+write, opaque raster.
			ctx.commandList.SetState(preDepthState_);
			ctx.commandList.BindPipeline(psoShadow_);

			const FrameCameraData camera = BuildFrameCameraData(scene, extent);

			SingleMatrixPassConstants c{};
			const mathUtils::Mat4 vpT = mathUtils::Transpose(camera.viewProj);
			std::memcpy(c.uLightViewProj.data(), mathUtils::ValuePtr(vpT), sizeof(float) * 16);
			ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));

			this->DrawInstancedShadowBatches(ctx.commandList, shadowBatches, instStride);

			if (psoShadowSkinned_ && skinPaletteBuffer_)
			{
				ctx.commandList.BindStructuredBufferSRV(19, skinPaletteBuffer_);
				for (const SkinnedOpaqueDraw& draw : skinnedOpaqueDraws)
				{
					if (!draw.mesh || draw.boneCount == 0)
					{
						continue;
					}

					ctx.commandList.BindPipeline(psoShadowSkinned_);
					SkinnedSingleMatrixPassConstants skinnedConstants{};
					std::memcpy(skinnedConstants.uLightViewProj.data(), mathUtils::ValuePtr(vpT), sizeof(float) * 16);
					const mathUtils::Mat4 modelT = mathUtils::Transpose(draw.model);
					std::memcpy(skinnedConstants.uModel.data(), mathUtils::ValuePtr(modelT), sizeof(float) * 16);
					skinnedConstants.uSkinning = { static_cast<float>(draw.paletteOffset), static_cast<float>(draw.boneCount), 0.0f, 0.0f };
					ctx.commandList.BindInputLayout(draw.mesh->layout);
					ctx.commandList.BindVertexBuffer(0, draw.mesh->vertexBuffer, draw.mesh->vertexStrideBytes, 0);
					ctx.commandList.BindIndexBuffer(draw.mesh->indexBuffer, draw.mesh->indexType, 0);
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &skinnedConstants, 1 }));
					ctx.commandList.DrawIndexed(draw.mesh->indexCount, draw.mesh->indexType, 0, 0);
				}
			}
		});
}