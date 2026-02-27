if (settings_.enablePlanarReflections && !planarMirrorDraws.empty())
{
	// Planar mask uses the depth-only Shadow pipeline to avoid writing color.
	struct PlanarMaskPassConstants
	{
		std::array<float, 16> uLightViewProj{};
	};


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

			const float d = -mathUtils::Dot(n, point); // plane: n·x + d = 0
			return { n, d };
		};
	
	auto ReflectPoint = [](const mathUtils::Vec3& p, const mathUtils::Vec3& n, float d) noexcept -> mathUtils::Vec3
		{
			// p' = p - 2 * (n·p + d) * n
			const float dist = mathUtils::Dot(n, p) + d;
			return p - n * (2.0f * dist);
		};

	auto ReflectVector = [](const mathUtils::Vec3& v, const mathUtils::Vec3& n) noexcept -> mathUtils::Vec3
		{
			// v' = v - 2 * (n·v) * n
			return v - n * (2.0f * mathUtils::Dot(n, v));
		};

	const mathUtils::Mat4 viewProjT = mathUtils::Transpose(viewProj);

	std::uint32_t mirrorIndex = 0u;

	for (const PlanarMirrorDraw& mirror : planarMirrorDraws)
	{
		if (!mirror.mesh || mirror.mesh->indexCount == 0)
		{
			continue;
		}

		
		if (mirrorIndex >= settings_.planarReflectionMaxMirrors)
		{
			break;
		}

		auto [planeN, planeD] = CanonicalizePlane(mirror.planeNormal, mirror.planePoint);
		
		// ---------------- (1) Stencil mask: visible mirror pixels -> stencil = ref ----------------
		ctx.commandList.SetState(planarMaskState_);
		ctx.commandList.SetStencilRef(1u + mirrorIndex);
		ctx.commandList.BindPipeline(psoShadow_);

		PlanarMaskPassConstants maskConstants{};
		std::memcpy(maskConstants.uLightViewProj.data(), mathUtils::ValuePtr(viewProjT), sizeof(float) * 16);
		ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &maskConstants, 1 }));

		ctx.commandList.BindInputLayout(mirror.mesh->layoutInstanced);
		ctx.commandList.BindVertexBuffer(0, mirror.mesh->vertexBuffer, mirror.mesh->vertexStrideBytes, 0);
		ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, mirror.instanceOffset* instStride);
		ctx.commandList.BindIndexBuffer(mirror.mesh->indexBuffer, mirror.mesh->indexType, 0);
		ctx.commandList.DrawIndexed(mirror.mesh->indexCount, mirror.mesh->indexType, 0, 0, 1, 0);

		// ---------------- (2) Reflected scene: reflected camera, stencil-gated ----------------
		const mathUtils::Vec3 reflEye = ReflectPoint(camPosLocal, planeN, planeD);
		const mathUtils::Vec3 reflTarget = ReflectPoint(scene.camera.target, planeN, planeD);
		mathUtils::Vec3 reflUp = ReflectVector(scene.camera.up, planeN);
		if (mathUtils::Length(reflUp) < 1e-6f)
		{
			reflUp = scene.camera.up;
		}

		const mathUtils::Mat4 viewRefl = mathUtils::LookAt(reflEye, reflTarget, reflUp);
		const mathUtils::Mat4 viewProjReflT = mathUtils::Transpose(proj * viewRefl);
		const mathUtils::Vec3 reflForward = mathUtils::Normalize(reflTarget - reflEye);
		
		ctx.commandList.SetState(planarReflectedState_);
		ctx.commandList.SetStencilRef(1u + mirrorIndex);

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

			// Use the regular main pipeline (no special planar defines).
			ctx.commandList.BindPipeline(MainPipelineFor(perm));
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
			const mathUtils::Mat4 dirVP_T = mathUtils::Transpose(dirLightViewProj);
			std::memcpy(constants.uViewProj.data(), mathUtils::ValuePtr(viewProjReflT), sizeof(float) * 16);
			std::memcpy(constants.uLightViewProj.data(), mathUtils::ValuePtr(dirVP_T), sizeof(float) * 16);
			constants.uCameraAmbient = { reflEye.x, reflEye.y, reflEye.z, 0.22f };
			constants.uCameraForward = { reflForward.x, reflForward.y, reflForward.z, 0.0f };
			constants.uBaseColor = { batch.material.baseColor.x, batch.material.baseColor.y, batch.material.baseColor.z, batch.material.baseColor.w };
			
			const float materialBiasTexels = batch.material.shadowBias;
			constants.uMaterialFlags = { 0.0f, 0.0f, materialBiasTexels, AsFloatBits(flags) };

			constants.uPbrParams = { batch.material.metallic, batch.material.roughness, batch.material.ao, batch.material.emissiveStrength };
			constants.uCounts = { static_cast<float>(lightCount), static_cast<float>(spotShadows.size()), static_cast<float>(pointShadows.size()), 0.0f };
			constants.uShadowBias = { settings_.dirShadowBaseBiasTexels, settings_.spotShadowBaseBiasTexels, settings_.pointShadowBaseBiasTexels, settings_.shadowSlopeScaleTexels };
			constants.uEnvProbeBoxMin = { 0.0f, 0.0f, 0.0f, 0.0f };
			constants.uEnvProbeBoxMax = { 0.0f, 0.0f, 0.0f, 0.0f };
			if (usingReflectionProbeEnv && batch.reflectionProbeIndex >= 0 &&
				static_cast<std::size_t>(batch.reflectionProbeIndex) < reflectionProbes_.size())
			{
				const auto& probe = reflectionProbes_[static_cast<std::size_t>(batch.reflectionProbeIndex)];
				const float h = settings_.reflectionProbeBoxHalfExtent;

				constants.uEnvProbeBoxMin = { probe.capturePos.x - h, probe.capturePos.y - h, probe.capturePos.z - h, 0.0f };
				constants.uEnvProbeBoxMax = { probe.capturePos.x + h, probe.capturePos.y + h, probe.capturePos.z + h, 0.0f };
			}

			ctx.commandList.BindInputLayout(batch.mesh->layoutInstanced);
			ctx.commandList.BindVertexBuffer(0, batch.mesh->vertexBuffer, batch.mesh->vertexStrideBytes, 0);
			ctx.commandList.BindVertexBuffer(1, instanceBuffer_, instStride, batch.instanceOffset * instStride);
			ctx.commandList.BindIndexBuffer(batch.mesh->indexBuffer, batch.mesh->indexType, 0);
			ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &constants, 1 }));
			ctx.commandList.DrawIndexed(batch.mesh->indexCount, batch.mesh->indexType, 0, 0, batch.instanceCount, 0);
		}

		++mirrorIndex;
	}

	// Restore state for the following passes (transparent / imgui).
	ctx.commandList.SetState(doDepthPrepass ? mainAfterPreDepthState_ : state_);
	ctx.commandList.SetStencilRef(0);
}