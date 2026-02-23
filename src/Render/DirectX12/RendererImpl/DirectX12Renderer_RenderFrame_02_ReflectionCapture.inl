// ---------------- ReflectionCapture pass (cubemap) ----------------
// Uses capture shaders/PSOs from 0007.
// Method selection:
//  - Layered: SV_RenderTargetArrayIndex (requires VPAndRTArrayIndexFromAnyShader + PSO)
//  - VI:      SV_ViewID (requires ViewInstancing + PSO)
//  - Fallback: 6 passes, one face at a time

if (settings_.enableReflectionCapture && reflectionCube_ && psoReflectionCapture_)
{
	// Decide capture position.
	// We want the capture centered on a * probe owner * (a specific Level node), not on the camera.
	// Priority:
	//  1) Debug reflection-atlas owner index (RendererSettings::debugCubeAtlasIndex in Reflection mode)
	//  2) Owner node (Scene::editorReflectionCaptureOwnerNode) if set
	//  3) Follow editor selection (if enabled and selected item is reflective)
	//  4) Otherwise, auto-pick the closest draw item that uses EnvSource::ReflectionCapture (typically MirrorSphere)
	//  5) Fallback to last capture position (or camera position for the very first capture)
	mathUtils::Vec3 capturePos = reflectionCaptureHasLastPos_ ? reflectionCaptureLastPos_ : camPos;
	int captureAnchorDrawItem = -1;

	int anchorKind = 0; // 0=auto/none, 1=selected, 2=owner, 3=debugOwnerIndex
	int anchorNode = -1;

	// IMPORTANT:
	// LevelInstance pushes node transforms into DrawItem::transform as a *matrix* (useMatrix=true),
	// but does NOT populate Transform::position. Using it.transform.position would always read (0,0,0).
	// Extract translation from the matrix when available.
	const auto GetDrawItemWorldPos = [&scene](int drawItemIndex) -> mathUtils::Vec3
		{
			if (drawItemIndex < 0 || static_cast<std::size_t>(drawItemIndex) >= scene.drawItems.size())
			{
				return { 0.0f, 0.0f, 0.0f };
			}
			const DrawItem& di = scene.drawItems[static_cast<std::size_t>(drawItemIndex)];
			if (di.transform.useMatrix)
			{
				const mathUtils::Vec4& t = di.transform.matrix[3]; // column-major translation
				return { t.x, t.y, t.z };
			}
			return di.transform.position;
		};

	const auto IsReflectionCaptureDrawItem = [&scene](int drawItemIndex) -> bool
		{
			if (drawItemIndex < 0 || static_cast<std::size_t>(drawItemIndex) >= scene.drawItems.size())
			{
				return false;
			}
			const DrawItem& di = scene.drawItems[static_cast<std::size_t>(drawItemIndex)];
			if (di.material.id == 0)
			{
				return false;
			}
			const auto& mat = scene.GetMaterial(di.material);
			return mat.envSource == EnvSource::ReflectionCapture;
		};

	auto ResolveDebugReflectionOwnerByIndex = [&scene, this, &IsReflectionCaptureDrawItem]() -> int
		{
			if (!(settings_.ShowCubeAtlas && settings_.debugShadowCubeMapType == 1u))
			{
				return -1;
			}

			std::vector<int> reflectiveDrawItems;
			reflectiveDrawItems.reserve(scene.drawItems.size());
			for (std::size_t i = 0; i < scene.drawItems.size(); ++i)
			{
				if (IsReflectionCaptureDrawItem(static_cast<int>(i)))
				{
					reflectiveDrawItems.push_back(static_cast<int>(i));
				}
			}
			if (reflectiveDrawItems.empty())
			{
				return -1;
			}

			const std::uint32_t maxIdx = static_cast<std::uint32_t>(reflectiveDrawItems.size() - 1u);
			const std::uint32_t idx = std::min(settings_.debugCubeAtlasIndex, maxIdx);
			return reflectiveDrawItems[idx];
		};

	// 1) Debug reflection-atlas owner index (only when cube-atlas debug is showing reflection capture).
	// In this mode, debugCubeAtlasIndex *defines* which reflective owner is being captured/debugged.
	if (anchorKind == 0)
	{
		const int debugOwnerDrawItem = ResolveDebugReflectionOwnerByIndex();
		if (debugOwnerDrawItem >= 0)
		{
			anchorKind = 3;
			anchorNode = -1;
			captureAnchorDrawItem = debugOwnerDrawItem;
			capturePos = GetDrawItemWorldPos(debugOwnerDrawItem);
		}
	}

	// 2) Owner node (stable by LevelAsset index). Draw-item mapping is updated by Level UI.
	const int ownerNode = scene.editorReflectionCaptureOwnerNode;
	const int ownerDrawItem = scene.editorReflectionCaptureOwnerDrawItem;

	if (anchorKind == 0 && ownerNode >= 0)
	{
		anchorKind = 2;
		anchorNode = ownerNode;
		if (IsReflectionCaptureDrawItem(ownerDrawItem))
		{
			captureAnchorDrawItem = ownerDrawItem;
			capturePos = GetDrawItemWorldPos(ownerDrawItem);
		}
	}
	// 3) Follow selected object (editor selection) if no debug owner / explicit owner is set.
	const int selectedDrawItem = scene.editorSelectedDrawItem;

	if (anchorKind == 0 && settings_.reflectionCaptureFollowSelectedObject && IsReflectionCaptureDrawItem(selectedDrawItem))
	{
		anchorKind = 1;
		anchorNode = scene.editorSelectedNode;
		captureAnchorDrawItem = selectedDrawItem;
		const auto& it = scene.drawItems[static_cast<std::size_t>(selectedDrawItem)];
		capturePos = GetDrawItemWorldPos(selectedDrawItem);
	}
	else if (anchorKind == 0)
	{
		// 4) Auto-pick closest reflective item around the last stable capture position.
		const mathUtils::Vec3 pickOrigin = reflectionCaptureHasLastPos_ ? reflectionCaptureLastPos_ : camPos;
		float bestDist2 = 3.402823466e+38f; // FLT_MAX
		for (std::size_t i = 0; i < scene.drawItems.size(); ++i)
		{
			const auto& it = scene.drawItems[i];
			if (it.material.id == 0)
				continue;

			const auto& mat = scene.GetMaterial(it.material);
			if (mat.envSource != EnvSource::ReflectionCapture)
				continue;

			const mathUtils::Vec3 itPos = GetDrawItemWorldPos(static_cast<int>(i));
			const mathUtils::Vec3 d = itPos - pickOrigin;
			const float dist2 = mathUtils::Dot(d, d);
			if (dist2 < bestDist2)
			{
				bestDist2 = dist2;
				captureAnchorDrawItem = static_cast<int>(i);
				capturePos = itPos;
			}
		}

		if (captureAnchorDrawItem < 0)
		{
			// No anchor found: keep last capture pos if available, otherwise fallback to camera position.
			capturePos = reflectionCaptureHasLastPos_ ? reflectionCaptureLastPos_ : camPos;
		}
	}

	// Dirty logic: anchor change or movement.
	if (captureAnchorDrawItem != reflectionCaptureLastSelectedDrawItem_
		|| anchorKind != reflectionCaptureLastAnchorKind_
		|| anchorNode != reflectionCaptureLastAnchorNode_)
	{
		reflectionCaptureLastSelectedDrawItem_ = captureAnchorDrawItem;
		reflectionCaptureLastAnchorKind_ = anchorKind;
		reflectionCaptureLastAnchorNode_ = anchorNode;
		reflectionCaptureDirty_ = true;
	}

	if (!reflectionCaptureHasLastPos_
		|| mathUtils::Length(capturePos - reflectionCaptureLastPos_) > 1e-4f)
	{
		reflectionCaptureDirty_ = true;
	}

	const bool doUpdate =
		settings_.reflectionCaptureUpdateEveryFrame ||
		reflectionCaptureDirty_;

	if (doUpdate)
	{
		reflectionCaptureDirty_ = false;
		reflectionCaptureHasLastPos_ = true;
		reflectionCaptureLastPos_ = capturePos;

		// Import persistent cube textures to render graph.
		const auto cubeRG = graph.ImportTexture(reflectionCube_, renderGraph::RGTextureDesc{
			.extent = reflectionCubeExtent_,
			.format = rhi::Format::RGBA8_UNORM,
			.usage = renderGraph::ResourceUsage::RenderTarget,
			.type = renderGraph::TextureType::Cube,
			.debugName = "ReflectionCaptureCube"
			});

		const auto depthCubeRG = graph.ImportTexture(reflectionDepthCube_, renderGraph::RGTextureDesc{
			.extent = reflectionCubeExtent_,
			.format = rhi::Format::D32_FLOAT,
			.usage = renderGraph::ResourceUsage::DepthStencil,
			.type = renderGraph::TextureType::Cube,
			.debugName = "ReflectionCaptureDepthCube"
			});

		// Capabilities / method choice.
		bool useLayered =
			(!disableReflectionCaptureLayered_) &&
			(psoReflectionCaptureLayered_) &&
			device_.SupportsShaderModel6() &&
			device_.SupportsVPAndRTArrayIndexFromAnyShader();

		bool useVI =
			(!useLayered) &&
			(!disableReflectionCaptureVI_) &&
			(psoReflectionCaptureVI_) &&
			device_.SupportsShaderModel6() &&
			device_.SupportsViewInstancing();

		auto FaceView = [](const mathUtils::Vec3& pos, int face) -> mathUtils::Mat4
			{
				static const mathUtils::Vec3 dirs[6] = {
					{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
				};
				static const mathUtils::Vec3 ups[6] = {
					{ 0, 1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 0 }
				};
				return mathUtils::LookAtRH(pos, pos + dirs[face], ups[face]);
			};

		const float nearZ = std::max(0.001f, settings_.reflectionCaptureNearZ);
		const float farZ = std::max(nearZ + 0.01f, settings_.reflectionCaptureFarZ);
		const mathUtils::Mat4 proj90 = mathUtils::PerspectiveRH_ZO(mathUtils::DegToRad(90.0f), 1.0f, nearZ, farZ);

		// Clear (two variants: full clear for skybox/background, depth-only for overlaying meshes)

		rhi::ClearDesc clearColorDepth{};
		clearColorDepth.clearColor = true;
		clearColorDepth.clearDepth = true;
		clearColorDepth.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearColorDepth.depth = 1.0f;

		rhi::ClearDesc clearDepthOnly{};
		clearDepthOnly.clearColor = false;
		clearDepthOnly.clearDepth = true;
		clearDepthOnly.depth = 1.0f;

		// Optional: render skybox into capture cube first (so reflections have a proper background).
		const std::uint32_t skyboxDesc = scene.skyboxDescIndex;
		const bool haveSkybox = (skyboxDesc != 0);

		// A temporary per-face depth buffer for fallback (and skybox) passes.
		const auto depthTmp = graph.CreateTexture(renderGraph::RGTextureDesc{
			.extent = reflectionCubeExtent_,
			.format = rhi::Format::D32_FLOAT,
			.usage = renderGraph::ResourceUsage::DepthStencil,
			.debugName = "ReflectionCaptureDepthTmp"
			});

		bool renderedSkybox = false;
		
		if (haveSkybox)
		{
			renderedSkybox = true;
			for (int face = 0; face < 6; ++face)
			{
				renderGraph::PassAttachments att{};
				att.useSwapChainBackbuffer = false;
				att.colorCubeFace = static_cast<std::uint32_t>(face);
				att.color = cubeRG;
				att.depth = depthTmp;
				att.clearDesc = clearColorDepth;

				mathUtils::Mat4 view = FaceView(capturePos, face);
				view[3] = mathUtils::Vec4(0, 0, 0, 1);

				const mathUtils::Mat4 viewProjSkybox = proj90 * view;
				const mathUtils::Mat4 viewProjSkyboxTranspose = mathUtils::Transpose(viewProjSkybox);

				SkyboxConstants skyboxConstants{};
				std::memcpy(skyboxConstants.uViewProj.data(), mathUtils::ValuePtr(viewProjSkyboxTranspose), sizeof(float) * 16);

				const std::string passName = "ReflectionCapture_Skybox_Face_" + std::to_string(face);
				graph.AddPass(passName, std::move(att),
					[this, skyboxDesc, skyboxConstants](renderGraph::PassContext& ctx) mutable
					{
						ctx.commandList.SetViewport(0, 0, (int)ctx.passExtent.width, (int)ctx.passExtent.height);
						ctx.commandList.SetState(skyboxState_);
						ctx.commandList.BindPipeline(psoSkybox_);
						ctx.commandList.BindTextureDesc(0, skyboxDesc);

						ctx.commandList.BindInputLayout(skyboxMesh_.layout);
						ctx.commandList.BindVertexBuffer(0, skyboxMesh_.vertexBuffer, skyboxMesh_.vertexStrideBytes, 0);
						ctx.commandList.BindIndexBuffer(skyboxMesh_.indexBuffer, skyboxMesh_.indexType, 0);

						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &skyboxConstants, 1 }));
						ctx.commandList.DrawIndexed(skyboxMesh_.indexCount, skyboxMesh_.indexType, 0, 0);

					});
			}
		}

		// For mesh passes: if we rendered skybox first, don't clear color again.
		const rhi::ClearDesc meshClear = renderedSkybox ? clearDepthOnly : clearColorDepth;

		// NOTE: Capture shaders (ReflectionCapture*_dx12.hlsl) do NOT sample the environment cubemap,
		// so capturing reflective objects does not create recursion/feedback.
		// Keeping all scene geometry here also avoids the case where materials default to EnvSource::ReflectionCapture
		// and would otherwise filter out everything.
		std::vector<Batch> captureMainBatches = (!captureMainBatchesNoCull.empty()) ? captureMainBatchesNoCull : mainBatches;
		std::vector<Batch> captureReflectionBatchesLayered = reflectionBatchesLayered;

		// ---------------- Layered path (one pass, SV_RenderTargetArrayIndex) ----------------
		if (useLayered && !captureReflectionBatchesLayered.empty())
		{
			renderGraph::PassAttachments att{};
			att.useSwapChainBackbuffer = false;
			att.color = cubeRG;
			att.colorCubeAllFaces = true;
			att.depth = depthCubeRG;
			att.clearDesc = meshClear;

			// Precompute face matrices once (stored transposed).
			ReflectionCaptureConstants base{};
			for (int face = 0; face < 6; ++face)
			{
				const mathUtils::Mat4 vp = proj90 * FaceView(capturePos, face);
				const mathUtils::Mat4 vpT = mathUtils::Transpose(vp);
				std::memcpy(base.uFaceViewProj.data() + face * 16, mathUtils::ValuePtr(vpT), sizeof(float) * 16);
			}

			// capturePos.xyz + ambientStrength
			base.uCapturePosAmbient = { capturePos.x, capturePos.y, capturePos.z, 0.22f };
			base.uParams = { static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f };

			graph.AddPass("ReflectionCapture_Layered", std::move(att),
				[this, base, lightCount, instStride, captureReflectionBatchesLayered](renderGraph::PassContext& ctx) mutable
				{
					ctx.commandList.SetViewport(0, 0, (int)ctx.passExtent.width, (int)ctx.passExtent.height);
					ctx.commandList.SetState(state_);
					ctx.commandList.BindPipeline(psoReflectionCaptureLayered_);
					ctx.commandList.BindStructuredBufferSRV(2, lightsBuffer_);

					int captureMainBatchesNum = captureReflectionBatchesLayered.size();
					std::cout << "ReflectionCapture: num batches = " << captureMainBatchesNum << std::endl;

					for (const Batch& b : captureReflectionBatchesLayered)
					{
						if (!b.mesh || b.instanceCount == 0)
							continue;

						assert((b.instanceOffset % 6u) == 0u);
						assert((b.instanceCount % 6u) == 0u);

						// flags
						std::uint32_t flags = 0u;
						const bool useTex = (b.materialHandle.id != 0) && (b.material.albedoDescIndex != 0);
						if (useTex) flags |= 1u; // FLAG_USE_TEX

						// bind albedo (t0)
						ctx.commandList.BindTextureDesc(0, useTex ? b.material.albedoDescIndex : 0);

						ReflectionCaptureConstants c = base;
						c.uBaseColor = { b.material.baseColor.x, b.material.baseColor.y, b.material.baseColor.z, b.material.baseColor.w };
						c.uParams[1] = AsFloatBits(flags);

						ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);
						ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
						ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, b.instanceOffset * instStride);
						ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));
						ctx.commandList.DrawIndexed(b.mesh->indexCount, b.mesh->indexType, 0, 0, b.instanceCount, 0);
					}
				});
		}
		// ---------------- VI path (one pass, SV_ViewID) ----------------
		else if (useVI)
		{
			renderGraph::PassAttachments att{};
			att.useSwapChainBackbuffer = false;
			att.color = cubeRG;
			att.colorCubeAllFaces = true;
			att.depth = depthCubeRG;
			att.clearDesc = meshClear;

			ReflectionCaptureConstants base{};
			for (int face = 0; face < 6; ++face)
			{
				const mathUtils::Mat4 vp = proj90 * FaceView(capturePos, face);
				const mathUtils::Mat4 vpT = mathUtils::Transpose(vp);
				std::memcpy(base.uFaceViewProj.data() + face * 16, mathUtils::ValuePtr(vpT), sizeof(float) * 16);
			}

			base.uCapturePosAmbient = { capturePos.x, capturePos.y, capturePos.z, 0.22f };
			base.uParams = { static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f };

			graph.AddPass("ReflectionCapture_VI", std::move(att),
				[this, base, lightCount, instStride, captureMainBatches](renderGraph::PassContext& ctx) mutable
				{
					ctx.commandList.SetViewport(0, 0, (int)ctx.passExtent.width, (int)ctx.passExtent.height);
					ctx.commandList.SetState(state_);
					ctx.commandList.BindPipeline(psoReflectionCaptureVI_);
					ctx.commandList.BindStructuredBufferSRV(2, lightsBuffer_);

					int captureMainBatchesNum = captureMainBatches.size();
					std::cout << "ReflectionCapture: num batches = " << captureMainBatchesNum << std::endl;

					for (const Batch& b : captureMainBatches)
					{
						if (!b.mesh || b.instanceCount == 0)
							continue;

						std::uint32_t flags = 0u;
						const bool useTex = (b.materialHandle.id != 0) && (b.material.albedoDescIndex != 0);
						if (useTex) flags |= 1u;

						ctx.commandList.BindTextureDesc(0, useTex ? b.material.albedoDescIndex : 0);

						ReflectionCaptureConstants c = base;
						c.uBaseColor = { b.material.baseColor.x, b.material.baseColor.y, b.material.baseColor.z, b.material.baseColor.w };
						c.uParams[1] = AsFloatBits(flags);

						ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);
						ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
						ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, b.instanceOffset * instStride);
						ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

						ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));
						ctx.commandList.DrawIndexed(b.mesh->indexCount, b.mesh->indexType, 0, 0, b.instanceCount, 0);
					}
				});
		}
		// ---------------- Fallback path (6 passes) ----------------
		else
		{
			// Fallback depth: RenderGraph cannot select cube DSV per-face (only all-faces),
			// so we reuse the 2D temp depth texture created above (depthTmp)
			for (int face = 0; face < 6; ++face)
			{
				renderGraph::PassAttachments att{};
				att.useSwapChainBackbuffer = false;
				att.color = cubeRG;
				att.colorCubeFace = static_cast<std::uint32_t>(face);
				att.depth = depthTmp;
				att.clearDesc = meshClear;

				const mathUtils::Mat4 vp = proj90 * FaceView(capturePos, face);
				const mathUtils::Mat4 vpT = mathUtils::Transpose(vp);

				ReflectionCaptureFaceConstants base{};
				std::memcpy(base.uViewProj.data(), mathUtils::ValuePtr(vpT), sizeof(float) * 16);
				base.uCapturePosAmbient = { capturePos.x, capturePos.y, capturePos.z, 0.22f };
				base.uParams = { static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f };

				const std::string passName = "ReflectionCapture_Face_" + std::to_string(face);

				graph.AddPass(passName, std::move(att),
					[this, base, lightCount, instStride, captureMainBatches](renderGraph::PassContext& ctx) mutable
					{
						ctx.commandList.SetViewport(0, 0, (int)ctx.passExtent.width, (int)ctx.passExtent.height);
						ctx.commandList.SetState(state_);
						ctx.commandList.BindPipeline(psoReflectionCapture_);
						ctx.commandList.BindStructuredBufferSRV(2, lightsBuffer_);

						int captureMainBatchesNum = captureMainBatches.size();
						std::cout << "ReflectionCapture: num batches = " << captureMainBatchesNum << std::endl;
	
						for (const Batch& b : captureMainBatches)
						{
							if (!b.mesh || b.instanceCount == 0)
								continue;

							std::uint32_t flags = 0u;
							const bool useTex = (b.materialHandle.id != 0) && (b.material.albedoDescIndex != 0);
							if (useTex) flags |= 1u;

							ctx.commandList.BindTextureDesc(0, useTex ? b.material.albedoDescIndex : 0);

							ReflectionCaptureFaceConstants c = base;
							c.uBaseColor = { b.material.baseColor.x, b.material.baseColor.y, b.material.baseColor.z, b.material.baseColor.w };
							c.uParams[1] = AsFloatBits(flags);

							ctx.commandList.BindInputLayout(b.mesh->layoutInstanced);
							ctx.commandList.BindVertexBuffer(0, b.mesh->vertexBuffer, b.mesh->vertexStrideBytes, 0);
							ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, b.instanceOffset * instStride);
							ctx.commandList.BindIndexBuffer(b.mesh->indexBuffer, b.mesh->indexType, 0);

							ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &c, 1 }));
							ctx.commandList.DrawIndexed(b.mesh->indexCount, b.mesh->indexType, 0, 0, b.instanceCount, 0);
						}
					});
			}
		}
	}
}
