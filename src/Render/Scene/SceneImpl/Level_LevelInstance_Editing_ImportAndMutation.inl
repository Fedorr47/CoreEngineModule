// Create a new node and (optionally) spawn a DrawItem.
// Returns the new node index.
int AddNode(LevelAsset& asset,
	Scene& scene,
	AssetManager& assets,
	std::string_view meshId,
	std::string_view materialId,
	int parentNodeIndex,
	const Transform& localTransform,
	std::string_view name = {})
{
	LevelNode node;
	node.name = std::string(name);
	node.parent = parentNodeIndex;
	node.visible = true;
	node.alive = true;
	node.transform = localTransform;
	node.mesh = std::string(meshId);
	node.material = std::string(materialId);

	asset.nodes.push_back(std::move(node));

	if (nodeToDraw_.size() < asset.nodes.size())
	{
		nodeToDraw_.resize(asset.nodes.size(), -1);
	}
	if (nodeToDraws_.size() < asset.nodes.size())
	{
		nodeToDraws_.resize(asset.nodes.size());
	}
	if (world_.size() < asset.nodes.size())
	{
		world_.resize(asset.nodes.size(), mathUtils::Mat4(1.0f));
	}
	if (nodeToEntity_.size() < asset.nodes.size())
	{
		nodeToEntity_.resize(asset.nodes.size(), kNullEntity);
	}

	const int newIndex = static_cast<int>(asset.nodes.size() - 1);

	EnsureDrawForNode_(asset, scene, assets, newIndex);
	EnsureEntityForNode_(asset, newIndex);
	SyncEntityRenderableForNode_(asset, scene, newIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
	transformsDirty_ = true;
	return newIndex;
}

void EnsureImportedTexture(
	LevelAsset& asset,
	AssetManager& assets,
	std::string_view textureId,
	std::string_view relativePath,
	bool srgb,
	bool isNormalMap)
{
	if (textureId.empty() || relativePath.empty())
	{
		return;
	}

	LevelTextureDef td{};
	td.kind = LevelTextureKind::Tex2D;
	td.props.dimension = TextureDimension::Tex2D;
	td.props.filePath = std::string(relativePath);
	td.props.srgb = srgb;
	td.props.generateMips = true;
	td.props.flipY = false;
	td.props.isNormalMap = isNormalMap;

	auto& dst = asset.textures[std::string(textureId)];
	dst = std::move(td);

	assets.LoadTextureAsync(textureId, dst.props);
}

void BindImportedTexture(
	LevelAsset& asset,
	AssetManager& assets,
	LevelMaterialDef& md,
	std::string_view modelId,
	std::uint32_t materialIndex,
	std::string_view slotName,
	const std::optional<ImportedMaterialTextureRef>& texRef,
	bool srgb)
{
	if (!texRef || texRef->path.empty())
	{
		return;
	}

	const std::string normalizedSlot = (slotName == "metallic") ? "metalness" : std::string(slotName);
	const bool isNormalMap = (normalizedSlot == "normal");
	const std::string texId =
		std::string(modelId) + "__mat_" + std::to_string(materialIndex) + "__" + normalizedSlot;

	EnsureImportedTexture(asset, assets, texId, texRef->path, srgb, isNormalMap);
	md.textureBindings[normalizedSlot] = texId;
}

std::string ImportSkinnedMaterials(
	LevelAsset& asset,
	Scene& scene,
	AssetManager& assets,
	std::string_view skinnedId,
	std::string_view sourcePath,
	bool flipUVs,
	bool cleanupExistingImportedArtifacts = true)
{
	const std::string assetPrefix = std::string(skinnedId) + "__";
	if (cleanupExistingImportedArtifacts)
	{
		for (auto it = asset.materials.begin(); it != asset.materials.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? asset.materials.erase(it) : std::next(it);
		}
		for (auto it = asset.textures.begin(); it != asset.textures.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? asset.textures.erase(it) : std::next(it);
		}
		for (auto it = materialHandles_.begin(); it != materialHandles_.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? materialHandles_.erase(it) : std::next(it);
		}
		pendingBindings_.erase(
			std::remove_if(
				pendingBindings_.begin(),
				pendingBindings_.end(),
				[&](const PendingMaterialBinding& pb)
				{
					return pb.textureId.rfind(assetPrefix, 0) == 0;
				}),
			pendingBindings_.end());
	}

	const ImportedModelScene imported = LoadAssimpScene(std::string(sourcePath), flipUVs, false, true);
	std::string defaultMaterialId;
	for (std::size_t i = 0; i < imported.materials.size(); ++i)
	{
		const ImportedMaterialInfo& srcMat = imported.materials[i];
		const std::string matId = std::string(skinnedId) + "__mat_" + std::to_string(i);

		LevelMaterialDef md{};
		md.material.params.baseColor = mathUtils::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		md.material.permFlags |= MaterialPerm::Skinning;
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "albedo", srcMat.baseColor, true);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "normal", srcMat.normal, false);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "metallic", srcMat.metallic, false);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "roughness", srcMat.roughness, false);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "ao", srcMat.ao, false);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "emissive", srcMat.emissive, true);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "specular", srcMat.specular, false);
		BindImportedTexture(asset, assets, md, skinnedId, static_cast<std::uint32_t>(i), "gloss", srcMat.gloss, false);
		if (!md.textureBindings.empty())
		{
			md.material.permFlags |= MaterialPerm::UseTex;
		}

		asset.materials[matId] = std::move(md);
		[[maybe_unused]] const MaterialHandle runtimeMat = EnsureMaterial(asset, scene, matId);
		if (defaultMaterialId.empty())
		{
			defaultMaterialId = matId;
		}
	}

	return defaultMaterialId;
}

// Import an FBX/Assimp scene as regular Level nodes + mesh defs.
// Each imported Assimp mesh becomes a dedicated Level mesh entry that points to the same source file
// with a submeshIndex override, so saved JSON remains editable and runtime stays mesh-based.
int ImportModelSceneAsNodes(LevelAsset& asset,
	Scene& scene,
	AssetManager& assets,
	std::string_view modelId,
	int parentNodeIndex = -1,
	bool createMaterialPlaceholders = true,
	bool importSkeletonNodes = false,
	bool cleanupExistingImportedArtifacts = true)
{
	namespace fs = std::filesystem;
	auto modelIt = asset.models.find(std::string(modelId));
	if (modelIt == asset.models.end())
	{
		throw std::runtime_error("Level: unknown modelId for scene import: " + std::string(modelId));
	}

	auto MakeAssetRelativePath = [](const fs::path& absolutePath) -> std::string
		{
			const fs::path assetRoot = corefs::FindAssetRoot();
			std::error_code ec;
			const fs::path rel = fs::relative(absolutePath, assetRoot, ec);
			if (!ec && !rel.empty())
			{
				return rel.generic_string();
			}
			return absolutePath.generic_string();
		};

	if (!modelIt->second.path.empty())
	{
		fs::path modelPath(modelIt->second.path);
		if (!modelPath.is_absolute())
		{
			modelPath = corefs::ResolveAsset(modelPath);
		}
		modelIt->second.path = MakeAssetRelativePath(modelPath);
	}

	if (cleanupExistingImportedArtifacts)
	{
		const std::string assetPrefix = std::string(modelId) + "__";
		for (auto it = asset.meshes.begin(); it != asset.meshes.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? asset.meshes.erase(it) : std::next(it);
		}
		for (auto it = asset.materials.begin(); it != asset.materials.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? asset.materials.erase(it) : std::next(it);
		}
		for (auto it = asset.textures.begin(); it != asset.textures.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? asset.textures.erase(it) : std::next(it);
		}

		for (auto it = materialHandles_.begin(); it != materialHandles_.end();)
		{
			it = (it->first.rfind(assetPrefix, 0) == 0) ? materialHandles_.erase(it) : std::next(it);
		}
		pendingBindings_.erase(
			std::remove_if(
				pendingBindings_.begin(),
				pendingBindings_.end(),
				[&](const PendingMaterialBinding& pb)
				{
					return pb.textureId.rfind(assetPrefix, 0) == 0;
				}),
			pendingBindings_.end());
	}

	const ImportedModelScene imported = LoadAssimpScene(modelIt->second.path, modelIt->second.flipUVs, importSkeletonNodes, true);
	if (imported.nodes.empty())
	{
		return -1;
	}


	if (createMaterialPlaceholders)
	{
		for (std::size_t i = 0; i < imported.materials.size(); ++i)
		{
			const ImportedMaterialInfo& srcMat = imported.materials[i];
			const std::string matId = std::string(modelId) + "__mat_" + std::to_string(i);

			LevelMaterialDef md{};
			md.material.params.baseColor = mathUtils::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "albedo", srcMat.baseColor, true);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "normal", srcMat.normal, false);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "metallic", srcMat.metallic, false);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "roughness", srcMat.roughness, false);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "ao", srcMat.ao, false);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "emissive", srcMat.emissive, true);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "specular", srcMat.specular, false);
			BindImportedTexture(asset, assets, md, modelId, static_cast<std::uint32_t>(i), "gloss", srcMat.gloss, false);
			if (!md.textureBindings.empty())
			{
				md.material.permFlags |= MaterialPerm::UseTex;
			}

			asset.materials[matId] = std::move(md);
			[[maybe_unused]] const MaterialHandle runtimeMat = EnsureMaterial(asset, scene, matId);
		}
	}

	std::vector<int> importedNodeMap(imported.nodes.size(), -1);
	int firstImportedNode = -1;

	for (std::size_t i = 0; i < imported.nodes.size(); ++i)
	{
		const ImportedSceneNode& srcNode = imported.nodes[i];
		int runtimeParent = parentNodeIndex;
		if (srcNode.parent >= 0 && static_cast<std::size_t>(srcNode.parent) < importedNodeMap.size())
		{
			runtimeParent = importedNodeMap[static_cast<std::size_t>(srcNode.parent)];
		}

		const int containerNode = AddNode(asset, scene, assets, "", "", runtimeParent, srcNode.localTransform, srcNode.name);
		importedNodeMap[i] = containerNode;
		if (firstImportedNode < 0)
		{
			firstImportedNode = containerNode;
		}

		for (const std::uint32_t submeshIndex : srcNode.submeshes)
		{
			const std::string meshId = std::string(modelId) + "__mesh_" + std::to_string(submeshIndex);
			LevelMeshDef meshDef{};
			meshDef.path = modelIt->second.path;
			meshDef.debugName = modelIt->second.debugName.empty()
				? (std::string(modelId) + "_mesh_" + std::to_string(submeshIndex))
				: (modelIt->second.debugName + "_mesh_" + std::to_string(submeshIndex));
			meshDef.flipUVs = modelIt->second.flipUVs;
			meshDef.submeshIndex = submeshIndex;
			meshDef.bakeNodeTransforms = false;
			asset.meshes[meshId] = std::move(meshDef);

			std::string materialId;
			if (static_cast<std::size_t>(submeshIndex) < imported.submeshes.size())
			{
				const std::uint32_t materialIndex = imported.submeshes[static_cast<std::size_t>(submeshIndex)].materialIndex;
				materialId = std::string(modelId) + "__mat_" + std::to_string(materialIndex);
			}
			AddNode(asset, scene, assets, meshId, materialId, containerNode, Transform{}, std::string(srcNode.name) + "_mesh_" + std::to_string(submeshIndex));
		}
	}

	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
	transformsDirty_ = true;
	return firstImportedNode;
}

// Delete selected node and all its children. (tombstone - keeps indices stable)
void DeleteSubtree(LevelAsset& asset, Scene& scene, int rootNodeIndex)
{
	if (!IsNodeAlive(asset, rootNodeIndex))
	{
		return;
	}

	const std::vector<int> toDelete = CollectSubtree_(asset, rootNodeIndex);
	for (int idx : toDelete)
	{
		if (!IsNodeAlive(asset, idx))
		{
			continue;
		}
		LevelNode& n = asset.nodes[static_cast<std::size_t>(idx)];
		n.alive = false;
		n.visible = false;
		DestroyDrawForNode_(scene, idx);
		DestroySkinnedDrawForNode_(scene, idx);
		DestroyEntityForNode_(idx);
	}

	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
	transformsDirty_ = true;
}

void SetNodeVisible(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, bool visible)
{
	if (!IsNodeAlive(asset, nodeIndex))
		return;

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	n.visible = visible;

	EnsureEntityForNode_(asset, nodeIndex);
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeMesh(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::string_view meshId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	n.mesh = std::string(meshId);
	n.model.clear();
	n.skinnedMesh.clear();
	n.animation.clear();
	n.animationController.clear();
	n.animationClip.clear();
	n.animationRootMotionBone.clear();
	n.animationAutoplay = true;
	n.animationLoop = true;
	n.animationPlayRate = 1.0f;
	n.materialOverrides.clear();

	EnsureEntityForNode_(asset, nodeIndex);
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeModel(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::string_view modelId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	n.model = std::string(modelId);
	n.mesh.clear();
	n.skinnedMesh.clear();
	n.animation.clear();
	n.animationController.clear();
	n.animationClip.clear();
	n.animationRootMotionBone.clear();
	n.animationAutoplay = true;
	n.animationLoop = true;
	n.animationPlayRate = 1.0f;

	EnsureEntityForNode_(asset, nodeIndex);
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeMaterialOverride(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::uint32_t submeshIndex, std::string_view materialId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	if (materialId.empty())
	{
		n.materialOverrides.erase(submeshIndex);
	}
	else
	{
		n.materialOverrides[submeshIndex] = std::string(materialId);
	}

	EnsureEntityForNode_(asset, nodeIndex);
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}


void SetNodeSkinnedMesh(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::string_view skinnedMeshId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	if (n.skinnedMesh != skinnedMeshId)
	{
		n.animation.clear();
		n.animationController.clear();
		n.animationClip.clear();
		n.animationRootMotionBone.clear();
	}
	if (skinnedMeshId.empty())
	{
		n.animation.clear();
		n.animationController.clear();
		n.animationClip.clear();
		n.animationRootMotionBone.clear();
		n.animationAutoplay = true;
		n.animationLoop = true;
		n.animationPlayRate = 1.0f;
	}
	else
	{
		n.animationPlayRate = std::max(0.0f, n.animationPlayRate);
		if (n.material.empty())
		{
			auto it = asset.skinnedMeshes.find(std::string(skinnedMeshId));
			if (it != asset.skinnedMeshes.end())
			{
				const std::string defaultMaterialId = ImportSkinnedMaterials(
					asset,
					scene,
					assets,
					it->first,
					it->second.path,
					it->second.flipUVs,
					false);
				if (!defaultMaterialId.empty())
				{
					n.material = defaultMaterialId;
				}
			}
		}
	}

	n.skinnedMesh = std::string(skinnedMeshId);
	n.mesh.clear();
	n.model.clear();
	n.materialOverrides.clear();

	EnsureEntityForNode_(asset, nodeIndex);
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeAnimationAsset(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::string_view animationId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	if (n.skinnedMesh.empty() && !animationId.empty())
	{
		return;
	}
	if (n.animation == animationId)
	{
		return;
	}

	n.animation = std::string(animationId);
	n.animationClip.clear();
	n.animationAutoplay = true;
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeAnimationController(LevelAsset& asset, Scene& scene, AssetManager& assets, int nodeIndex, std::string_view controllerId)
{
	if (!IsNodeAlive(asset, nodeIndex))
	{
		return;
	}

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	if (n.skinnedMesh.empty() && !controllerId.empty())
	{
		return;
	}
	if (n.animationController == controllerId)
	{
		return;
	}

	n.animationController = std::string(controllerId);
	if (!controllerId.empty())
	{
		n.animationClip.clear();
		n.animationAutoplay = true;
	}
	EnsureDrawForNode_(asset, scene, assets, nodeIndex);
	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

void SetNodeMaterial(LevelAsset& asset, Scene& scene, int nodeIndex, std::string_view materialId)
{
	if (!IsNodeAlive(asset, nodeIndex))
		return;

	LevelNode& n = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	n.material = std::string(materialId);
	if (!n.model.empty())
	{
		n.materialOverrides.clear();
	}

	EnsureEntityForNode_(asset, nodeIndex);

	const auto& drawIndices = GetNodeDrawIndices(nodeIndex);
	for (const int di : drawIndices)
	{
		if (di >= 0 && static_cast<std::size_t>(di) < scene.drawItems.size())
		{
			scene.drawItems[static_cast<std::size_t>(di)].material = EnsureMaterial(asset, scene, materialId);
		}
	}

	SyncEntityRenderableForNode_(asset, scene, nodeIndex);
	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
}

