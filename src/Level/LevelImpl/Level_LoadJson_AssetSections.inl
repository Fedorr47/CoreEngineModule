inline void ParseMeshSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- meshes ---
	if (auto* meshesV = TryGet(jsonObject, "meshes"))
	{
		const JsonObject& meshesO = meshesV->AsObject();
		for (const auto& [id, defV] : meshesO)
		{
			const JsonObject& md = defV.AsObject();
			LevelMeshDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			def.bakeNodeTransforms = GetBoolOpt(md, "bakeNodeTransforms", true);
			if (auto* submeshV = TryGet(md, "submeshIndex"))
			{
				if (!submeshV->IsNumber())
				{
					throw std::runtime_error("Level JSON: meshes." + id + ".submeshIndex must be number");
				}
				def.submeshIndex = static_cast<std::uint32_t>(submeshV->AsNumber());
			}
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: meshes." + id + ".path is required");
			}
			out.meshes.emplace(id, std::move(def));
		}
	}

}

inline void ParseModelSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- models ---
	if (auto* modelsV = TryGet(jsonObject, "models"))
	{
		const JsonObject& modelsO = modelsV->AsObject();
		for (const auto& [id, defV] : modelsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelModelDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: models." + id + ".path is required");
			}
			out.models.emplace(id, std::move(def));
		}
	}

}

inline void ParseTextureSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- textures ---
	if (auto* texV = TryGet(jsonObject, "textures"))
	{
		const JsonObject& texO = texV->AsObject();
		for (const auto& [id, defV] : texO)
		{
			const JsonObject& td = defV.AsObject();
			LevelTextureDef def;

			const std::string kind = GetStringOpt(td, "kind", "tex2d");
			if (kind == "tex2d")
			{
				def.kind = LevelTextureKind::Tex2D;
				def.props.dimension = TextureDimension::Tex2D;
				def.props.filePath = GetStringOpt(td, "path");
				if (def.props.filePath.empty())
				{
					throw std::runtime_error("Level JSON: textures." + id + ".path is required for tex2d");
				}
			}
			else if (kind == "cube")
			{
				def.kind = LevelTextureKind::Cube;
				def.props.dimension = TextureDimension::Cube;

				const std::string source = GetStringOpt(td, "source", "cross");
				if (source == "cross")
				{
					def.cubeSource = LevelCubeSource::Cross;
					def.props.cubeFromCross = true;
					def.props.filePath = GetStringOpt(td, "cross");
					if (def.props.filePath.empty())
					{
						throw std::runtime_error("Level JSON: textures." + id + ".cross is required for cube/cross");
					}
				}
				else if (source == "auto")
				{
					def.cubeSource = LevelCubeSource::AutoFaces;
					def.baseOrDir = GetStringOpt(td, "baseOrDir");
					def.preferBase = GetStringOpt(td, "preferBase");
					if (def.baseOrDir.empty())
					{
						throw std::runtime_error("Level JSON: textures." + id + ".baseOrDir is required for cube/auto");
					}
				}
				else if (source == "faces")
				{
					def.cubeSource = LevelCubeSource::Faces;
					const JsonObject& facesO = GetReq(td, "faces").AsObject();
					def.facePaths[0] = GetStringOpt(facesO, "px");
					def.facePaths[1] = GetStringOpt(facesO, "nx");
					def.facePaths[2] = GetStringOpt(facesO, "py");
					def.facePaths[3] = GetStringOpt(facesO, "ny");
					def.facePaths[4] = GetStringOpt(facesO, "pz");
					def.facePaths[5] = GetStringOpt(facesO, "nz");

					for (const auto& p : def.facePaths)
					{
						if (p.empty())
						{
							throw std::runtime_error("Level JSON: textures." + id + ".faces must define px/nx/py/ny/pz/nz");
						}
					}
				}
				else
				{
					throw std::runtime_error("Level JSON: textures." + id + ".source must be cross|auto|faces");
				}
			}
			else
			{
				throw std::runtime_error("Level JSON: textures." + id + ".kind must be tex2d|cube");
			}

			// Common props
			def.props.srgb = GetBoolOpt(td, "srgb", true);
			def.props.generateMips = GetBoolOpt(td, "mips", true);
			def.props.flipY = GetBoolOpt(td, "flipY", false);
			def.props.isNormalMap = GetBoolOpt(td, "normalMap", GetBoolOpt(td, "isNormalMap", false));

			out.textures.emplace(id, std::move(def));
		}
	}

}

inline void ParseAnimationSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- animations ---
	if (auto* animationsV = TryGet(jsonObject, "animations"))
	{
		const JsonObject& animationsO = animationsV->AsObject();
		for (const auto& [id, defV] : animationsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelAnimationDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: animations." + id + ".path is required");
			}
			out.animations.emplace(id, std::move(def));
		}
	}

}

inline void ParseExternalAnimationControllerAssetSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- external animationControllerAssets ---
	if (auto* controllerAssetsV = TryGet(jsonObject, "animationControllerAssets"))
	{
		const JsonObject& controllerAssetsO = controllerAssetsV->AsObject();
		for (const auto& [id, defV] : controllerAssetsO)
		{
			const JsonObject& ad = defV.AsObject();
			const std::string path = GetStringOpt(ad, "path");
			if (path.empty())
			{
				throw std::runtime_error("Level JSON: animationControllerAssets." + id + ".path is required");
			}
			out.animationControllerAssetPaths[id] = path;
			out.animationControllers.insert_or_assign(id, LoadExternalAnimationControllerAssetFromJson_(path, id));
		}
	}

}

inline void ParseAnimationControllerSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- animationControllers ---
	if (auto* controllersV = TryGet(jsonObject, "animationControllers"))
	{
		const JsonObject& controllersO = controllersV->AsObject();
		for (const auto& [id, defV] : controllersO)
		{
			const JsonObject& cd = defV.AsObject();
			out.animationControllers.insert_or_assign(
				id,
				ParseAnimationControllerAssetObject_(cd, id, "Level JSON: animationControllers." + id));
		}
	}

}

inline void ParseSkinnedMeshSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- skinnedMeshes ---
	if (auto* skinnedV = TryGet(jsonObject, "skinnedMeshes"))
	{
		const JsonObject& skinnedO = skinnedV->AsObject();
		for (const auto& [id, defV] : skinnedO)
		{
			const JsonObject& md = defV.AsObject();
			LevelSkinnedMeshDef def;
			def.path = GetStringOpt(md, "path");
			def.debugName = GetStringOpt(md, "debugName");
			def.flipUVs = GetBoolOpt(md, "flipUVs", true);
			if (auto* smi = TryGet(md, "submeshIndex"))
			{
				if (!smi->IsNumber())
				{
					throw std::runtime_error("Level JSON: skinnedMeshes." + id + ".submeshIndex must be number");
				}
				def.submeshIndex = static_cast<std::uint32_t>(smi->AsNumber());
			}
			if (def.path.empty())
			{
				throw std::runtime_error("Level JSON: skinnedMeshes." + id + ".path is required");
			}
			out.skinnedMeshes.emplace(id, std::move(def));
		}
	}


}

inline void ParseMaterialSection_(LevelAsset& out, const JsonObject& jsonObject)
{
	// --- materials ---
	if (auto* matsV = TryGet(jsonObject, "materials"))
	{
		const JsonObject& matsO = matsV->AsObject();
		for (const auto& [id, defV] : matsO)
		{
			const JsonObject& md = defV.AsObject();
			LevelMaterialDef def;

			if (auto* bc = TryGet(md, "baseColor"))
			{
				auto a = ReadFloatArray(*bc, 4, "baseColor");
				def.material.params.baseColor = { a[0], a[1], a[2], a[3] };
			}

			def.material.params.shininess = GetFloatOpt(md, "shininess", def.material.params.shininess);
			def.material.params.specStrength = GetFloatOpt(md, "specStrength", def.material.params.specStrength);
			def.material.params.shadowBias = GetFloatOpt(md, "shadowBias", def.material.params.shadowBias);

			def.material.params.metallic = GetFloatOpt(md, "metallic", def.material.params.metallic);
			def.material.params.roughness = GetFloatOpt(md, "roughness", def.material.params.roughness);
			def.material.params.ao = GetFloatOpt(md, "ao", def.material.params.ao);
			def.material.params.emissiveStrength = GetFloatOpt(md, "emissiveStrength", def.material.params.emissiveStrength);
			def.material.params.heightScale = GetFloatOpt(md, "heightScale", def.material.params.heightScale);

			if (auto* flagsV = TryGet(md, "flags"))
			{
				def.material.permFlags = ParsePermFlags(*flagsV);
			}

			if (auto* envV = TryGet(md, "envSource"))
			{
				def.material.envSource = ParseEnvSourceOrThrow(*envV, id);
			}
			else if (auto* envV2 = TryGet(md, "env"))
			{
				def.material.envSource = ParseEnvSourceOrThrow(*envV2, id);
			}

			if (auto* texBindV = TryGet(md, "textures"))
			{
				const JsonObject& tbo = texBindV->AsObject();
				for (const auto& [slot, tv] : tbo)
				{
					if (!tv.IsString())
					{
						throw std::runtime_error("Level JSON: materials." + id + ".textures values must be strings");
					}
					def.textureBindings.emplace(slot, tv.AsString());
				}
			}

			out.materials.emplace(id, std::move(def));
		}
	}

}
