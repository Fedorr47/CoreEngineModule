// --- Editor selection (opaque) over deferred SceneColor ---
if (!selectionOpaque.empty())
{
	renderGraph::PassAttachments att{};
	att.useSwapChainBackbuffer = false;
	att.colors = { sceneColorAfterFog };
	att.depth = depthRG;
	att.clearDesc.clearColor = false;
	att.clearDesc.clearDepth = false;
	att.clearDesc.clearStencil = false;

	graph.AddPass("DeferredSelectionOpaque", std::move(att),
		[this, &scene,
		sceneColor,
		depthRG,
		dirLightViewProj,
		selectionOpaque,
		selectionOpaqueStart,
		selectionInstances,
		DrawEditorSelectionGroup,
		instStride](renderGraph::PassContext& ctx)
		{
			const auto extent = ctx.passExtent;
			ctx.commandList.SetViewport(0, 0,
				static_cast<int>(extent.width),
				static_cast<int>(extent.height));

			const FrameCameraData camera = BuildFrameCameraData(scene, extent);
			const mathUtils::Mat4& viewProj = camera.viewProj;
			const mathUtils::Vec3& camPosLocal = camera.camPos;
			const mathUtils::Vec3& camFLocal = camera.camForward;

			DrawEditorSelectionGroup(
				ctx,
				state_,
				mathUtils::Vec4(1.0f, 0.86f, 0.10f, 0.22f),
				viewProj,
				dirLightViewProj,
				camPosLocal,
				camFLocal,
				extent,
				selectionOpaque,
				selectionOpaqueStart);
		});
}
// --- Transparent forward pass over deferred SceneColor ---
if (!transparentDraws.empty())
{
	renderGraph::PassAttachments att{};
	att.useSwapChainBackbuffer = false;
	att.colors = { sceneColorAfterFog };
	att.depth = depthRG;

	att.clearDesc.clearColor = false;
	att.clearDesc.clearDepth = false;
	att.clearDesc.clearStencil = false;

	graph.AddPass("DeferredTransparent", std::move(att),
		[this, &scene,
		shadowRG,
		dirLightViewProj,
		lightCount,
		spotShadows,
		pointShadows,
		transparentDraws,
		activeReflectionProbeCount,
		BindMainPassMaterialTextures,
		ResolveTransparentEnvBinding,
		ResolveMainPassMaterialPerm,
		BuildMainPassMaterialFlags,
		FillPerBatchViewLightingConstants,
		ResetPerBatchEnvProbeBox,
		instStride](renderGraph::PassContext& ctx)
	{
		const auto extent = ctx.passExtent;

		ctx.commandList.SetViewport(0, 0,
			static_cast<int>(extent.width),
			static_cast<int>(extent.height));

		ctx.commandList.SetState(transparentState_);

		const float aspect = extent.height
			? (static_cast<float>(extent.width) / static_cast<float>(extent.height))
			: 1.0f;

		const mathUtils::Mat4 proj = mathUtils::PerspectiveRH_ZO(mathUtils::DegToRad(scene.camera.fovYDeg), aspect, scene.camera.nearZ, scene.camera.farZ);
		const mathUtils::Mat4 view = mathUtils::LookAt(scene.camera.position, scene.camera.target, scene.camera.up);
		const mathUtils::Mat4 viewProj = proj * view;

		const mathUtils::Vec3 camPosLocal = scene.camera.position;
		const mathUtils::Vec3 camFLocal = mathUtils::Normalize(scene.camera.target - scene.camera.position);

		// Bind dir shadow map at t1.
		{
			const auto shadowTex = ctx.resources.GetTexture(shadowRG);
			if (shadowTex)
			{
				ctx.commandList.BindTexture2D(1, shadowTex);
			}
		}

		// Bind Spot shadow maps at t3..t6 and Point shadow cubemaps at t7..t10.
		for (std::size_t spotShadowIndex = 0; spotShadowIndex < spotShadows.size(); ++spotShadowIndex)
		{
			const auto tex = ctx.resources.GetTexture(spotShadows[spotShadowIndex].tex);
			ctx.commandList.BindTexture2D(3 + static_cast<std::uint32_t>(spotShadowIndex), tex);
		}
		for (std::size_t pointShadowIndex = 0; pointShadowIndex < pointShadows.size(); ++pointShadowIndex)
		{
			const auto tex = ctx.resources.GetTexture(pointShadows[pointShadowIndex].cube);
			ctx.commandList.BindTexture2DArray(7 + static_cast<std::uint32_t>(pointShadowIndex), tex);
		}

		// Bind shadow metadata SB at t11
		ctx.commandList.BindStructuredBufferSRV(11, shadowDataBuffer_);

		// Bind lights (t2 StructuredBuffer SRV)
		ctx.commandList.BindStructuredBufferSRV(2, lightsBuffer_);

		for (const TransparentDraw& batchTransparent : transparentDraws)
		{
			if (!batchTransparent.mesh)
			{
				continue;
			}

			const MaterialPerm perm = ResolveMainPassMaterialPerm(
				batchTransparent.material,
				batchTransparent.materialHandle);
			const bool useTex = HasFlag(perm, MaterialPerm::UseTex);
			const bool useShadow = HasFlag(perm, MaterialPerm::UseShadow);
			const ResolvedMaterialEnvBinding env = ResolveTransparentEnvBinding(batchTransparent.materialHandle);

			ctx.commandList.BindPipeline(MainPipelineFor(perm));
			BindMainPassMaterialTextures(ctx.commandList, batchTransparent.material, env);

			const std::uint32_t flags = BuildMainPassMaterialFlags(
				batchTransparent.material,
				useTex,
				useShadow,
				env);

			PerBatchConstants constants{};
			FillPerBatchViewLightingConstants(constants, viewProj, dirLightViewProj, camPosLocal, camFLocal);
			constants.uBaseColor = { batchTransparent.material.baseColor.x, batchTransparent.material.baseColor.y, batchTransparent.material.baseColor.z, batchTransparent.material.baseColor.w };

			const float materialBiasTexels = batchTransparent.material.shadowBias;
			constants.uMaterialFlags = { 0.0f, 0.0f, materialBiasTexels, AsFloatBits(flags) };

			constants.uPbrParams = { batchTransparent.material.metallic, batchTransparent.material.roughness, batchTransparent.material.ao, batchTransparent.material.emissiveStrength };

			constants.uCounts = {
				static_cast<float>(lightCount),
				static_cast<float>(spotShadows.size()),
				static_cast<float>(pointShadows.size()),
				static_cast<float>(activeReflectionProbeCount)
			};

			constants.uShadowBias = {
				settings_.dirShadowBaseBiasTexels,
				settings_.spotShadowBaseBiasTexels,
				settings_.pointShadowBaseBiasTexels,
				settings_.shadowSlopeScaleTexels
			};
			ResetPerBatchEnvProbeBox(constants);

			ctx.commandList.BindInputLayout(batchTransparent.mesh->layoutInstanced);
			ctx.commandList.BindVertexBuffer(0, batchTransparent.mesh->vertexBuffer, batchTransparent.mesh->vertexStrideBytes, 0);
			ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, batchTransparent.instanceOffset * instStride);
			ctx.commandList.BindIndexBuffer(batchTransparent.mesh->indexBuffer, batchTransparent.mesh->indexType, 0);

			ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &constants, 1 }));

			// IMPORTANT: transparent = one object per draw (instanceCount = 1)
			ctx.commandList.DrawIndexed(batchTransparent.mesh->indexCount, batchTransparent.mesh->indexType, 0, 0, 1, 0);
		}
	});
}
// --- Editor selection (transparent) over deferred SceneColor ---
if (!selectionTransparent.empty())
{
	renderGraph::PassAttachments att{};
	att.useSwapChainBackbuffer = false;
	att.colors = { sceneColorAfterFog };
	att.depth = depthRG;
	att.clearDesc.clearColor = false;
	att.clearDesc.clearDepth = false;
	att.clearDesc.clearStencil = false;

	graph.AddPass("DeferredSelectionTransparent", std::move(att),
		[this, &scene,
		dirLightViewProj,
		selectionTransparent,
		selectionTransparentStart,
		selectionInstances,
		DrawEditorSelectionGroup,
		instStride](renderGraph::PassContext& ctx)
		{
			// Reuse the same implementation as DeferredSelectionOpaque by just delegating through the
			// exact same code path (outline+highlight), but after the transparent pass.
			// NOTE: we keep this as a separate pass so ordering stays correct.
			const auto extent = ctx.passExtent;
			ctx.commandList.SetViewport(0, 0,
				static_cast<int>(extent.width),
				static_cast<int>(extent.height));

			const FrameCameraData camera = BuildFrameCameraData(scene, extent);
			const mathUtils::Mat4& viewProj = camera.viewProj;
			const mathUtils::Vec3& camPosLocal = camera.camPos;
			const mathUtils::Vec3& camFLocal = camera.camForward;

			DrawEditorSelectionGroup(
				ctx,
				state_,
				mathUtils::Vec4(1.0f, 0.86f, 0.10f, 0.22f),
				viewProj,
				dirLightViewProj,
				camPosLocal,
				camFLocal,
				extent,
				selectionTransparent,
				selectionTransparentStart);
		});
}
// --- Present: copy SceneColor to swapchain ---
{
	rhi::ClearDesc clear{};
	clear.clearColor = false;
	clear.clearDepth = false;
	clear.clearStencil = false;

	graph.AddSwapChainPass("DeferredPresent", clear,
		[this, sceneColorAfterFog](renderGraph::PassContext& ctx)
		{
			const auto extent = ctx.passExtent;

			ctx.commandList.SetViewport(0, 0,
				static_cast<int>(extent.width),
				static_cast<int>(extent.height));

			ctx.commandList.SetState(copyToSwapChainState_);
			ctx.commandList.BindInputLayout(fullscreenLayout_);
			ctx.commandList.BindPipeline(psoCopyToSwapChain_);

			// t0: SceneColor_Lit				
			ctx.commandList.BindTexture2D(0, ctx.resources.GetTexture(sceneColorAfterFog));
			ctx.commandList.Draw(3, 0);
		});

	// --- ImGui overlay (optional) ---
	if (imguiDrawData)
	{
		rhi::ClearDesc clear{};
		clear.clearColor = false;
		clear.clearDepth = false;
		clear.clearStencil = false;

		graph.AddSwapChainPass("DeferredImGui", clear,
			[this, imguiDrawData](renderGraph::PassContext& ctx)
			{
				ctx.commandList.DX12ImGuiRender(imguiDrawData);
			});
	}
}
}
