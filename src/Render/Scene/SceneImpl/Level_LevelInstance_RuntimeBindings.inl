bool TryParseMaterialTextureSlot_(std::string_view slotName, MaterialTextureSlot& outSlot) const noexcept
{
	if (slotName == "albedo")
	{
		outSlot = MaterialTextureSlot::Albedo;
		return true;
	}
	if (slotName == "normal")
	{
		outSlot = MaterialTextureSlot::Normal;
		return true;
	}
	if (slotName == "metalness" || slotName == "metallic")
	{
		outSlot = MaterialTextureSlot::Metalness;
		return true;
	}
	if (slotName == "roughness")
	{
		outSlot = MaterialTextureSlot::Roughness;
		return true;
	}
	if (slotName == "ao")
	{
		outSlot = MaterialTextureSlot::AO;
		return true;
	}
	if (slotName == "emissive")
	{
		outSlot = MaterialTextureSlot::Emissive;
		return true;
	}
	if (slotName == "specular" || slotName == "spec")
	{
		outSlot = MaterialTextureSlot::Specular;
		return true;
	}
	if (slotName == "gloss" || slotName == "glossiness")
	{
		outSlot = MaterialTextureSlot::Gloss;
		return true;
	}
	return false;
}

// -----------------------------
// Runtime: descriptor management
// -----------------------------
void ResolveTextureBindings(AssetManager& assets, BindlessTable& bindless, Scene& scene)
{
	ResourceManager& rm = assets.GetResourceManager();


	// Materials
	for (auto& pb : pendingBindings_)
	{
		rhi::TextureDescIndex idx = GetOrCreateTextureDesc_(rm, bindless, pb.textureId);
		if (idx == 0)
		{
			continue;
		}

		Material& m = scene.GetMaterial(pb.material);
		switch (pb.slot)
		{
		case MaterialTextureSlot::Albedo:    m.params.albedoDescIndex = idx; break;
		case MaterialTextureSlot::Normal:    m.params.normalDescIndex = idx; break;
		case MaterialTextureSlot::Metalness: m.params.metalnessDescIndex = idx; break;
		case MaterialTextureSlot::Roughness: m.params.roughnessDescIndex = idx; break;
		case MaterialTextureSlot::AO:        m.params.aoDescIndex = idx; break;
		case MaterialTextureSlot::Emissive:  m.params.emissiveDescIndex = idx; break;
		case MaterialTextureSlot::Specular:  m.params.specularDescIndex = idx; break;
		case MaterialTextureSlot::Gloss:     m.params.glossDescIndex = idx; break;
		}
	}

	// Skybox
	if (skyboxTextureId_)
	{
		rhi::TextureDescIndex idx = GetOrCreateTextureDesc_(rm, bindless, *skyboxTextureId_);
		if (idx != 0)
		{
			scene.skyboxDescIndex = idx;
		}
	}

	// Particle emitters
	for (auto& emitter : scene.particleEmitters)
	{
		if (!emitter.textureId.empty())
		{
			emitter.textureDescIndex = GetOrCreateTextureDesc_(rm, bindless, emitter.textureId);
		}
		else
		{
			emitter.textureDescIndex = 0;
		}
	}
}

void FreeDescriptors(BindlessTable& bindless) noexcept
{
	for (auto& [_, idx] : textureDesc_)
	{
		if (idx != 0)
		{
			bindless.UnregisterTexture(idx);
		}
	}
	textureDesc_.clear();
}

// Ensure that a materialId exists as a runtime Scene material handle.
// Useful for editor-created materials or late-added materials.
MaterialHandle EnsureMaterial(const LevelAsset& asset, Scene& scene, std::string_view materialId)
{
	if (materialId.empty())
	{
		return {};
	}

	const std::string id{ materialId };
	MaterialHandle h{};
	if (auto it = materialHandles_.find(id); it != materialHandles_.end())
	{
		h = it->second;
	}

	auto defIt = asset.materials.find(id);
	if (defIt == asset.materials.end())
	{
		return {};
	}

	if (!h)
	{
		h = scene.CreateMaterial(defIt->second.material);
		materialHandles_[id] = h;
	}
	else
	{
		scene.GetMaterial(h) = defIt->second.material;
	}

	pendingBindings_.erase(
		std::remove_if(
			pendingBindings_.begin(),
			pendingBindings_.end(),
			[h](const PendingMaterialBinding& pb)
			{
				return pb.material == h;
			}),
		pendingBindings_.end());

	// Register texture bindings (resolved later as textures upload to GPU).
	for (const auto& [slot, texId] : defIt->second.textureBindings)
	{
		PendingMaterialBinding pb;
		pb.material = h;
		pb.textureId = texId;

		if (!TryParseMaterialTextureSlot_(slot, pb.slot))
		{
			throw std::runtime_error("Level: unknown material texture slot: " + slot);
		}

		pendingBindings_.push_back(std::move(pb));
	}

	return h;
}