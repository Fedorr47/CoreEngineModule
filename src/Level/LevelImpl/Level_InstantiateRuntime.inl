LevelInstance InstantiateLevel(Scene& scene, AssetManager& assets, BindlessTable&, const LevelAsset& asset, const mathUtils::Mat4& root)
{
	LevelInstance inst;
	inst.root_ = root;

	// Camera
	if (asset.camera)
	{
		scene.camera = *asset.camera;
	}

	// Lights
	for (const auto& l : asset.lights)
	{
		scene.AddLight(l);
	}

	// Particle emitters
	inst.RebuildParticleEmitters(scene, asset);

	auto IsTextureUsedAsNormalMap = [&](std::string_view textureId) -> bool
	{
		for (const auto& [_, md] : asset.materials)
		{
			for (const auto& [slot, boundTextureId] : md.textureBindings)
			{
				if (boundTextureId == textureId && slot == "normal")
				{
					return true;
				}
			}
		}
		return false;
	};

	inst.textureHandles_.reserve(asset.textures.size());
	// Textures: request loads (descriptor indices are resolved later)
	for (const auto& [textureId, textureDefinition] : asset.textures)
	{
		TextureProperties properties = textureDefinition.props;
		properties.isNormalMap = properties.isNormalMap || IsTextureUsedAsNormalMap(textureId);

		const std::string resourceKey = LevelInstance::MakeLevelResourceKey_(asset, "texture", textureId);
		std::shared_ptr<TextureResource> textureHandle;
		
		if (textureDefinition.kind == LevelTextureKind::Tex2D)
		{
			textureHandle = assets.LoadTextureAsync(resourceKey, std::move(properties));
		}
		else if (textureDefinition.cubeSource == LevelCubeSource::Cross)
		{
			properties.cubeFromCross = true;

			textureHandle =
				assets.LoadTextureAsync(
					resourceKey,
					std::move(properties));
		}
		else if (textureDefinition.cubeSource == LevelCubeSource::AutoFaces)
		{
			if (!textureDefinition.preferBase.empty())
			{
				textureHandle =
					assets.LoadTextureCubeAsync(
						resourceKey,
						textureDefinition.baseOrDir,
						textureDefinition.preferBase,
						std::move(properties));
			}
			else
			{
				textureHandle =
					assets.LoadTextureCubeAsync(
						resourceKey,
						textureDefinition.baseOrDir,
						std::move(properties));
			}
		}
		else
		{
			textureHandle =
				assets.LoadTextureCubeAsync(
					resourceKey,
					textureDefinition.facePaths,
					std::move(properties));
		}

		inst.textureHandles_.emplace(textureId,std::move(textureHandle));
	}

	// Meshes: request loads
	std::unordered_map<std::string, MeshHandle> meshHandles;
	meshHandles.reserve(asset.meshes.size());

	for (const auto& [meshId, meshDefinition] : asset.meshes)
	{
		MeshProperties properties{};
		properties.filePath = meshDefinition.path;
		properties.debugName = meshDefinition.debugName;
		properties.flipUVs = meshDefinition.flipUVs;
		properties.submeshIndex = meshDefinition.submeshIndex;
		properties.bakeNodeTransforms =
			meshDefinition.bakeNodeTransforms;

		const std::string resourceKey = LevelInstance::MakeLevelResourceKey_(asset, "mesh", meshId);

		meshHandles.emplace(
			meshId,
			assets.LoadMeshAsync(resourceKey, std::move(properties)));
	}

	// Materials: create in Scene and collect pending texture bindings
	std::unordered_map<std::string, MaterialHandle> materialHandles;
	materialHandles.reserve(asset.materials.size());
	for (const auto& [id, md] : asset.materials)
	{
		MaterialHandle h = scene.CreateMaterial(md.material);
		materialHandles.emplace(id, h);

		for (const auto& [slot, texId] : md.textureBindings)
		{
			PendingMaterialBinding pb;
			pb.material = h;
			pb.textureId = texId;

			if (slot == "albedo") pb.slot = MaterialTextureSlot::Albedo;
			else if (slot == "normal") pb.slot = MaterialTextureSlot::Normal;
			else if (slot == "metalness" || slot == "metallic") pb.slot = MaterialTextureSlot::Metalness;
			else if (slot == "roughness") pb.slot = MaterialTextureSlot::Roughness;
			else if (slot == "ao") pb.slot = MaterialTextureSlot::AO;
			else if (slot == "emissive") pb.slot = MaterialTextureSlot::Emissive;
			else if (slot == "specular" || slot == "spec") pb.slot = MaterialTextureSlot::Specular;
			else if (slot == "gloss" || slot == "glossiness") pb.slot = MaterialTextureSlot::Gloss;
			else if (slot == "height" || slot == "heightMap" || slot == "displacement" || slot == "disp") pb.slot = MaterialTextureSlot::Height;
			else
			{
				throw std::runtime_error("Level JSON: unknown material texture slot: " + slot);
			}

			inst.pendingBindings_.push_back(std::move(pb));
		}
	}

	inst.materialHandles_ = materialHandles;

	inst.skyboxTextureId_ = asset.skyboxTexture;
	
	for (std::size_t nodeIdx = 0; nodeIdx < asset.nodes.size(); ++nodeIdx)
	{
		const LevelNode& lNode = asset.nodes[nodeIdx];
		if (lNode.parent < 0)
		{
			continue;
		}
		
		const size_t parentIdx = static_cast<std::size_t>(lNode.parent);
		if (parentIdx >= asset.nodes.size())
		{
			throw std::runtime_error("Level instantiate: node index " 
				+ std::to_string(nodeIdx) 
				+ " references unknown parent node index " 
				+ std::to_string(lNode.parent));
		}
	}

	// Nodes: create draw items
	inst.nodeToDraw_.assign(asset.nodes.size(), -1);
	inst.nodeToDraws_.assign(asset.nodes.size(), {});
	inst.drawToNode_.clear();
	inst.drawToNode_.reserve(asset.nodes.size());
	inst.nodeToSkinnedDraw_.assign(asset.nodes.size(), -1);
	inst.skinnedDrawToNode_.clear();
	inst.skinnedDrawToNode_.reserve(asset.nodes.size());

	// Compute world matrices (handles arbitrary parent order)
	inst.transformsDirty_ = true;
	inst.RecomputeWorld_(asset);
	inst.transformsDirty_ = false;

	inst.staticMeshSources_.reserve(asset.nodes.size());
	for (std::size_t i = 0; i < asset.nodes.size(); ++i)
	{
		const LevelNode& n = asset.nodes[i];
		const LevelNode& node = asset.nodes[i];
		if (!IsStaticNavigationGeometry(node) || node.mesh.empty() || !node.skinnedMesh.empty())
		{
			continue;
		}

		const auto meshIt = meshHandles.find(node.mesh);
		if (meshIt != meshHandles.end())
		{
			inst.staticMeshSources_.push_back(LevelStaticMeshSource{ meshIt->second, inst.world_[i], static_cast<int>(i) });
		}
	}
	for (std::size_t i = 0; i < asset.nodes.size(); ++i)
	{
		const LevelNode& n = asset.nodes[i];
		if (!n.alive)
		{
			continue;
		}
		if (n.mesh.empty() && n.model.empty() && n.skinnedMesh.empty())
		{
			continue;
		}

		if (!n.skinnedMesh.empty())
		{
			if (!n.visible)
			{
				continue;
			}
			const int skinnedDrawIndex = inst.MakeSkinnedDrawForNode_(asset, scene, static_cast<int>(i), n);
			inst.nodeToSkinnedDraw_[i] = skinnedDrawIndex;
			continue;
		}

		if (!n.model.empty())
		{
			const bool includeInNavigation = IsStaticNavigationGeometry(n);
			auto modelIt = asset.models.find(n.model);
			if (modelIt == asset.models.end())
			{
				throw std::runtime_error("Level JSON: node references unknown modelId: " + n.model);
			}
			const ImportedModelScene meta = LoadAssimpScene(modelIt->second.path, modelIt->second.flipUVs);
			for (const ImportedSubmeshInfo& sub : meta.submeshes)
			{
				MeshProperties properties{};
				properties.filePath = modelIt->second.path;
				properties.debugName = modelIt->second.debugName.empty() ? sub.name : (modelIt->second.debugName + "_" + sub.name);
				properties.flipUVs = modelIt->second.flipUVs;
				properties.submeshIndex = sub.submeshIndex;
				const std::string modelSubmeshId =
					n.model
					+ "#submesh="
					+ std::to_string(sub.submeshIndex);

				const std::string resourceKey =
					LevelInstance::MakeLevelResourceKey_(asset, "model-submesh", modelSubmeshId);
				MeshHandle meshHandle = assets.LoadMeshAsync(resourceKey, std::move(properties));
				if (includeInNavigation)
				{
					inst.staticMeshSources_.push_back(LevelStaticMeshSource{
						meshHandle, inst.world_[i], static_cast<int>(i) });
				}
				if (!n.visible)
				{
					continue;
				}

				std::string materialId = n.material;
				if (auto itOv = n.materialOverrides.find(sub.submeshIndex); itOv != n.materialOverrides.end())
				{
					materialId = itOv->second;
				}
				MaterialHandle mat{};
				if (!materialId.empty())
				{
					auto it = materialHandles.find(materialId);
					if (it == materialHandles.end())
					{
						throw std::runtime_error("Level JSON: node references unknown materialId: " + materialId);
					}
					mat = it->second;
				}
				DrawItem item{};
				item.mesh = meshHandle;
				item.material = mat;
				item.transform.useMatrix = true;
				item.transform.matrix = inst.world_[i];
				const int drawIndex = static_cast<int>(scene.drawItems.size());
				scene.AddDraw(item);
				inst.drawToNode_.push_back(static_cast<int>(i));
				inst.nodeToDraws_[i].push_back(drawIndex);
			}
			inst.nodeToDraw_[i] = inst.nodeToDraws_[i].empty() ? -1 : inst.nodeToDraws_[i].front();
			continue;
		}
		if (!n.visible)
		{
			continue;
		}

		auto meshIt = meshHandles.find(n.mesh);
		if (meshIt == meshHandles.end())
		{
			throw std::runtime_error("Level JSON: node references unknown meshId: " + n.mesh);
		}

		MaterialHandle mat{};
		if (!n.material.empty())
		{
			auto it = materialHandles.find(n.material);
			if (it == materialHandles.end())
			{
				throw std::runtime_error("Level JSON: node references unknown materialId: " + n.material);
			}
			mat = it->second;
		}

		DrawItem item{};
		item.mesh = meshIt->second;
		item.material = mat;
		item.transform.useMatrix = true;
		item.transform.matrix = inst.world_[i];

		const int drawIndex = static_cast<int>(scene.drawItems.size());
		scene.AddDraw(item);
		inst.nodeToDraw_[i] = drawIndex;
		inst.nodeToDraws_[i].push_back(drawIndex);
		inst.drawToNode_.push_back(static_cast<int>(i));
	}

	// ------------------------------------------------------------
	// ECS build (hybrid phase): one entity per LevelNode (alive)
	// ------------------------------------------------------------

	inst.ecs_.Clear();
	inst.nodeToEntity_.assign(asset.nodes.size(), kNullEntity);

	for (std::size_t i = 0; i < asset.nodes.size(); ++i)
	{
		const LevelNode& n = asset.nodes[i];
		if (!n.alive)
		{
			continue;
		}

		const EntityHandle e = inst.ecs_.CreateEntity();
		inst.nodeToEntity_[i] = e;

		inst.ecs_.EmplaceNodeData(e, static_cast<int>(i), n.parent, n.transform, inst.world_[i], Flags{ .alive = n.alive, .visible = n.visible });

		// Renderable is optional (node can be non-renderable)
		const int drawIndex = inst.nodeToDraw_[i];
		const int skinnedDrawIndex = inst.nodeToSkinnedDraw_[i];
		if (drawIndex >= 0)
		{
			// We can grab handles from scene.drawItems since they were created above
			const DrawItem& di = scene.drawItems[static_cast<std::size_t>(drawIndex)];
			inst.ecs_.EmplaceRenderable(e, Renderable{ .mesh = di.mesh, .material = di.material, .drawIndex = drawIndex, .skinnedDrawIndex = -1, .isSkinned = false });
		}
		else if (skinnedDrawIndex >= 0)
		{
			const SkinnedDrawItem& sdi = scene.skinnedDrawItems[static_cast<std::size_t>(skinnedDrawIndex)];
			inst.ecs_.EmplaceRenderable(e, Renderable{ .mesh = {}, .material = sdi.material, .drawIndex = -1, .skinnedDrawIndex = skinnedDrawIndex, .isSkinned = true });
		}
	}

	inst.SyncEditorRuntimeBindings(asset, scene);
	inst.ValidateRuntimeMappingsDebug(asset, scene);
	return inst;
}