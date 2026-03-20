constexpr std::uint32_t kMaterialFlagUseTex = 1u << 0;
constexpr std::uint32_t kMaterialFlagUseShadow = 1u << 1;
constexpr std::uint32_t kMaterialFlagUseNormal = 1u << 2;
constexpr std::uint32_t kMaterialFlagUseMetalTex = 1u << 3;
constexpr std::uint32_t kMaterialFlagUseRoughTex = 1u << 4;
constexpr std::uint32_t kMaterialFlagUseAOTex = 1u << 5;
constexpr std::uint32_t kMaterialFlagUseEmissiveTex = 1u << 6;
constexpr std::uint32_t kMaterialFlagUseEnv = 1u << 7;
constexpr std::uint32_t kMaterialFlagEnvForceMip0 = 1u << 8;
constexpr std::uint32_t kMaterialFlagEnvFlipZ = 1u << 9;
constexpr std::uint32_t kMaterialFlagUseSpecularTex = 1u << 10;
constexpr std::uint32_t kMaterialFlagUseGlossTex = 1u << 11;
constexpr std::uint32_t kMaterialFlagUseHeightTex = 1u << 12;

auto ResolveMainPassMaterialPerm = [&](const auto& material, const auto& materialHandle) -> MaterialPerm
	{
		MaterialPerm perm = MaterialPerm::UseShadow;
		if (materialHandle.id != 0)
		{
			perm = EffectivePerm(scene.GetMaterial(materialHandle));
		}
		else if (material.albedoDescIndex != 0)
		{
			perm = perm | MaterialPerm::UseTex;
		}
		return perm;
	};

auto ResolveOpaqueEnvBinding = [&](const auto& materialHandle, int reflectionProbeIndex) -> ResolvedMaterialEnvBinding
	{
		ResolvedMaterialEnvBinding env{};
		env.descIndex = scene.skyboxDescIndex;

		if (materialHandle.id == 0)
		{
			return env;
		}

		const auto& mat = scene.GetMaterial(materialHandle);
		if (mat.envSource != EnvSource::ReflectionCapture || !settings_.enableReflectionCapture)
		{
			return env;
		}

		if (reflectionProbeIndex < 0 ||
			static_cast<std::size_t>(reflectionProbeIndex) >= reflectionProbes_.size())
		{
			return env;
		}

		const auto& probe = reflectionProbes_[static_cast<std::size_t>(reflectionProbeIndex)];
		if (probe.cubeDescIndex == 0 || !probe.prefilteredCube)
		{
			return env;
		}

		env.descIndex = probe.cubeDescIndex;
		env.arrayTexture = probe.prefilteredCube;
		env.usingReflectionProbeEnv = true;
		return env;
	};

auto ResolveTransparentEnvBinding = [&](const auto& materialHandle) -> ResolvedMaterialEnvBinding
	{
		ResolvedMaterialEnvBinding env{};
		env.descIndex = scene.skyboxDescIndex;

		if (materialHandle.id == 0)
		{
			return env;
		}

		const auto& mat = scene.GetMaterial(materialHandle);
		if (mat.envSource != EnvSource::ReflectionCapture || !settings_.enableReflectionCapture)
		{
			return env;
		}

		if (reflectionCubeDescIndex_ == 0 || !reflectionCube_)
		{
			return env;
		}

		env.descIndex = reflectionCubeDescIndex_;
		env.arrayTexture = reflectionCube_;
		env.usingReflectionProbeEnv = true;
		return env;
	};

auto FillMainPassMaterialTextureIndices = [&](auto& constants, const auto& material)
	{
		constants.uTexIndices0 = {
			static_cast<float>(material.albedoDescIndex),
			static_cast<float>(material.normalDescIndex),
			static_cast<float>(material.metalnessDescIndex),
			static_cast<float>(material.roughnessDescIndex)
		};
		constants.uTexIndices1 = {
			static_cast<float>(material.aoDescIndex),
			static_cast<float>(material.emissiveDescIndex),
			static_cast<float>(material.specularDescIndex),
			static_cast<float>(material.glossDescIndex)
		};
		constants.uTexIndices2 = {
			static_cast<float>(material.heightDescIndex),
			0.0f,
			0.0f,
			0.0f
		};
		constants.uParallaxParams = { material.heightScale, 0.0f, 0.0f, 0.0f };
	};

auto BindMainPassMaterialTextures = [&](auto& commandList, const auto& material, const ResolvedMaterialEnvBinding& env)
	{
		commandList.BindTextureDesc(0, material.albedoDescIndex);
		commandList.BindTextureDesc(12, material.normalDescIndex);
		commandList.BindTextureDesc(13, material.metalnessDescIndex);
		commandList.BindTextureDesc(14, material.roughnessDescIndex);
		commandList.BindTextureDesc(15, material.aoDescIndex);
		commandList.BindTextureDesc(16, material.emissiveDescIndex);
		commandList.BindTextureDesc(17, env.descIndex);

		if (env.usingReflectionProbeEnv && env.arrayTexture)
		{
			commandList.BindTexture2DArray(18, env.arrayTexture);
		}
	};

auto BuildMainPassMaterialFlags = [&](const auto& material, bool useTex, bool useShadow, const ResolvedMaterialEnvBinding& env) -> std::uint32_t
	{
		std::uint32_t flags = 0;
		if (useTex)
		{
			flags |= kMaterialFlagUseTex;
		}
		if (useShadow)
		{
			flags |= kMaterialFlagUseShadow;
		}
		if (material.normalDescIndex != 0)
		{
			flags |= kMaterialFlagUseNormal;
		}
		if (material.metalnessDescIndex != 0)
		{
			flags |= kMaterialFlagUseMetalTex;
		}
		if (material.roughnessDescIndex != 0)
		{
			flags |= kMaterialFlagUseRoughTex;
		}
		if (material.aoDescIndex != 0)
		{
			flags |= kMaterialFlagUseAOTex;
		}
		if (material.emissiveDescIndex != 0)
		{
			flags |= kMaterialFlagUseEmissiveTex;
		}
		if (env.descIndex != 0)
		{
			flags |= kMaterialFlagUseEnv;
		}
		if (material.specularDescIndex != 0)
		{
			flags |= kMaterialFlagUseSpecularTex;
		}
		if (material.glossDescIndex != 0)
		{
			flags |= kMaterialFlagUseGlossTex;
		}
		if (material.heightDescIndex != 0 && std::abs(material.heightScale) > 1e-6f)
		{
			flags |= kMaterialFlagUseHeightTex;
		}
		if (settings_.enableReflectionCapture && env.usingReflectionProbeEnv)
		{
			// Reflection probes use the manual cube-as-array sampling path to preserve capture orientation
			// and box-parallax correction, but now rely on a prefiltered mip chain for roughness.
			flags |= kMaterialFlagEnvForceMip0;
			flags |= kMaterialFlagEnvFlipZ;
		}
		return flags;
	};
