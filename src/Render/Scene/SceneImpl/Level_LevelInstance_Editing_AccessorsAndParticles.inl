void SetRootTransform(const mathUtils::Mat4& root)
{
	root_ = root;
	transformsDirty_ = true;
}

bool IsValidNodeIndex(const LevelAsset& asset, int nodeIndex) const noexcept
{
	return nodeIndex >= 0 && static_cast<std::size_t>(nodeIndex) < asset.nodes.size();
}

bool IsNodeAlive(const LevelAsset& asset, int nodeIndex) const noexcept
{
	if (!IsValidNodeIndex(asset, nodeIndex))
	{
		return false;
	}
	return asset.nodes[static_cast<std::size_t>(nodeIndex)].alive;
}

int GetNodeDrawIndex(int nodeIndex) const noexcept
{
	if (nodeIndex < 0)
	{
		return -1;
	}
	const std::size_t i = static_cast<std::size_t>(nodeIndex);
	if (i >= nodeToDraw_.size())
	{
		return -1;
	}
	if (i < nodeToDraws_.size() && !nodeToDraws_[i].empty())
	{
		return nodeToDraws_[i].front();
	}
	return nodeToDraw_[i];
}

const std::vector<int>& GetNodeDrawIndices(int nodeIndex) const noexcept
{
	static const std::vector<int> empty;
	if (nodeIndex < 0)
	{
		return empty;
	}
	const std::size_t i = static_cast<std::size_t>(nodeIndex);
	if (i >= nodeToDraws_.size())
	{
		return empty;
	}
	return nodeToDraws_[i];
}

int GetNodeSkinnedDrawIndex(int nodeIndex) const noexcept
{
	if (nodeIndex < 0)
	{
		return -1;
	}
	const std::size_t i = static_cast<std::size_t>(nodeIndex);
	if (i >= nodeToSkinnedDraw_.size())
	{
		return -1;
	}
	return nodeToSkinnedDraw_[i];
}

int GetNodeIndexFromSkinnedDrawIndex(int skinnedDrawIndex) const noexcept
{
	if (skinnedDrawIndex < 0)
	{
		return -1;
	}
	const std::size_t i = static_cast<std::size_t>(skinnedDrawIndex);
	if (i >= skinnedDrawToNode_.size())
	{
		return -1;
	}
	return skinnedDrawToNode_[i];
}

int GetNodeIndexFromDrawIndex(int drawIndex) const noexcept
{
	if (drawIndex < 0)
	{
		return -1;
	}
	const std::size_t i = static_cast<std::size_t>(drawIndex);
	if (i >= drawToNode_.size())
	{
		return -1;
	}
	return drawToNode_[i];
}

const LevelWorld& GetLevelWorld() const noexcept
{
	return ecs_;
}

EntityHandle GetNodeEntity(int nodeIndex) const noexcept
{
	return GetEntityForNode_(nodeIndex);
}

const mathUtils::Mat4& GetNodeWorldMatrix(int nodeIndex) const noexcept
{
	static const mathUtils::Mat4 identity{ 1.0f };
	if (nodeIndex < 0)
	{
		return identity;
	}
	const std::size_t i = static_cast<std::size_t>(nodeIndex);
	if (i >= world_.size())
	{
		return identity;
	}
	return world_[i];
}

mathUtils::Mat4 GetParentWorldMatrix(const LevelAsset& asset, int nodeIndex) const noexcept
{
	if (!IsValidNodeIndex(asset, nodeIndex))
	{
		return root_;
	}

	const LevelNode& node = asset.nodes[static_cast<std::size_t>(nodeIndex)];
	if (node.parent < 0)
	{
		return root_;
	}

	const std::size_t parentIndex = static_cast<std::size_t>(node.parent);
	if (parentIndex >= world_.size())
	{
		return root_;
	}

	return world_[parentIndex];
}

mathUtils::Vec3 GetNodeWorldPosition(int nodeIndex) const noexcept
{
	return GetNodeWorldMatrix(nodeIndex)[3].xyz();
}


bool IsParticleEmitterAlive(const LevelAsset& asset, int emitterIndex) const noexcept
{
	return emitterIndex >= 0 && static_cast<std::size_t>(emitterIndex) < asset.particleEmitters.size();
}

bool IsValidParticleEmitterIndex(int emitterIndex) const noexcept
{
	return emitterIndex >= 0 && static_cast<std::size_t>(emitterIndex) < particleEmitterToSceneEmitter_.size();
}

std::size_t GetParticleEmitterCount() const noexcept
{
	return particleEmitterToSceneEmitter_.size();
}

int AddParticleEmitter(LevelAsset& asset, Scene& scene, const ParticleEmitter& emitter)
{
	asset.particleEmitters.push_back(emitter);
	RebuildParticleEmitters_(asset, scene);
	SyncEditorRuntimeBindings(asset, scene);
	return static_cast<int>(asset.particleEmitters.size() - 1);
}

void DeleteParticleEmitter(LevelAsset& asset, Scene& scene, int emitterIndex)
{
	if (!IsParticleEmitterAlive(asset, emitterIndex))
	{
		return;
	}
	asset.particleEmitters.erase(asset.particleEmitters.begin() + static_cast<std::vector<ParticleEmitter>::difference_type>(emitterIndex));
	RebuildParticleEmitters_(asset, scene);
	if (scene.editorSelectedParticleEmitter == emitterIndex)
	{
		scene.editorSelectedParticleEmitter = -1;
	}
	else if (scene.editorSelectedParticleEmitter > emitterIndex)
	{
		--scene.editorSelectedParticleEmitter;
	}
	SyncEditorRuntimeBindings(asset, scene);
}

void RebuildParticleEmitters(Scene& scene, const LevelAsset& asset)
{
	RebuildParticleEmitters_(asset, scene);
}

void RestartParticleEmitter(const LevelAsset& asset, Scene& scene, int emitterIndex)
{
	if (!IsParticleEmitterAlive(asset, emitterIndex))
	{
		return;
	}
	ParticleEmitter& runtime = scene.particleEmitters[static_cast<std::size_t>(emitterIndex)];
	runtime = asset.particleEmitters[static_cast<std::size_t>(emitterIndex)];
	scene.RestartParticleEmitter(emitterIndex);
}

void SetParticleEmitterPosition(const LevelAsset& asset, Scene& scene, int emitterIndex, const mathUtils::Vec3& position)
{
	if (!IsParticleEmitterAlive(asset, emitterIndex))
	{
		return;
	}
	if (static_cast<std::size_t>(emitterIndex) >= scene.particleEmitters.size())
	{
		return;
	}
	scene.particleEmitters[static_cast<std::size_t>(emitterIndex)].position = position;
}

void TriggerParticleEmitterBurst(const LevelAsset& asset, Scene& scene, int emitterIndex)
{
	if (!IsParticleEmitterAlive(asset, emitterIndex))
	{
		return;
	}
	if (static_cast<std::size_t>(emitterIndex) >= scene.particleEmitters.size())
	{
		return;
	}
	ParticleEmitter& runtime = scene.particleEmitters[static_cast<std::size_t>(emitterIndex)];
	const ParticleEmitter& authoring = asset.particleEmitters[static_cast<std::size_t>(emitterIndex)];
	for (std::uint32_t i = 0; i < authoring.burstCount; ++i)
	{
		scene.EmitParticleFromEmitter(runtime, emitterIndex);
	}
}

const ParticleEmitter* GetRuntimeParticleEmitter(const Scene& scene, int emitterIndex) const noexcept
{
	if (emitterIndex < 0 || static_cast<std::size_t>(emitterIndex) >= scene.particleEmitters.size())
	{
		return nullptr;
	}
	return &scene.particleEmitters[static_cast<std::size_t>(emitterIndex)];
}

ParticleEmitter* GetRuntimeParticleEmitter(Scene& scene, int emitterIndex) noexcept
{
	if (emitterIndex < 0 || static_cast<std::size_t>(emitterIndex) >= scene.particleEmitters.size())
	{
		return nullptr;
	}
	return &scene.particleEmitters[static_cast<std::size_t>(emitterIndex)];
}

