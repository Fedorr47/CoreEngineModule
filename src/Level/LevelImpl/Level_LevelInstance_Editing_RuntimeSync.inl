void MarkTransformsDirty() noexcept
{
	transformsDirty_ = true;
}

void SyncEditorRuntimeBindings(const LevelAsset& asset, Scene& scene) const noexcept
{
	auto SanitizeNodeIndex = [&](int& nodeIndex) noexcept
		{
			if (!IsNodeAlive(asset, nodeIndex))
			{
				nodeIndex = -1;
			}
		};

	auto SanitizeParticleEmitterIndex = [&](int& emitterIndex) noexcept
		{
			if (!IsParticleEmitterAlive(asset, emitterIndex))
			{
				emitterIndex = -1;
			}
		};

	auto SanitizeSelectedNodes = [&]() noexcept
		{
			// Remove dead/out-of-range nodes.
			auto& sel = scene.editorSelectedNodes;
			std::size_t write = 0;
			for (std::size_t i = 0; i < sel.size(); ++i)
			{
				const int nodeIndex = sel[i];
				if (IsNodeAlive(asset, nodeIndex))
				{
					sel[write++] = nodeIndex;
				}
			}
			sel.resize(write);

			// Deduplicate (keep order stable).
			for (std::size_t i = 0; i < sel.size(); ++i)
			{
				for (std::size_t j = i + 1; j < sel.size();)
				{
					if (sel[j] == sel[i])
					{
						sel.erase(sel.begin() + static_cast<std::vector<int>::difference_type>(j));
						continue;
					}
					++j;
				}
			}
		};

	SanitizeNodeIndex(scene.editorSelectedNode);
	SanitizeParticleEmitterIndex(scene.editorSelectedParticleEmitter);
	SanitizeNodeIndex(scene.editorReflectionCaptureOwnerNode);
	SanitizeSelectedNodes();

	// Keep primary selection consistent with the selection set.
	if (scene.editorSelectedNode >= 0)
	{
		bool found = false;
		for (const int v : scene.editorSelectedNodes)
		{
			if (v == scene.editorSelectedNode)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			// Someone set primary directly (e.g. via UI). Treat it as single selection.
			scene.editorSelectedNodes.clear();
			scene.editorSelectedNodes.push_back(scene.editorSelectedNode);
		}
	}
	else
	{
		// If primary is invalid but we still have a set, pick a new primary.
		if (!scene.editorSelectedNodes.empty())
		{
			scene.editorSelectedNode = scene.editorSelectedNodes.back();
		}
	}

	if (scene.editorSelectedParticleEmitter >= 0)
	{
		scene.editorSelectedNode = -1;
		scene.editorSelectedNodes.clear();
	}

	scene.editorSelectedDrawItem = GetNodeDrawIndex(scene.editorSelectedNode);
	scene.editorSelectedSkinnedDrawItem = GetNodeSkinnedDrawIndex(scene.editorSelectedNode);

	// Build selected draw item lists.
	scene.editorSelectedDrawItems.clear();
	scene.editorSelectedSkinnedDrawItems.clear();
	for (const int nodeIndex : scene.editorSelectedNodes)
	{
		const auto& drawIndices = GetNodeDrawIndices(nodeIndex);
		for (const int di : drawIndices)
		{
			if (di >= 0)
			{
				scene.editorSelectedDrawItems.push_back(di);
			}
		}

		const int skinnedDrawIndex = GetNodeSkinnedDrawIndex(nodeIndex);
		if (skinnedDrawIndex >= 0)
		{
			scene.editorSelectedSkinnedDrawItems.push_back(skinnedDrawIndex);
		}
	}
	scene.editorReflectionCaptureOwnerDrawItem = GetNodeDrawIndex(scene.editorReflectionCaptureOwnerNode);
}

void ValidateRuntimeMappingsDebug(const LevelAsset& asset, const Scene& scene) const noexcept
{
#ifndef NDEBUG
	ValidateRuntimeMappings_(asset, scene);
#endif
}

// Recompute world transforms (with hierarchy) and push to Scene draw items.
void SyncTransformsIfDirty(const LevelAsset& asset, Scene& scene)
{
	if (!transformsDirty_)
		return;

	RecomputeWorld_(asset);

	// Push to Scene + ECS
	const std::size_t ncount = asset.nodes.size();
	if (nodeToDraw_.size() < ncount)
		nodeToDraw_.resize(ncount, -1);
	if (nodeToDraws_.size() < ncount)
		nodeToDraws_.resize(ncount);
	if (nodeToEntity_.size() < ncount)
		nodeToEntity_.resize(ncount, kNullEntity);
	if (nodeToSkinnedDraw_.size() < ncount)
		nodeToSkinnedDraw_.resize(ncount, -1);

	for (std::size_t i = 0; i < ncount; ++i)
	{
		const LevelNode& n = asset.nodes[i];
		if (!n.alive)
		{
			continue;
		}

		const EntityHandle e = EnsureEntityForNode_(asset, static_cast<int>(i));
		if (e != kNullEntity)
		{
			ecs_.UpsertNodeData(e, static_cast<int>(i), n.parent, n.transform, world_[i], Flags{ .alive = n.alive, .visible = n.visible });
		}

		const auto& drawIndices = (i < nodeToDraws_.size()) ? nodeToDraws_[i] : std::vector<int>{};
		const int skinnedDrawIndex = (i < nodeToSkinnedDraw_.size()) ? nodeToSkinnedDraw_[i] : -1;
		if (drawIndices.empty())
		{
			if (skinnedDrawIndex >= 0 && static_cast<std::size_t>(skinnedDrawIndex) < scene.skinnedDrawItems.size())
			{
				SkinnedDrawItem& item = scene.skinnedDrawItems[static_cast<std::size_t>(skinnedDrawIndex)];
				item.transform.useMatrix = true;
				item.transform.matrix = world_[i];
			}
			SyncEntityRenderableForNode_(asset, scene, static_cast<int>(i));
			continue;
		}

		for (const int di : drawIndices)
		{
			if (di < 0 || static_cast<std::size_t>(di) >= scene.drawItems.size())
			{
				continue;
			}
			DrawItem& item = scene.drawItems[static_cast<std::size_t>(di)];
			item.transform.useMatrix = true;
			item.transform.matrix = world_[i];
		}

		if (skinnedDrawIndex >= 0 && static_cast<std::size_t>(skinnedDrawIndex) < scene.skinnedDrawItems.size())
		{
			SkinnedDrawItem& item = scene.skinnedDrawItems[static_cast<std::size_t>(skinnedDrawIndex)];
			item.transform.useMatrix = true;
			item.transform.matrix = world_[i];
		}

		SyncEntityRenderableForNode_(asset, scene, static_cast<int>(i));
	}

	SyncEditorRuntimeBindings(asset, scene);
	ValidateRuntimeMappingsDebug(asset, scene);
	transformsDirty_ = false;
}

std::size_t GetSkinnedDrawCount(const Scene& scene) const noexcept
{
	return scene.skinnedDrawItems.size();
}

SkinnedDrawItem* GetSkinnedDrawItem(Scene& scene, int skinnedDrawIndex) noexcept
{
	if (skinnedDrawIndex < 0 || static_cast<std::size_t>(skinnedDrawIndex) >= scene.skinnedDrawItems.size())
	{
		return nullptr;
	}
	return &scene.skinnedDrawItems[static_cast<std::size_t>(skinnedDrawIndex)];
}

const SkinnedDrawItem* GetSkinnedDrawItem(const Scene& scene, int skinnedDrawIndex) const noexcept
{
	if (skinnedDrawIndex < 0 || static_cast<std::size_t>(skinnedDrawIndex) >= scene.skinnedDrawItems.size())
	{
		return nullptr;
	}
	return &scene.skinnedDrawItems[static_cast<std::size_t>(skinnedDrawIndex)];
}