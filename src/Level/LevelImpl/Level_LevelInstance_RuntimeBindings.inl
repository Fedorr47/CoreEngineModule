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
	if (slotName == "height" || slotName == "heightMap" || slotName == "displacement" || slotName == "disp")
	{
		outSlot = MaterialTextureSlot::Height;
		return true;
	}
	return false;
}

// -----------------------------
// Runtime: descriptor management
// -----------------------------
void ResolveTextureBindings(BindlessTable& bindless, Scene& scene)
{
    for (const PendingMaterialBinding& pendingBinding
         : pendingBindings_)
    {
        const rhi::TextureDescIndex descriptorIndex =
            GetOrCreateTextureDesc_(
                bindless,
                pendingBinding.textureId);

        if (descriptorIndex == 0)
        {
            continue;
        }

        Material& material =
            scene.GetMaterial(
                pendingBinding.material);

        switch (pendingBinding.slot)
        {
        case MaterialTextureSlot::Albedo:
            material.params.albedoDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Normal:
            material.params.normalDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Metalness:
            material.params.metalnessDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Roughness:
            material.params.roughnessDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::AO:
            material.params.aoDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Emissive:
            material.params.emissiveDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Specular:
            material.params.specularDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Gloss:
            material.params.glossDescIndex =
                descriptorIndex;
            break;

        case MaterialTextureSlot::Height:
            material.params.heightDescIndex =
                descriptorIndex;
            break;
        }
    }

    if (skyboxTextureId_)
    {
        const rhi::TextureDescIndex descriptorIndex =
            GetOrCreateTextureDesc_(
                bindless,
                *skyboxTextureId_);

        if (descriptorIndex != 0)
        {
            scene.skyboxDescIndex =
                descriptorIndex;
        }
    }

    for (ParticleEmitter& emitter
         : scene.particleEmitters)
    {
        if (emitter.textureId.empty())
        {
            emitter.textureDescIndex = 0;
            continue;
        }

        emitter.textureDescIndex =
            GetOrCreateTextureDesc_(
                bindless,
                emitter.textureId);
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