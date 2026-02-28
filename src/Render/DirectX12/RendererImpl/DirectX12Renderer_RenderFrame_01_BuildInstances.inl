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

std::unordered_map<BatchKey, BatchTemp, BatchKeyHash, BatchKeyEq> mainTmp;
mainTmp.reserve(scene.drawItems.size());

std::vector<InstanceData> transparentInstances;
transparentInstances.reserve(scene.drawItems.size());

std::vector<TransparentTemp> transparentTmp;
transparentTmp.reserve(scene.drawItems.size());

std::vector<InstanceData> planarMirrorInstances;
planarMirrorInstances.reserve(std::min<std::size_t>(scene.drawItems.size(), static_cast<std::size_t>(settings_.planarReflectionMaxMirrors)));

std::vector<PlanarMirrorDraw> planarMirrorDraws;
planarMirrorDraws.reserve(std::min<std::size_t>(scene.drawItems.size(), static_cast<std::size_t>(settings_.planarReflectionMaxMirrors)));

// ---------------- Reflection probe assignment (multi-probe) ----------------
drawItemReflectionProbeIndices_.assign(scene.drawItems.size(), -1);
reflectiveOwnerDrawItems_.clear();
reflectiveOwnerDrawItems_.reserve(scene.drawItems.size());

auto IsReflectionCaptureReceiver = [&scene](int drawItemIndex) -> bool
	{
		if (drawItemIndex < 0 || static_cast<std::size_t>(drawItemIndex) >= scene.drawItems.size())
			return false;

		const DrawItem& di = scene.drawItems[static_cast<std::size_t>(drawItemIndex)];
		if (di.material.id == 0)
			return false;

		const auto& mat = scene.GetMaterial(di.material);
		return mat.envSource == EnvSource::ReflectionCapture;
	};

for (std::size_t i = 0; i < scene.drawItems.size(); ++i)
{
	if (!IsReflectionCaptureReceiver(static_cast<int>(i)))
		continue;

	if (reflectiveOwnerDrawItems_.size() >= kMaxReflectionProbes)
		break;

	const int probeIndex = static_cast<int>(reflectiveOwnerDrawItems_.size());
	reflectiveOwnerDrawItems_.push_back(static_cast<int>(i));
	drawItemReflectionProbeIndices_[i] = probeIndex;
}

EnsureReflectionProbeResources(reflectiveOwnerDrawItems_.size());

// ---- Main packing: opaque (batched) + transparent (sorted per-item) ----
// NOTE: mainTmp is camera-culled (IsVisible), but reflection capture must NOT depend on the camera.
// We therefore build an additional "no-cull" packing for reflection capture / cube atlas.
const bool buildCaptureNoCull = settings_.enableReflectionCapture || settings_.ShowCubeAtlas || settings_.enablePlanarReflections;
std::unordered_map<BatchKey, BatchTemp, BatchKeyHash, BatchKeyEq> captureTmp;
if (buildCaptureNoCull)
{
	captureTmp.reserve(scene.drawItems.size());
}
for (std::size_t drawItemIndex = 0; drawItemIndex < scene.drawItems.size(); ++drawItemIndex)
{
	const auto& item = scene.drawItems[drawItemIndex];
	const rendern::MeshRHI* mesh = item.mesh ? &item.mesh->GetResource() : nullptr;
	if (!mesh || mesh->indexCount == 0)
	{
		continue;
	}

	const mathUtils::Mat4 model = item.transform.ToMatrix();
	// Camera visibility is used only for MAIN/transparent lists.
	// Reflection capture uses a separate no-cull packing (captureTmp).
	const bool visibleInMain = IsVisible(item.mesh.get(), model);

	BatchKey key{};
	key.mesh = mesh;

	MaterialParams params{};
	MaterialPerm perm = MaterialPerm::UseShadow;
	std::uint32_t itemEnvSource = 0u;
	if (item.material.id != 0)
	{
		const auto& mat = scene.GetMaterial(item.material);
		itemEnvSource = static_cast<std::uint32_t>(mat.envSource);
		params = mat.params;
		perm = EffectivePerm(mat);
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

	key.permBits = static_cast<std::uint32_t>(perm);
	key.envSource = itemEnvSource;
	if (drawItemIndex < drawItemReflectionProbeIndices_.size())
	{
		key.reflectionProbeIndex = drawItemReflectionProbeIndices_[drawItemIndex];
	}

	// IMPORTANT: BatchKey must include material parameters,
	// otherwise different materials get incorrectly merged.
	key.albedoDescIndex = params.albedoDescIndex;
	key.normalDescIndex = params.normalDescIndex;
	key.metalnessDescIndex = params.metalnessDescIndex;
	key.roughnessDescIndex = params.roughnessDescIndex;
	key.aoDescIndex = params.aoDescIndex;
	key.emissiveDescIndex = params.emissiveDescIndex;

	key.baseColor = params.baseColor;
	key.shadowBias = params.shadowBias; // texels

	key.metallic = params.metallic;
	key.roughness = params.roughness;
	key.ao = params.ao;
	key.emissiveStrength = params.emissiveStrength;

	// Legacy
	key.shininess = params.shininess;
	key.specStrength = params.specStrength;

	// Instance (ROWS)
	const bool isTransparent = HasFlag(perm, MaterialPerm::Transparent) || (params.baseColor.w < 0.999f);
	const bool isPlanarMirror = HasFlag(perm, MaterialPerm::PlanarMirror);
	InstanceData inst{};
	inst.i0 = model[0];
	inst.i1 = model[1];
	inst.i2 = model[2];
	inst.i3 = model[3];

	// Reflection-capture packing is NO-CULL: add before camera-cull so capture does not depend on the editor camera
	if (buildCaptureNoCull && !isTransparent)
	{
		auto& bucket = captureTmp[key];
		if (bucket.inst.empty())
		{
			bucket.materialHandle = item.material;
			bucket.material = params;
			bucket.reflectionProbeIndex = key.reflectionProbeIndex;
		}
		bucket.inst.push_back(inst);
	}

	// Main pass: camera-culled.
	if (!visibleInMain)
	{
		continue;
	}

	if (isTransparent)
	{
		mathUtils::Vec3 sortPos = item.transform.position;
		const auto& b = item.mesh->GetBounds();
		if (b.sphereRadius > 0.0f)
		{
			const mathUtils::Vec4 wc4 = model * mathUtils::Vec4(b.sphereCenter, 1.0f);
			sortPos = mathUtils::Vec3(wc4.x, wc4.y, wc4.z);
		}
		else
		{
			sortPos = mathUtils::Vec3(model[3].x, model[3].y, model[3].z);
		}

		const mathUtils::Vec3 deltaToCamera = sortPos - camPos;
		const float dist2 = mathUtils::Dot(deltaToCamera, deltaToCamera);
		const std::uint32_t localOff = static_cast<std::uint32_t>(transparentInstances.size());
		transparentInstances.push_back(inst);
		transparentTmp.push_back(TransparentTemp{ mesh, params, item.material, localOff, dist2 });

		continue;
	}

	if (settings_.enablePlanarReflections && isPlanarMirror && !isTransparent &&
		planarMirrorDraws.size() < static_cast<std::size_t>(settings_.planarReflectionMaxMirrors))
	{
		const auto TransformPoint = [](const mathUtils::Mat4& m, const mathUtils::Vec3& v) noexcept -> mathUtils::Vec3
			{
				const mathUtils::Vec4 r = m * mathUtils::Vec4(v, 1.0f);
				return { r.x, r.y, r.z };
			};

		const auto TransformVector = [](const mathUtils::Mat4& m, const mathUtils::Vec3& v) noexcept -> mathUtils::Vec3
			{
				const mathUtils::Vec4 r = m * mathUtils::Vec4(v, 0.0f);
				return { r.x, r.y, r.z };
			};

		PlanarMirrorDraw mirror{};
		mirror.mesh = mesh;
		mirror.material = params;
		mirror.materialHandle = item.material;
		mirror.instanceOffset = static_cast<std::uint32_t>(planarMirrorInstances.size());

		const mathUtils::Vec3 worldX = TransformVector(model, mathUtils::Vec3(1.0f, 0.0f, 0.0f));
		const mathUtils::Vec3 worldY = TransformVector(model, mathUtils::Vec3(0.0f, 1.0f, 0.0f));
		mirror.planePoint = TransformPoint(model, mathUtils::Vec3(0.0f, 0.0f, 0.0f));
		mirror.planeNormal = mathUtils::Cross(worldX, worldY);

		if (mathUtils::Length(mirror.planeNormal) > 0.0001f)
		{
			mirror.planeNormal = mathUtils::Normalize(mirror.planeNormal);
			planarMirrorInstances.push_back(inst);
			planarMirrorDraws.push_back(mirror);
		}

		continue;
	}

	auto& bucket = mainTmp[key];
	if (bucket.inst.empty())
	{
		bucket.materialHandle = item.material;
		bucket.material = params; // representative material for this batch
		bucket.reflectionProbeIndex = key.reflectionProbeIndex;
	}
	bucket.inst.push_back(inst);
}

std::vector<InstanceData> mainInstances;
mainInstances.reserve(scene.drawItems.size());

std::vector<Batch> mainBatches;
mainBatches.reserve(mainTmp.size());

for (auto& [key, bt] : mainTmp)
{
	if (bt.inst.empty())
	{
		continue;
	}

	Batch batch{};
	batch.mesh = key.mesh;
	batch.materialHandle = bt.materialHandle;
	batch.material = bt.material;
	batch.instanceOffset = static_cast<std::uint32_t>(mainInstances.size());
	batch.instanceCount = static_cast<std::uint32_t>(bt.inst.size());

	batch.reflectionProbeIndex = bt.reflectionProbeIndex;

	mainInstances.insert(mainInstances.end(), bt.inst.begin(), bt.inst.end());
	mainBatches.push_back(batch);
}

// ---- Reflection-capture no-cull packing (opaque) ----
std::vector<InstanceData> captureMainInstancesNoCull;
std::vector<Batch> captureMainBatchesNoCull;

if (buildCaptureNoCull && !captureTmp.empty())
{
	captureMainInstancesNoCull.reserve(scene.drawItems.size());
	captureMainBatchesNoCull.reserve(captureTmp.size());

	for (auto& [key, bt] : captureTmp)
	{
		if (bt.inst.empty())
			continue;

		Batch batch{};
		batch.mesh = key.mesh;
		batch.materialHandle = bt.materialHandle;
		batch.material = bt.material;
		batch.instanceOffset = static_cast<std::uint32_t>(captureMainInstancesNoCull.size());
		batch.instanceCount = static_cast<std::uint32_t>(bt.inst.size());
		batch.reflectionProbeIndex = bt.reflectionProbeIndex;

		captureMainInstancesNoCull.insert(captureMainInstancesNoCull.end(), bt.inst.begin(), bt.inst.end());
		captureMainBatchesNoCull.push_back(batch);
	}
}

// ---- Optional: layered reflection-capture packing (duplicate MAIN instances x6 for cubemap slices) ----
// Layered reflection capture uses SV_RenderTargetArrayIndex in VS and assumes each original instance
// is duplicated 6 times in order (faces 0..5).
std::vector<InstanceData> reflectionInstancesLayered;
std::vector<Batch> reflectionBatchesLayered;

const bool buildLayeredReflectionCapture =
(psoReflectionCaptureLayered_ && !disableReflectionCaptureLayered_) &&
device_.SupportsShaderModel6() && device_.SupportsVPAndRTArrayIndexFromAnyShader();

if (buildLayeredReflectionCapture && !captureMainBatchesNoCull.empty())
{
	constexpr std::uint32_t kFaces = 6u;

	// reserve roughly
	std::size_t totalMainInst = 0;
	for (const Batch& b : captureMainBatchesNoCull)
	{
		totalMainInst += b.instanceCount;
	}

	reflectionInstancesLayered.reserve(totalMainInst * kFaces);
	reflectionBatchesLayered.reserve(captureMainBatchesNoCull.size());

	for (const Batch& b : captureMainBatchesNoCull)
	{
		if (!b.mesh || b.instanceCount == 0)
			continue;

		Batch lb = b;
		lb.instanceOffset = static_cast<std::uint32_t>(reflectionInstancesLayered.size());
		lb.instanceCount = b.instanceCount * kFaces;

		const std::uint32_t begin = b.instanceOffset;
		const std::uint32_t end = begin + b.instanceCount;

		for (std::uint32_t i = begin; i < end; ++i)
		{
			const InstanceData& inst = captureMainInstancesNoCull[i];
			for (std::uint32_t face = 0; face < kFaces; ++face)
			{
				reflectionInstancesLayered.push_back(inst);
			}
		}

		reflectionBatchesLayered.push_back(lb);
	}
}

// ---- Combine and upload once ----
auto AlignUpU32 = [](std::uint32_t v, std::uint32_t a) -> std::uint32_t
	{
		return (v + (a - 1u)) / a * a;
	};
const std::uint32_t shadowBase = 0;
const std::uint32_t mainBase = static_cast<std::uint32_t>(shadowInstances.size());
const std::uint32_t captureMainBase = static_cast<std::uint32_t>(shadowInstances.size() + mainInstances.size());
const std::uint32_t transparentBase = captureMainBase + static_cast<std::uint32_t>(captureMainInstancesNoCull.size());
const std::uint32_t planarMirrorBase = transparentBase + static_cast<std::uint32_t>(transparentInstances.size());

const std::uint32_t transparentEnd =
planarMirrorBase + static_cast<std::uint32_t>(planarMirrorInstances.size());
const std::uint32_t layeredShadowBase = AlignUpU32(transparentEnd, 6u);
const std::uint32_t layeredReflectionBase =
AlignUpU32(layeredShadowBase + static_cast<std::uint32_t>(shadowInstancesLayered.size()), 6u);

for (auto& sbatch : shadowBatches)
{
	sbatch.instanceOffset += shadowBase;
}
for (auto& mbatch : mainBatches)
{
	mbatch.instanceOffset += mainBase;
}
for (auto& cbatch : captureMainBatchesNoCull)
{
	cbatch.instanceOffset += captureMainBase;
}
for (auto& lbatch : shadowBatchesLayered)
{
	lbatch.instanceOffset += layeredShadowBase;
}
for (auto& rbatch : reflectionBatchesLayered)
{
	rbatch.instanceOffset += layeredReflectionBase;
}
for (auto& mirrorDraw : planarMirrorDraws)
{
	mirrorDraw.instanceOffset = planarMirrorBase + mirrorDraw.instanceOffset;
}

std::vector<TransparentDraw> transparentDraws;
transparentDraws.reserve(transparentTmp.size());
for (const auto& transparentInst : transparentTmp)
{
	TransparentDraw transparentDraw{};
	transparentDraw.mesh = transparentInst.mesh;
	transparentDraw.material = transparentInst.material;
	transparentDraw.materialHandle = transparentInst.materialHandle;
	transparentDraw.instanceOffset = transparentBase + transparentInst.localInstanceOffset;
	transparentDraw.dist2 = transparentInst.dist2;
	transparentDraws.push_back(transparentDraw);
}

std::sort(transparentDraws.begin(), transparentDraws.end(),
	[](const TransparentDraw& first, const TransparentDraw& second)
	{
		return first.dist2 > second.dist2; // far -> near
	});

std::vector<InstanceData> combinedInstances;
const std::uint32_t finalCount =
layeredReflectionBase + static_cast<std::uint32_t>(reflectionInstancesLayered.size());

combinedInstances.clear();
combinedInstances.reserve(finalCount);

// 1) normal groups
combinedInstances.insert(combinedInstances.end(), shadowInstances.begin(), shadowInstances.end());
combinedInstances.insert(combinedInstances.end(), mainInstances.begin(), mainInstances.end());
combinedInstances.insert(combinedInstances.end(), captureMainInstancesNoCull.begin(), captureMainInstancesNoCull.end());
combinedInstances.insert(combinedInstances.end(), transparentInstances.begin(), transparentInstances.end());
combinedInstances.insert(combinedInstances.end(), planarMirrorInstances.begin(), planarMirrorInstances.end());

// 2) pad up to layeredShadowBase (between transparent/planar-mirror and layered shadow)
if (combinedInstances.size() < layeredShadowBase)
	combinedInstances.resize(layeredShadowBase);

// 3) layered shadow
combinedInstances.insert(combinedInstances.end(),
	shadowInstancesLayered.begin(), shadowInstancesLayered.end());

// 4) pad up to layeredReflectionBase (between layered shadow and layered reflection)
if (combinedInstances.size() < layeredReflectionBase)
	combinedInstances.resize(layeredReflectionBase);

// 5) layered reflection
combinedInstances.insert(combinedInstances.end(),
	reflectionInstancesLayered.begin(), reflectionInstancesLayered.end());

assert(shadowBase == 0u);
assert(mainBase == shadowInstances.size());
assert(captureMainBase == shadowInstances.size() + mainInstances.size());
assert(transparentBase == captureMainBase + captureMainInstancesNoCull.size());
assert(planarMirrorBase == transparentBase + transparentInstances.size());
assert(layeredShadowBase >= planarMirrorBase + planarMirrorInstances.size());
assert(layeredReflectionBase >= layeredShadowBase + shadowInstancesLayered.size());
assert(combinedInstances.size() == finalCount);

const std::uint32_t instStride = static_cast<std::uint32_t>(sizeof(InstanceData));

if (!combinedInstances.empty())
{
	const std::size_t bytes = combinedInstances.size() * sizeof(InstanceData);
	if (bytes > instanceBufferSizeBytes_)
	{
		throw std::runtime_error("DX12Renderer: instance buffer overflow (increase instanceBufferSizeBytes_)");
	}
	device_.UpdateBuffer(instanceBuffer_, std::as_bytes(std::span{ combinedInstances }));
}

if (settings_.debugPrintDrawCalls)
{
	static std::uint32_t frame = 0;
	if ((++frame % 60u) == 0u)
	{
		std::cout << "[DX12] MainPass draw calls: " << mainBatches.size()
			<< " (instances main: " << mainInstances.size()
			<< ", shadow: " << shadowInstances.size() << ")"
			<< " | DepthPrepass: " << (settings_.enableDepthPrepass ? "ON" : "OFF")
			<< " (draw calls: " << shadowBatches.size() << ")\n";
	}
}