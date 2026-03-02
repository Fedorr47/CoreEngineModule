// ---------------- Build instance draw lists (ONE upload) ----------------
// We build two packings:
//   1) Shadow packing: per-mesh batching (used by directional/spot/point shadow passes)
//   2) Main packing: per-(mesh+material params) batching (used by MainPass)
//
// Then we concatenate them into a single instanceBuffer_ update.
// ---- Shadow packing (per mesh) ----
std::unordered_map<const rendern::MeshRHI*, std::vector<InstanceData>> shadowTmp;
shadowTmp.reserve(scene.drawItems.size());
std::uint32_t envSource = 0u;

for (const auto& item : scene.drawItems)
{
	const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
	if (!mesh || mesh->indexCount == 0)
	{
		continue;
	}
	const mathUtils::Mat4 model = item.transform.ToMatrix();
	// IMPORTANT: exclude alpha-blended objects from shadow casting
	MaterialParams params{};
	envSource = 0u;
	MaterialPerm perm = MaterialPerm::UseShadow;
	std::uint32_t itemEnvSource = 0u;

	if (item.material.id != 0)
	{
		const auto& mat = scene.GetMaterial(item.material);
		itemEnvSource = static_cast<std::uint32_t>(mat.envSource);
		params = mat.params;
		perm = EffectivePerm(mat);
		envSource = static_cast<std::uint32_t>(mat.envSource);
	}
	else
	{
		params.baseColor = { 1,1,1,1 };
		params.shininess = 32.0f;
		params.specStrength = 0.2f;
		params.shadowBias = 0.0f;
		params.albedoDescIndex = 0;
		perm = MaterialPerm::UseShadow;
	}

	const bool isTransparent = HasFlag(perm, MaterialPerm::Transparent) || (params.baseColor.w < 0.999f);
	const bool isPlanarMirror = HasFlag(perm, MaterialPerm::PlanarMirror);
	if (isTransparent || isPlanarMirror)
	{
		continue;
	}

	InstanceData inst{};
	inst.i0 = model[0];
	inst.i1 = model[1];
	inst.i2 = model[2];
	inst.i3 = model[3];

	shadowTmp[mesh].push_back(inst);
}

std::vector<InstanceData> shadowInstances;
std::vector<ShadowBatch> shadowBatches;
shadowInstances.reserve(scene.drawItems.size());
shadowBatches.reserve(shadowTmp.size());

{
	std::vector<const rendern::MeshRHI*> meshes;
	meshes.reserve(shadowTmp.size());
	for (auto& [shadowMesh, _] : shadowTmp)
	{
		meshes.push_back(shadowMesh);
	}
	std::sort(meshes.begin(), meshes.end());

	for (const rendern::MeshRHI* mesh : meshes)
	{
		auto& vec = shadowTmp[mesh];
		if (!mesh || vec.empty())
		{
			continue;
		}

		ShadowBatch shadowBatch{};
		shadowBatch.mesh = mesh;
		shadowBatch.instanceOffset = static_cast<std::uint32_t>(shadowInstances.size());
		shadowBatch.instanceCount = static_cast<std::uint32_t>(vec.size());

		shadowInstances.insert(shadowInstances.end(), vec.begin(), vec.end());
		shadowBatches.push_back(shadowBatch);
	}
}

// ---- Optional: layered point-shadow packing (duplicate instances x6 for cubemap slices) ----
// Layered point shadow renders into a Texture2DArray(6) in a single pass and uses
// SV_RenderTargetArrayIndex in VS. The shader assumes instance data is duplicated 6 times:
// for each original instance we emit faces 0..5 in order.
std::vector<InstanceData> shadowInstancesLayered;
std::vector<ShadowBatch> shadowBatchesLayered;
const bool buildLayeredPointShadow = (psoPointShadowLayered_ && !disablePointShadowLayered_) &&
device_.SupportsShaderModel6() && device_.SupportsVPAndRTArrayIndexFromAnyShader();
if (buildLayeredPointShadow && !shadowBatches.empty())
{
	constexpr std::uint32_t kPointShadowFaces = 6u;
	shadowInstancesLayered.reserve(static_cast<std::size_t>(shadowInstances.size()) * kPointShadowFaces);
	shadowBatchesLayered.reserve(shadowBatches.size());

	for (const ShadowBatch& sb : shadowBatches)
	{
		if (!sb.mesh || sb.instanceCount == 0)
		{
			continue;
		}

		ShadowBatch lb{};
		lb.mesh = sb.mesh;
		lb.instanceOffset = static_cast<std::uint32_t>(shadowInstancesLayered.size());
		lb.instanceCount = sb.instanceCount * kPointShadowFaces;

		const std::uint32_t begin = sb.instanceOffset;
		const std::uint32_t end = begin + sb.instanceCount;
		for (std::uint32_t i = begin; i < end; ++i)
		{
			const InstanceData& inst = shadowInstances[i];
			for (std::uint32_t face = 0; face < kPointShadowFaces; ++face)
			{
				shadowInstancesLayered.push_back(inst);
			}
		}

		shadowBatchesLayered.push_back(lb);
	}
}