// ---------------- Deferred path (DX12) ----------------
const bool canDeferred =
settings_.enableDeferred &&
device_.GetBackend() == rhi::Backend::DirectX12 &&
psoDeferredGBuffer_ &&
psoDeferredLighting_ &&
fullscreenLayout_ &&
swapChain.GetDepthTexture();

std::uint32_t activeReflectionProbeCount = 0u;

struct EditorSelectionLists
{
	std::vector<EditorSelectionDraw> opaque{};
	std::vector<EditorSelectionDraw> transparent{};
	std::vector<InstanceData> instances{};
	std::vector<std::uint32_t> opaqueStarts{};
	std::vector<std::uint32_t> transparentStarts{};
};

constexpr std::uint32_t kEditorOutlineStencilRef = 0x80u;
auto BuildEditorSelectionLists = [&]() -> EditorSelectionLists
	{
		EditorSelectionLists result{};
		constexpr std::size_t kMaxSelectionInstances = 4096;

		const std::size_t selectedStaticCount = scene.editorSelectedDrawItems.size();
		const std::size_t selectedSkinnedCount = scene.editorSelectedSkinnedDrawItems.size();
		const std::size_t selectedTotal = selectedStaticCount + selectedSkinnedCount;
		result.opaque.reserve(selectedTotal);
		result.transparent.reserve(selectedTotal);
		result.instances.reserve(std::min(selectedStaticCount, kMaxSelectionInstances));
		result.opaqueStarts.reserve(selectedTotal);
		result.transparentStarts.reserve(selectedTotal);

		auto PushSelection = [&](EditorSelectionDraw sel, std::uint32_t startInstance)
			{
				if (sel.isTransparent)
				{
					result.transparent.push_back(std::move(sel));
					result.transparentStarts.push_back(startInstance);
				}
				else
				{
					result.opaque.push_back(std::move(sel));
					result.opaqueStarts.push_back(startInstance);
				}
			};

		for (const int diIndex : scene.editorSelectedDrawItems)
		{
			if (diIndex < 0)
			{
				continue;
			}
			if (result.instances.size() >= kMaxSelectionInstances)
			{
				break;
			}

			const std::size_t idx = static_cast<std::size_t>(diIndex);
			if (idx >= scene.drawItems.size())
			{
				continue;
			}

			const DrawItem& di = scene.drawItems[idx];
			const rendern::MeshRHI* mesh = di.mesh ? &di.mesh->GetResource() : nullptr;
			if (!mesh || mesh->indexCount == 0 || !mesh->vertexBuffer || !mesh->indexBuffer)
			{
				continue;
			}

			EditorSelectionDraw sel{};
			sel.mesh = mesh;

			const mathUtils::Mat4 model = di.transform.ToMatrix();
			sel.instance.i0 = model[0];
			sel.instance.i1 = model[1];
			sel.instance.i2 = model[2];
			sel.instance.i3 = model[3];

			sel.outlineWorldOffset = 0.01f;
			if (di.mesh)
			{
				const auto& bounds = di.mesh->GetBounds();
				if (bounds.sphereRadius > 0.0f)
				{
					sel.outlineWorldOffset = std::max(0.01f, bounds.sphereRadius * 0.03f);
				}
			}

			if (di.material.id != 0)
			{
				const auto& mat = scene.GetMaterial(di.material);
				const MaterialPerm perm = EffectivePerm(mat);
				sel.isTransparent = HasFlag(perm, MaterialPerm::Transparent);
			}
			else
			{
				sel.isTransparent = false;
			}

			const std::uint32_t startInstance = static_cast<std::uint32_t>(result.instances.size());
			result.instances.push_back(sel.instance);
			PushSelection(std::move(sel), startInstance);
		}

		std::unordered_map<int, const SkinnedOpaqueDraw*> selectedSkinnedLookup{};
		selectedSkinnedLookup.reserve(skinnedOpaqueDraws.size());
		for (const SkinnedOpaqueDraw& draw : skinnedOpaqueDraws)
		{
			if (draw.sourceSkinnedDrawIndex >= 0)
			{
				selectedSkinnedLookup.emplace(draw.sourceSkinnedDrawIndex, &draw);
			}
		}

		for (const int skinnedIndex : scene.editorSelectedSkinnedDrawItems)
		{
			if (skinnedIndex < 0)
			{
				continue;
			}
			const auto it = selectedSkinnedLookup.find(skinnedIndex);
			if (it == selectedSkinnedLookup.end() || !it->second || !it->second->mesh)
			{
				continue;
			}

			const SkinnedOpaqueDraw& draw = *it->second;
			EditorSelectionDraw sel{};
			sel.skinnedMesh = draw.mesh;
			sel.model = draw.model;
			sel.paletteOffset = draw.paletteOffset;
			sel.boneCount = draw.boneCount;
			sel.isSkinned = true;
			sel.outlineWorldOffset = 0.01f;
			if (static_cast<std::size_t>(skinnedIndex) < scene.skinnedDrawItems.size())
			{
				const SkinnedDrawItem& item = scene.skinnedDrawItems[static_cast<std::size_t>(skinnedIndex)];
				if (item.asset)
				{
					const auto& bounds = item.asset->mesh.bounds.maxAnimatedBounds;
					if (bounds.sphereRadius > 0.0f)
					{
						sel.outlineWorldOffset = std::max(0.01f, bounds.sphereRadius * 0.03f);
					}
				}
			}
			if (draw.materialHandle.id != 0)
			{
				const auto& mat = scene.GetMaterial(draw.materialHandle);
				const MaterialPerm perm = EffectivePerm(mat);
				sel.isTransparent = HasFlag(perm, MaterialPerm::Transparent);
			}
			PushSelection(std::move(sel), 0u);
		}

		return result;
	};

const EditorSelectionLists editorSelection = BuildEditorSelectionLists();
const auto& selectionOpaque = editorSelection.opaque;
const auto& selectionTransparent = editorSelection.transparent;
const auto& selectionInstances = editorSelection.instances;
const auto& selectionOpaqueStart = editorSelection.opaqueStarts;
const auto& selectionTransparentStart = editorSelection.transparentStarts;

auto UploadEditorSelectionInstances = [&]()
	{
		if (!highlightInstanceBuffer_ || selectionInstances.empty())
		{
			return;
		}

	};

UploadEditorSelectionInstances();

auto ComputeForwardGBufferReflectionMeta = [&](MaterialHandle materialHandle, int reflectionProbeIndex, std::uint32_t activeProbeCount)
	{
		std::pair<float, float> result{ 0.0f, 0.0f };
		if (!settings_.enableReflectionCapture || materialHandle.id == 0 || activeProbeCount == 0u)
		{
			return result;
		}

		const auto& mat = scene.GetMaterial(materialHandle);
		if (mat.envSource != EnvSource::ReflectionCapture || reflectionProbeIndex < 0 || static_cast<std::uint32_t>(reflectionProbeIndex) >= activeProbeCount)
		{
			return result;
		}

		result.first = 1.0f;
		result.second = (static_cast<float>(reflectionProbeIndex) + 0.5f) / static_cast<float>(activeProbeCount);
		return result;
	};

auto ComputeDeferredGBufferReflectionMeta = [&](MaterialHandle materialHandle, int reflectionProbeIndex, const std::vector<int>& deferredProbeRemap, std::uint32_t activeProbeCount)
	{
		std::pair<float, float> result{ 0.0f, 0.0f };
		if (!settings_.enableReflectionCapture || materialHandle.id == 0 || reflectionProbeIndex < 0 || activeProbeCount == 0u || static_cast<std::size_t>(reflectionProbeIndex) >= deferredProbeRemap.size())
		{
			return result;
		}

		const auto& mat = scene.GetMaterial(materialHandle);
		if (mat.envSource != EnvSource::ReflectionCapture)
		{
			return result;
		}

		const int compactProbeIndex = deferredProbeRemap[static_cast<std::size_t>(reflectionProbeIndex)];
		if (compactProbeIndex < 0)
		{
			return result;
		}

		result.first = 1.0f;
		result.second = (static_cast<float>(compactProbeIndex) + 0.5f) / static_cast<float>(activeProbeCount);
		return result;
	};

auto MakeEditorSelectionConstants = [&](const mathUtils::Mat4& viewProj,
	const mathUtils::Mat4& dirLightViewProj,
	const mathUtils::Vec3& camPosLocal,
	const mathUtils::Vec3& camFLocal,
	const mathUtils::Vec4& baseColor) -> PerBatchConstants
	{
		PerBatchConstants constants{};
		const mathUtils::Mat4 viewProjT = mathUtils::Transpose(viewProj);
		const mathUtils::Mat4 dirVP_T = mathUtils::Transpose(dirLightViewProj);
		std::memcpy(constants.uViewProj.data(), mathUtils::ValuePtr(viewProjT), sizeof(float) * 16);
		std::memcpy(constants.uLightViewProj.data(), mathUtils::ValuePtr(dirVP_T), sizeof(float) * 16);
		constants.uCameraAmbient = { camPosLocal.x, camPosLocal.y, camPosLocal.z, 0.0f };
		constants.uCameraForward = { camFLocal.x, camFLocal.y, camFLocal.z, 0.0f };
		constants.uBaseColor = { baseColor.x, baseColor.y, baseColor.z, baseColor.w };
		constants.uMaterialFlags = { 0.0f, 0.0f, 0.0f, AsFloatBits(0u) };
		constants.uPbrParams = { 0.0f, 1.0f, 1.0f, 0.0f };
		constants.uCounts = { 0.0f, 0.0f, 0.0f, 0.0f };
		constants.uShadowBias = { 0.0f, 0.0f, 0.0f, 0.0f };
		constants.uEnvProbeBoxMin = { 0.0f, 0.0f, 0.0f, 0.0f };
		constants.uEnvProbeBoxMax = { 0.0f, 0.0f, 0.0f, 0.0f };
		return constants;
	};

auto DrawEditorSelectionGroup = [&](renderGraph::PassContext& ctx,
	const rhi::GraphicsState& restoreState,
	const mathUtils::Vec4& highlightColor,
	const mathUtils::Mat4& viewProj,
	const mathUtils::Mat4& dirLightViewProj,
	const mathUtils::Vec3& camPosLocal,
	const mathUtils::Vec3& camFLocal,
	const rhi::Extent2D& extent,
	const std::vector<EditorSelectionDraw>& group,
	const std::vector<std::uint32_t>& starts)
	{
		if (group.empty())
		{
			return;
		}

		if (highlightInstanceBuffer_ && !selectionInstances.empty())
		{
			device_.UpdateBuffer(highlightInstanceBuffer_, std::as_bytes(std::span{ selectionInstances }));
		}

		auto BindEditorSelectionGeometry = [&](const rendern::MeshRHI& mesh)
			{
				ctx.commandList.BindInputLayout(mesh.layoutInstanced);
				ctx.commandList.BindVertexBuffer(0, mesh.vertexBuffer, mesh.vertexStrideBytes, 0);
				ctx.commandList.BindVertexBuffer(1, highlightInstanceBuffer_, instStride, 0);
				ctx.commandList.BindIndexBuffer(mesh.indexBuffer, mesh.indexType, 0);
			};

		auto BindEditorSelectionSkinnedGeometry = [&](const rendern::SkinnedMeshRHI& mesh)
			{
				ctx.commandList.BindInputLayout(mesh.layout);
				ctx.commandList.BindVertexBuffer(0, mesh.vertexBuffer, mesh.vertexStrideBytes, 0);
				ctx.commandList.BindIndexBuffer(mesh.indexBuffer, mesh.indexType, 0);
				ctx.commandList.BindStructuredBufferSRV(19, skinPaletteBuffer_);
			};

		auto MakeEditorSelectionSkinnedConstants = [&](const mathUtils::Mat4& model,
			const mathUtils::Mat4& viewProjMatrix,
			const mathUtils::Mat4& lightViewProjMatrix,
			const mathUtils::Vec3& cameraPosition,
			const mathUtils::Vec3& cameraForward,
			const mathUtils::Vec4& baseColor,
			std::uint32_t paletteOffset,
			std::uint32_t boneCount) -> SkinnedPerDrawConstants
			{
				SkinnedPerDrawConstants constants{};
				const mathUtils::Mat4 viewProjT = mathUtils::Transpose(viewProjMatrix);
				const mathUtils::Mat4 dirVP_T = mathUtils::Transpose(lightViewProjMatrix);
				const mathUtils::Mat4 modelT = mathUtils::Transpose(model);
				std::memcpy(constants.uViewProj.data(), mathUtils::ValuePtr(viewProjT), sizeof(float) * 16);
				std::memcpy(constants.uLightViewProj.data(), mathUtils::ValuePtr(dirVP_T), sizeof(float) * 16);
				std::memcpy(constants.uModel.data(), mathUtils::ValuePtr(modelT), sizeof(float) * 16);
				constants.uCameraAmbient = { cameraPosition.x, cameraPosition.y, cameraPosition.z, 0.0f };
				constants.uCameraForward = { cameraForward.x, cameraForward.y, cameraForward.z, 0.0f };
				constants.uBaseColor = { baseColor.x, baseColor.y, baseColor.z, baseColor.w };
				constants.uMaterialFlags = { 0.0f, 0.0f, 0.0f, AsFloatBits(0u) };
				constants.uPbrParams = { 0.0f, 1.0f, 1.0f, 0.0f };
				constants.uCounts = { 0.0f, 0.0f, 0.0f, 0.0f };
				constants.uShadowBias = { 0.0f, 0.0f, 0.0f, 0.0f };
				constants.uEnvProbeBoxMin = { 0.0f, 0.0f, 0.0f, 0.0f };
				constants.uEnvProbeBoxMax = { 0.0f, 0.0f, 0.0f, 0.0f };
				constants.uSkinning = { static_cast<float>(paletteOffset), static_cast<float>(boneCount), 0.0f, 0.0f };
				return constants;
			};

		const std::size_t count = std::min(group.size(), starts.size());
		for (std::size_t i = 0; i < count; ++i)
		{
			const EditorSelectionDraw& s = group[i];
			if ((!s.isSkinned && !s.mesh) || (s.isSkinned && !s.skinnedMesh))
			{
				continue;
			}

			const std::uint32_t startInstance = starts[i];
			if (s.isSkinned)
			{
				BindEditorSelectionSkinnedGeometry(*s.skinnedMesh);
			}
			else
			{
				if (!highlightInstanceBuffer_)
				{
					continue;
				}
				BindEditorSelectionGeometry(*s.mesh);
			}

			if (psoOutline_)
			{
				ctx.commandList.SetStencilRef(kEditorOutlineStencilRef);
				ctx.commandList.SetState(outlineMarkState_);
				ctx.commandList.BindPipeline(psoHighlight_);

				if (s.isSkinned)
				{
					ctx.commandList.BindPipeline(psoHighlightSkinned_ ? psoHighlightSkinned_ : psoHighlight_);
					SkinnedPerDrawConstants markConstants = MakeEditorSelectionSkinnedConstants(
						s.model,
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 1.0f, 1.0f, 0.0f),
						s.paletteOffset,
						s.boneCount);
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &markConstants, 1 }));
					ctx.commandList.DrawIndexed(s.skinnedMesh->indexCount, s.skinnedMesh->indexType, 0, 0);
				}
				else
				{
					PerBatchConstants markConstants = MakeEditorSelectionConstants(
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 1.0f, 1.0f, 0.0f));
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &markConstants, 1 }));
					ctx.commandList.DrawIndexed(s.mesh->indexCount, s.mesh->indexType, 0, 0, 1, startInstance);
				}

				ctx.commandList.SetState(outlineState_);
				ctx.commandList.BindPipeline(s.isSkinned ? (psoOutlineSkinned_ ? psoOutlineSkinned_ : psoOutline_) : psoOutline_);

				if (s.isSkinned)
				{
					SkinnedPerDrawConstants outlineConstants = MakeEditorSelectionSkinnedConstants(
						s.model,
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 0.72f, 0.10f, 0.95f),
						s.paletteOffset,
						s.boneCount);
					outlineConstants.uPbrParams = { s.outlineWorldOffset, 0.0f, 0.0f, 0.0f };
					outlineConstants.uCounts = { 0.0f, 0.0f, 0.0f, 3.0f };
					outlineConstants.uShadowBias = {
						extent.width ? (1.0f / static_cast<float>(extent.width)) : 0.0f,
						extent.height ? (1.0f / static_cast<float>(extent.height)) : 0.0f,
						0.0f,
						0.0f
					};
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &outlineConstants, 1 }));
					ctx.commandList.DrawIndexed(s.skinnedMesh->indexCount, s.skinnedMesh->indexType, 0, 0);
				}
				else
				{
					PerBatchConstants outlineConstants = MakeEditorSelectionConstants(
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 0.72f, 0.10f, 0.95f));
					outlineConstants.uPbrParams = { s.outlineWorldOffset, 0.0f, 0.0f, 0.0f };
					outlineConstants.uCounts = { 0.0f, 0.0f, 0.0f, 3.0f };
					outlineConstants.uShadowBias = {
						extent.width ? (1.0f / static_cast<float>(extent.width)) : 0.0f,
						extent.height ? (1.0f / static_cast<float>(extent.height)) : 0.0f,
						0.0f,
						0.0f
					};
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &outlineConstants, 1 }));
					ctx.commandList.DrawIndexed(s.mesh->indexCount, s.mesh->indexType, 0, 0, 1, startInstance);
				}

				ctx.commandList.SetState(outlineMarkState_);
				ctx.commandList.BindPipeline(s.isSkinned ? (psoHighlightSkinned_ ? psoHighlightSkinned_ : psoHighlight_) : psoHighlight_);
				ctx.commandList.SetStencilRef(0u);
				if (s.isSkinned)
				{
					SkinnedPerDrawConstants clearMarkConstants = MakeEditorSelectionSkinnedConstants(
						s.model,
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 1.0f, 1.0f, 0.0f),
						s.paletteOffset,
						s.boneCount);
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &clearMarkConstants, 1 }));
					ctx.commandList.DrawIndexed(s.skinnedMesh->indexCount, s.skinnedMesh->indexType, 0, 0);
				}
				else
				{
					PerBatchConstants clearMarkConstants = MakeEditorSelectionConstants(
						viewProj,
						dirLightViewProj,
						camPosLocal,
						camFLocal,
						mathUtils::Vec4(1.0f, 1.0f, 1.0f, 0.0f));
					ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &clearMarkConstants, 1 }));
					ctx.commandList.DrawIndexed(s.mesh->indexCount, s.mesh->indexType, 0, 0, 1, startInstance);
				}
				ctx.commandList.SetStencilRef(0u);
			}

			ctx.commandList.SetState(highlightState_);
			ctx.commandList.BindPipeline(s.isSkinned ? (psoHighlightSkinned_ ? psoHighlightSkinned_ : psoHighlight_) : psoHighlight_);

			if (s.isSkinned)
			{
				SkinnedPerDrawConstants highlightConstants = MakeEditorSelectionSkinnedConstants(
					s.model,
					viewProj,
					dirLightViewProj,
					camPosLocal,
					camFLocal,
					highlightColor,
					s.paletteOffset,
					s.boneCount);
				ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &highlightConstants, 1 }));
				ctx.commandList.DrawIndexed(s.skinnedMesh->indexCount, s.skinnedMesh->indexType, 0, 0);
			}
			else
			{
				PerBatchConstants highlightConstants = MakeEditorSelectionConstants(
					viewProj,
					dirLightViewProj,
					camPosLocal,
					camFLocal,
					highlightColor);
				ctx.commandList.SetConstants(0, std::as_bytes(std::span{ &highlightConstants, 1 }));
				ctx.commandList.DrawIndexed(s.mesh->indexCount, s.mesh->indexType, 0, 0, 1, startInstance);
			}
			ctx.commandList.SetState(restoreState);
		}
	};
#include "DirectX12Renderer_RenderFrame_04_SharedMaterialEnvHelpers.inl"
#include "DirectX12Renderer_RenderFrame_04_SharedPerBatchConstantsHelpers.inl"