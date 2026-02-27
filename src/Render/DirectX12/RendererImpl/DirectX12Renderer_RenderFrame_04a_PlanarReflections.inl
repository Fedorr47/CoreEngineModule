if (settings_.enablePlanarReflections && !planarMirrorDraws.empty())
{
	// Planar mask uses the depth-only Shadow pipeline to avoid writing color.
	struct PlanarMaskPassConstants
	{
		std::array<float, 16> uLightViewProj{};
	};

	struct PlanarGroup
	{
		mathUtils::Vec3 n{};
		float d = 0.0f; // plane: n·x + d = 0
		mathUtils::Vec3 point{};
		std::vector<const PlanarMirrorDraw*> mirrors;
	};

	std::vector<PlanarGroup> groups;
	groups.reserve(planarMirrorDraws.size());

	auto CanonicalizePlane = [&](mathUtils::Vec3 n, const mathUtils::Vec3& point) noexcept -> std::pair<mathUtils::Vec3, float>
		{
			if (mathUtils::Length(n) < 1e-6f)
			{
				return { mathUtils::Vec3(0, 1, 0), 0.0f };
			}
			n = mathUtils::Normalize(n);

			// Ensure the plane normal points towards the ORIGINAL camera (stable orientation).
			if (mathUtils::Dot(n, camPosLocal - point) < 0.0f)
			{
				n = n * -1.0f;
			}

			const float d = -mathUtils::Dot(n, point);
			return { n, d };
		};

	// Group mirrors by (nearly) the same plane to avoid visible seams between tiles.
	const float kNormalCosEps = 0.9995f; // ~1.8 degrees
	const float kDistEps = 0.02f;        // world units

	for (const PlanarMirrorDraw& mirror : planarMirrorDraws)
	{
		if (!mirror.mesh || mirror.mesh->indexCount == 0)
		{
			continue;
		}

		auto [n, d] = CanonicalizePlane(mirror.planeNormal, mirror.planePoint);

		bool merged = false;
		for (auto& g : groups)
		{
			if (mathUtils::Dot(n, g.n) >= kNormalCosEps && std::fabs(d - g.d) <= kDistEps)
			{
				g.mirrors.push_back(&mirror);
				merged = true;
				break;
			}
		}

		if (!merged)
		{
			PlanarGroup g{};
			g.n = n;
			g.d = d;
			g.point = mirror.planePoint;
			g.mirrors.push_back(&mirror);
			groups.push_back(std::move(g));
		}
	}

	// Render each plane-group: (1) mark stencil, (2) draw reflected scene once for the whole group.
	std::uint32_t groupIndex = 0u;
	for (const PlanarGroup& grp : groups)
	{
		if (groupIndex >= settings_.planarReflectionMaxMirrors)
		{
			break;
		}

		// 1) Visible mirror pixels -> stencil = ref (depth tested against current scene depth).
		ctx.commandList.SetState(planarMaskState_);
		ctx.commandList.SetStencilRef(1u + groupIndex);
		ctx.commandList.BindPipeline(psoShadow_);

		PlanarMaskPassConstants maskConstants{};
		const mathUtils::Mat4 viewProjMaskT = mathUtils::Transpose(viewProj);
		std::memcpy(maskConstants.uLightViewProj.data(), mathUtils::ValuePtr(viewProjMaskT), sizeof(float) * 16);
		ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &maskConstants, 1 }));

		for (const PlanarMirrorDraw* mirrorPtr : grp.mirrors)
		{
			const PlanarMirrorDraw& mirror = *mirrorPtr;

			ctx.commandList.BindInputLayout(mirror.mesh->layoutInstanced);
			ctx.commandList.BindVertexBuffer(0, mirror.mesh->vertexBuffer, mirror.mesh->vertexStrideBytes, 0);
			ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, mirror.instanceOffset * instStride);
			ctx.commandList.BindIndexBuffer(mirror.mesh->indexBuffer, mirror.mesh->indexType, 0);
			ctx.commandList.DrawIndexed(mirror.mesh->indexCount, mirror.mesh->indexType, 0, 0, 1, 0);
		}

		// 2) Render reflected scene only inside this stencil region.
		// IMPORTANT: We use the classic stencil-mirror approach (as in Frank Luna's StencilDemo):
		// keep the camera the same, but reflect the *geometry* in the vertex shader about the mirror plane.
		// That way the reflected scene projects correctly onto the mirror pixels.
		const mathUtils::Vec3 planeN = grp.n;
		const mathUtils::Vec3 planePoint = grp.point;

		// Clip plane: keep only geometry "behind" the mirror plane (opposite to the camera side).
		// In D3D, SV_ClipDistance clips when < 0, so we want positive on the kept side.
		const mathUtils::Vec3 clipN = planeN * -1.0f;
		const float clipD = mathUtils::Dot(planeN, planePoint) + 0.01f;

		ctx.commandList.SetState(planarReflectedState_);
		ctx.commandList.SetStencilRef(1u + groupIndex);

		const auto& planarBatches = !captureMainBatchesNoCull.empty() ? captureMainBatchesNoCull : mainBatches;

		for (const Batch& batch : planarBatches)
		{
			if (!batch.mesh || batch.instanceCount == 0)
			{
				continue;
			}

			MaterialPerm perm = MaterialPerm::UseShadow;
			if (batch.materialHandle.id != 0)
			{
				perm = EffectivePerm(scene.GetMaterial(batch.materialHandle));
			}
			else
			{
				if (batch.material.albedoDescIndex != 0)
				{
					perm = perm | MaterialPerm::UseTex;
				}
			}
			if (HasFlag(perm, MaterialPerm::PlanarMirror))
			{
				continue; // avoid self-recursion in planar path
			}

			const bool useTex = HasFlag(perm, MaterialPerm::UseTex);
			const bool useShadow = HasFlag(perm, MaterialPerm::UseShadow);

			ctx.commandList.BindPipeline(PlanarPipelineFor(perm));
			ctx.commandList.BindTextureDesc(0, batch.material.albedoDescIndex);
			ctx.commandList.BindTextureDesc(12, batch.material.normalDescIndex);
			ctx.commandList.BindTextureDesc(13, batch.material.metalnessDescIndex);
			ctx.commandList.BindTextureDesc(14, batch.material.roughnessDescIndex);
			ctx.commandList.BindTextureDesc(15, batch.material.aoDescIndex);
			ctx.commandList.BindTextureDesc(16, batch.material.emissiveDescIndex);

			rhi::TextureDescIndex envDescIndex = scene.skyboxDescIndex;
			bool usingReflectionProbeEnv = false;
			rhi::TextureHandle envArrayTexture{};
			if (batch.materialHandle.id != 0)
			{
				const auto& mat = scene.GetMaterial(batch.materialHandle);
				if (mat.envSource == EnvSource::ReflectionCapture)
				{
					if (settings_.enableReflectionCapture)
					{
						if (batch.reflectionProbeIndex >= 0 && static_cast<std::size_t>(batch.reflectionProbeIndex) < reflectionProbes_.size())
						{
							const auto& probe = reflectionProbes_[static_cast<std::size_t>(batch.reflectionProbeIndex)];
							if (probe.cubeDescIndex != 0 && probe.cube)
							{
								envDescIndex = probe.cubeDescIndex;
								envArrayTexture = probe.cube;
								usingReflectionProbeEnv = true;
							}
						}
						else if (reflectionCubeDescIndex_ != 0 && reflectionCube_)
						{
							envDescIndex = reflectionCubeDescIndex_;
							envArrayTexture = reflectionCube_;
							usingReflectionProbeEnv = true;
						}
					}
				}
				else
				{
					envDescIndex = scene.skyboxDescIndex;
				}
			}

			ctx.commandList.BindTextureDesc(17, envDescIndex);
			if (usingReflectionProbeEnv && envArrayTexture)
			{
				ctx.commandList.BindTexture2DArray(18, envArrayTexture);
			}

			std::uint32_t flags = 0;
			if (useTex) flags |= kFlagUseTex;
			if (useShadow) flags |= kFlagUseShadow;
			if (batch.material.normalDescIndex != 0) flags |= kFlagUseNormal;
			if (batch.material.metalnessDescIndex != 0) flags |= kFlagUseMetalTex;
			if (batch.material.roughnessDescIndex != 0) flags |= kFlagUseRoughTex;
			if (batch.material.aoDescIndex != 0) flags |= kFlagUseAOTex;
			if (batch.material.emissiveDescIndex != 0) flags |= kFlagUseEmissiveTex;
			if (envDescIndex != 0) flags |= kFlagUseEnv;
			if (settings_.enableReflectionCapture && usingReflectionProbeEnv)
			{
				flags |= kFlagEnvForceMip0;
				flags |= kFlagEnvFlipZ;
			}

			PerBatchConstants constants{};
			const mathUtils::Mat4 viewProjT = mathUtils::Transpose(viewProj);
			const mathUtils::Mat4 dirVP_T = mathUtils::Transpose(dirLightViewProj);
			std::memcpy(constants.uViewProj.data(), mathUtils::ValuePtr(viewProjT), sizeof(float) * 16);
			std::memcpy(constants.uLightViewProj.data(), mathUtils::ValuePtr(dirVP_T), sizeof(float) * 16);
			constants.uCameraAmbient = { camPosLocal.x, camPosLocal.y, camPosLocal.z, 0.22f };
			constants.uCameraForward = { camFLocal.x, camFLocal.y, camFLocal.z, clipN.z };
			constants.uBaseColor = { batch.material.baseColor.x, batch.material.baseColor.y, batch.material.baseColor.z, batch.material.baseColor.w };
			constants.uMaterialFlags = { clipN.x, clipN.y, batch.material.shadowBias, AsFloatBits(flags) };
			constants.uPbrParams = { batch.material.metallic, batch.material.roughness, batch.material.ao, batch.material.emissiveStrength };
			constants.uCounts = { static_cast<float>(lightCount), static_cast<float>(spotShadows.size()), static_cast<float>(pointShadows.size()), clipD };
			constants.uShadowBias = { settings_.dirShadowBaseBiasTexels, settings_.spotShadowBaseBiasTexels, settings_.pointShadowBaseBiasTexels, settings_.shadowSlopeScaleTexels };
			constants.uEnvProbeBoxMin = { 0.0f, 0.0f, 0.0f, 0.0f };
			constants.uEnvProbeBoxMax = { 0.0f, 0.0f, 0.0f, 0.0f };
			if (usingReflectionProbeEnv && batch.reflectionProbeIndex >= 0 &&
				static_cast<std::size_t>(batch.reflectionProbeIndex) < reflectionProbes_.size())
			{
				const auto& probe = reflectionProbes_[static_cast<std::size_t>(batch.reflectionProbeIndex)];
				const float h = settings_.reflectionProbeBoxHalfExtent;

				constants.uEnvProbeBoxMin = {
					probe.capturePos.x - h,
					probe.capturePos.y - h,
					probe.capturePos.z - h,
					0.0f
				};

				constants.uEnvProbeBoxMax = {
					probe.capturePos.x + h,
					probe.capturePos.y + h,
					probe.capturePos.z + h,
					0.0f
				};
			}

			ctx.commandList.BindInputLayout(batch.mesh->layoutInstanced);
			ctx.commandList.BindVertexBuffer(0, batch.mesh->vertexBuffer, batch.mesh->vertexStrideBytes, 0);
			ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, batch.instanceOffset * instStride);
			ctx.commandList.BindIndexBuffer(batch.mesh->indexBuffer, batch.mesh->indexType, 0);
			ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &constants, 1 }));
			ctx.commandList.DrawIndexed(batch.mesh->indexCount, batch.mesh->indexType, 0, 0, batch.instanceCount, 0);
		}

		++groupIndex;
	}

	// Restore state for the following passes (transparent / imgui).
	ctx.commandList.SetState(doDepthPrepass ? mainAfterPreDepthState_ : state_);
	ctx.commandList.SetStencilRef(0);
}