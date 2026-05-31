module;

#include <cstddef>

export module core:common_DX12_Structs;

import std;
import :mesh;
import :skinned_mesh;
import :rhi;
import :math_utils;
import :scene;
import :render_graph;

export namespace rendern
{
	// multiple Spot/Point shadow casters (DX12). Keep small caps for now.
	constexpr std::uint32_t kMaxSpotShadows = 4;
	constexpr std::uint32_t kMaxPointShadows = 4;

	struct DeferredReflectionProbeGpu
	{
		std::array<float, 4> boxMin;
		std::array<float, 4> boxMax;
		std::array<float, 4> capturePosDesc;
	};
	// StructuredBuffer<ReflectionProbeGpu> in DeferredLighting_dx12.hlsl is three float4 rows.
	static_assert(std::is_standard_layout_v<DeferredReflectionProbeGpu>);
	static_assert(std::is_trivially_copyable_v<DeferredReflectionProbeGpu>);
	static_assert(sizeof(DeferredReflectionProbeGpu) == 48);
	static_assert(offsetof(DeferredReflectionProbeGpu, boxMin) == 0);
	static_assert(offsetof(DeferredReflectionProbeGpu, boxMax) == 16);
	static_assert(offsetof(DeferredReflectionProbeGpu, capturePosDesc) == 32);

	struct alignas(16) GPULight
	{
		std::array<float, 4> p0{}; // pos.xyz, type
		std::array<float, 4> p1{}; // dir.xyz (FROM light), intensity
		std::array<float, 4> p2{}; // color.rgb, range
		std::array<float, 4> p3{}; // cosInner, cosOuter, attLin, attQuad
	};
	// StructuredBuffer<GPULight> is consumed by deferred and reflection-capture shaders as four float4 rows.
	static_assert(std::is_standard_layout_v<GPULight>);
	static_assert(std::is_trivially_copyable_v<GPULight>);
	static_assert(alignof(GPULight) == 16);
	static_assert(sizeof(GPULight) == 64);
	static_assert(offsetof(GPULight, p0) == 0);
	static_assert(offsetof(GPULight, p1) == 16);
	static_assert(offsetof(GPULight, p2) == 32);
	static_assert(offsetof(GPULight, p3) == 48);

	struct alignas(16) ReflectionCaptureConstants
	{
		// 6 matrices * 4 rows = 24 float4 rows = 96 floats = 384 bytes
		std::array<float, 16u * 6u> uFaceViewProj{};   // row-major rows for each face (4 rows per face)

		std::array<float, 4> uCapturePosAmbient{};     // xyz + ambientStrength
		std::array<float, 4> uBaseColor{};             // rgba
		std::array<float, 4> uParams{};                // x=lightCount, y=flagsBits(asfloat), z,w unused
	};
	static_assert(std::is_standard_layout_v<ReflectionCaptureConstants>);
	static_assert(std::is_trivially_copyable_v<ReflectionCaptureConstants>);
	static_assert(alignof(ReflectionCaptureConstants) == 16);
	static_assert(sizeof(ReflectionCaptureConstants) == 432);
	static_assert(sizeof(ReflectionCaptureConstants) <= 512);
	static_assert(offsetof(ReflectionCaptureConstants, uCapturePosAmbient) == 384);
	static_assert(offsetof(ReflectionCaptureConstants, uBaseColor) == 400);
	static_assert(offsetof(ReflectionCaptureConstants, uParams) == 416);
	struct alignas(16) ReflectionCaptureFaceConstants
	{
		std::array<float, 16> uViewProj{};             // 4 rows
		std::array<float, 4>  uCapturePosAmbient{};    // xyz + ambientStrength
		std::array<float, 4>  uBaseColor{};            // rgba
		std::array<float, 4>  uParams{};               // x=lightCount, y=flagsBits(asfloat), z,w unused
	};
	static_assert(std::is_standard_layout_v<ReflectionCaptureFaceConstants>);
	static_assert(std::is_trivially_copyable_v<ReflectionCaptureFaceConstants>);
	static_assert(alignof(ReflectionCaptureFaceConstants) == 16);
	static_assert(sizeof(ReflectionCaptureFaceConstants) == 112);
	static_assert(sizeof(ReflectionCaptureFaceConstants) <= 512);
	static_assert(offsetof(ReflectionCaptureFaceConstants, uCapturePosAmbient) == 64);
	static_assert(offsetof(ReflectionCaptureFaceConstants, uBaseColor) == 80);
	static_assert(offsetof(ReflectionCaptureFaceConstants, uParams) == 96);

	struct alignas(16) SingleMatrixPassConstants
	{
		std::array<float, 16> uLightViewProj{};
	};

	struct alignas(16) PointShadowCubeConstants
	{
		std::array<float, 16 * 6> uFaceViewProj{};
		std::array<float, 4> uLightPosRange{};
		std::array<float, 4> uMisc{};
	};

	struct alignas(16) PointShadowFaceConstants
	{
		std::array<float, 16> uFaceViewProj{};
		std::array<float, 4> uLightPosRange{};
		std::array<float, 4> uMisc{};
	};

	struct InstanceData
	{
		mathUtils::Vec4 i0; // column 0 of model
		mathUtils::Vec4 i1; // column 1
		mathUtils::Vec4 i2; // column 2
		mathUtils::Vec4 i3; // column 3
	};
	// Vertex input slot 1 maps these columns to TEXCOORD1..4 in DX12 shaders.
	static_assert(std::is_standard_layout_v<InstanceData>);
	static_assert(std::is_trivially_copyable_v<InstanceData>);
	static_assert(alignof(InstanceData) == 16);
	static_assert(sizeof(InstanceData) == 64);
	static_assert(offsetof(InstanceData, i0) == 0);
	static_assert(offsetof(InstanceData, i1) == 16);
	static_assert(offsetof(InstanceData, i2) == 32);
	static_assert(offsetof(InstanceData, i3) == 48);

	struct ParticleInstanceData
	{
		mathUtils::Vec4 centerSize; // xyz = world center, w = size
		mathUtils::Vec4 color;      // rgba
		mathUtils::Vec4 params0;    // x = rotationRad, yzw unused for now
		mathUtils::Vec4 params1{};  // reserved
	};
	static_assert(std::is_standard_layout_v<ParticleInstanceData>);
	static_assert(std::is_trivially_copyable_v<ParticleInstanceData>);
	static_assert(alignof(ParticleInstanceData) == 16);
	static_assert(sizeof(ParticleInstanceData) == 64);
	static_assert(offsetof(ParticleInstanceData, centerSize) == 0);
	static_assert(offsetof(ParticleInstanceData, color) == 16);
	static_assert(offsetof(ParticleInstanceData, params0) == 32);
	static_assert(offsetof(ParticleInstanceData, params1) == 48);

	struct alignas(16) ParticleConstants
	{
		std::array<float, 16> uViewProj{};
		std::array<float, 4> uCameraRight{};
		std::array<float, 4> uCameraUp{};
	};
	static_assert(std::is_standard_layout_v<ParticleConstants>);
	static_assert(std::is_trivially_copyable_v<ParticleConstants>);
	static_assert(alignof(ParticleConstants) == 16);
	static_assert(sizeof(ParticleConstants) == 96);
	static_assert(sizeof(ParticleConstants) <= 96);
	static_assert(offsetof(ParticleConstants, uCameraRight) == 64);
	static_assert(offsetof(ParticleConstants, uCameraUp) == 80);

	// shadow metadata for Spot/Point arrays (bound as StructuredBuffer at t11).
	// We pack indices/bias as floats to keep the struct simple across compilers.
	struct alignas(16) ShadowDataSB
	{
		// ---------------- Directional CSM (atlas) ----------------
		// Cascades are packed into a single D32 atlas.
		// Layout: [C0|C1|C2] horizontally, each tile is dirTileSize x dirTileSize.
		// dirVPRows: cascadeCount * 4 rows.
		std::array<mathUtils::Vec4, 12> dirVPRows{}; // 3 cascades * 4 rows
		// dirSplits = { split1, split2, split3 (max shadow distance), fadeFraction }
		mathUtils::Vec4 dirSplits{};
		// dirInfo = { invAtlasW, invAtlasH, invTileRes, cascadeCount }
		mathUtils::Vec4 dirInfo{};

		// Spot view-projection matrices as ROWS (4 matrices * 4 rows = 16 float4).
		std::array<mathUtils::Vec4, kMaxSpotShadows * 4> spotVPRows{};
		// spotInfo[i] = { lightIndexBits, bias, 0, 0 }
		std::array<mathUtils::Vec4, kMaxSpotShadows>     spotInfo{};

		// pointPosRange[i] = { pos.x, pos.y, pos.z, range }
		std::array<mathUtils::Vec4, kMaxPointShadows>    pointPosRange{};
		// pointInfo[i] = { lightIndexBits, bias, 0, 0 }
		std::array<mathUtils::Vec4, kMaxPointShadows>    pointInfo{};
	};
	// Mirrors struct ShadowDataSB in DeferredLighting_dx12.hlsl; every member is a float4 row/array.
	static_assert(std::is_standard_layout_v<ShadowDataSB>);
	static_assert(std::is_trivially_copyable_v<ShadowDataSB>);
	static_assert(alignof(ShadowDataSB) == 16);
	static_assert(sizeof(ShadowDataSB) == 672);
	static_assert((sizeof(ShadowDataSB) % 16) == 0);
	static_assert(offsetof(ShadowDataSB, dirVPRows) == 0);
	static_assert(offsetof(ShadowDataSB, dirSplits) == 192);
	static_assert(offsetof(ShadowDataSB, dirInfo) == 208);
	static_assert(offsetof(ShadowDataSB, spotVPRows) == 224);
	static_assert(offsetof(ShadowDataSB, spotInfo) == 480);
	static_assert(offsetof(ShadowDataSB, pointPosRange) == 544);
	static_assert(offsetof(ShadowDataSB, pointInfo) == 608);

	// ---------------- Spot/Point shadow maps (arrays) ----------------
	struct SpotShadowRec
	{
		renderGraph::RGTextureHandle tex{};
		mathUtils::Mat4 viewProj{};
		std::uint32_t lightIndex{ 0 };
	};

	struct PointShadowRec
	{
		renderGraph::RGTextureHandle cube{};
		renderGraph::RGTextureHandle depthTmp{};
		mathUtils::Vec3 pos{};
		float range{ 10.0f };;
		std::uint32_t lightIndex{ 0 };
	};

	struct alignas(16) ShadowConstants
	{
		std::array<float, 16> uMVP{}; // lightProj * lightView * model
	};

	struct ShadowBatch
	{
		const rendern::MeshRHI* mesh{};
		std::uint32_t instanceOffset{ 0 }; // in combinedInstances[]
		std::uint32_t instanceCount{ 0 };
	};

	struct TransparentDraw
	{
		const rendern::MeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::uint32_t instanceOffset{ 0 }; // absolute offset in combined instance buffer
		float dist2{ 0.0f };               // for sorting (bigger first)
	};

	struct PlanarMirrorDraw
	{
		const rendern::MeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::uint32_t instanceOffset{ 0 }; // absolute offset in combined instance buffer
		mathUtils::Vec3 planePoint{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 planeNormal{ 0.0f, 1.0f, 0.0f };
	};

	struct TransparentTemp
	{
		const rendern::MeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::uint32_t localInstanceOffset{};
		float dist2{};
	};

	struct alignas(16) SkyboxConstants
	{
		std::array<float, 16> uViewProj{};
	};

	struct ReflectionProbeRuntime
	{
		int ownerDrawItem = -1;
		mathUtils::Vec3 capturePos{};
		bool dirty = true;
		bool hasLastPos = false;
		mathUtils::Vec3 lastPos{};
		rhi::TextureHandle cube{};              // raw capture cube (mip0 written by capture pass)
		rhi::TextureHandle prefilteredCube{};   // final sampled cube with roughness-prefiltered mip chain
		rhi::TextureHandle depthCube{};
		rhi::TextureDescIndex cubeDescIndex{};  // descriptor for prefilteredCube
	};

	struct BatchKey
	{
		const rendern::MeshRHI* mesh{};
		// Material key (must be immutable during RenderFrame)
		rhi::TextureDescIndex albedoDescIndex{};
		rhi::TextureDescIndex normalDescIndex{};
		rhi::TextureDescIndex metalnessDescIndex{};
		rhi::TextureDescIndex roughnessDescIndex{};
		rhi::TextureDescIndex aoDescIndex{};
		rhi::TextureDescIndex emissiveDescIndex{};
		rhi::TextureDescIndex specularDescIndex{};
		rhi::TextureDescIndex glossDescIndex{};
		rhi::TextureDescIndex heightDescIndex{};

		mathUtils::Vec4 baseColor{};
		float shadowBias{}; // texels

		// PBR scalars (used if corresponding texture isn't provided)
		float metallic{};
		float roughness{};
		float ao{};
		float emissiveStrength{};
		float heightScale{};

		// Legacy (kept for batching stability with OpenGL fallback / old materials)
		float shininess{};
		float specStrength{};

		std::uint32_t permBits{};
		std::uint32_t envSource{};
		int reflectionProbeIndex = -1;
	};

	struct BatchKeyEq
	{
		bool operator()(const BatchKey& lhs, const BatchKey& rhs) const noexcept
		{
			return lhs.mesh == rhs.mesh &&
				lhs.permBits == rhs.permBits &&
				lhs.envSource == rhs.envSource &&
				lhs.reflectionProbeIndex == rhs.reflectionProbeIndex &&
				lhs.albedoDescIndex == rhs.albedoDescIndex &&
				lhs.normalDescIndex == rhs.normalDescIndex &&
				lhs.metalnessDescIndex == rhs.metalnessDescIndex &&
				lhs.roughnessDescIndex == rhs.roughnessDescIndex &&
				lhs.aoDescIndex == rhs.aoDescIndex &&
				lhs.emissiveDescIndex == rhs.emissiveDescIndex &&
				lhs.specularDescIndex == rhs.specularDescIndex &&
				lhs.glossDescIndex == rhs.glossDescIndex &&
				lhs.heightDescIndex == rhs.heightDescIndex &&
				lhs.baseColor == rhs.baseColor &&
				lhs.shadowBias == rhs.shadowBias &&
				lhs.metallic == rhs.metallic &&
				lhs.roughness == rhs.roughness &&
				lhs.ao == rhs.ao &&
				lhs.emissiveStrength == rhs.emissiveStrength &&
				lhs.heightScale == rhs.heightScale &&
				lhs.shininess == rhs.shininess &&
				lhs.specStrength == rhs.specStrength;
		}
	};

	struct BatchTemp
	{
		MaterialParams material{};
		MaterialHandle materialHandle{};
		int reflectionProbeIndex = -1;
		std::vector<InstanceData> inst;
	};

	struct Batch
	{
		const rendern::MeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		std::uint32_t instanceOffset = 0; // in instances[]
		std::uint32_t instanceCount = 0;
		int reflectionProbeIndex = -1;
	};

	struct SkinnedOpaqueDraw
	{
		const rendern::SkinnedMeshRHI* mesh{};
		MaterialParams material{};
		MaterialHandle materialHandle{};
		mathUtils::Mat4 model{ 1.0f };
		std::uint32_t firstIndex{ 0 };
		std::uint32_t indexCount{ 0 };
		std::uint32_t paletteOffset{ 0 };
		std::uint32_t boneCount{ 0 };
		int sourceSkinnedDrawIndex{ -1 };
	};

	struct alignas(16) PerBatchConstants
	{
		std::array<float, 16> uViewProj{};
		std::array<float, 16> uLightViewProj{};
		std::array<float, 4>  uCameraAmbient{}; // xyz + ambient
		std::array<float, 4>  uCameraForward{}; // xyz + 0
		std::array<float, 4>  uBaseColor{};     // fallback baseColor

		// shininess, specStrength, materialShadowBiasTexels, flagsBits
		std::array<float, 4>  uMaterialFlags{};


		// metallic, roughness, ao, emissiveStrength
		std::array<float, 4>  uPbrParams{};
		// lightCount, spotShadowCount, pointShadowCount, unused
		std::array<float, 4>  uCounts{};

		// dirBaseTexels, spotBaseTexels, pointBaseTexels, slopeScaleTexels
		std::array<float, 4>  uShadowBias{};

		// xyz = probe capture position, w = box half-extent (world units).
		// Used for parallax-corrected (box-projected) reflection probes when sampling dynamic env cubemaps.
		std::array<float, 4> uEnvProbeBoxMin{};
		std::array<float, 4> uEnvProbeBoxMax{};

		// Bindless / auxiliary material texture descriptor indices.
		// x=albedo, y=normal, z=metalness, w=roughness
		std::array<float, 4> uTexIndices0{};
		// x=ao, y=emissive, z=specular, w=gloss
		std::array<float, 4> uTexIndices1{};
		// x=height, yzw unused
		std::array<float, 4> uTexIndices2{};
		// x=heightScale, y=minSteps, z=maxSteps, w=reserved
		std::array<float, 4> uParallaxParams{};
	};
	// Mirrors cbuffer PerBatch in DeferredGBuffer*_dx12.hlsl; material texture indices must stay on float4 rows.
	static_assert(std::is_standard_layout_v<PerBatchConstants>);
	static_assert(std::is_trivially_copyable_v<PerBatchConstants>);
	static_assert(alignof(PerBatchConstants) == 16);
	static_assert(sizeof(PerBatchConstants) == 336);
	static_assert(offsetof(PerBatchConstants, uViewProj) == 0);
	static_assert(offsetof(PerBatchConstants, uLightViewProj) == 64);
	static_assert(offsetof(PerBatchConstants, uCameraAmbient) == 128);
	static_assert(offsetof(PerBatchConstants, uCameraForward) == 144);
	static_assert(offsetof(PerBatchConstants, uBaseColor) == 160);
	static_assert(offsetof(PerBatchConstants, uMaterialFlags) == 176);
	static_assert(offsetof(PerBatchConstants, uPbrParams) == 192);
	static_assert(offsetof(PerBatchConstants, uCounts) == 208);
	static_assert(offsetof(PerBatchConstants, uShadowBias) == 224);
	static_assert(offsetof(PerBatchConstants, uEnvProbeBoxMin) == 240);
	static_assert(offsetof(PerBatchConstants, uEnvProbeBoxMax) == 256);
	static_assert(offsetof(PerBatchConstants, uTexIndices0) == 272);
	static_assert(offsetof(PerBatchConstants, uTexIndices1) == 288);
	static_assert(offsetof(PerBatchConstants, uTexIndices2) == 304);
	static_assert(offsetof(PerBatchConstants, uParallaxParams) == 320);

	struct alignas(16) SkinnedPerDrawConstants
	{
		std::array<float, 16> uViewProj{};
		std::array<float, 16> uLightViewProj{};
		std::array<float, 4>  uCameraAmbient{};
		std::array<float, 4>  uCameraForward{};
		std::array<float, 4>  uBaseColor{};
		std::array<float, 4>  uMaterialFlags{};
		std::array<float, 4>  uPbrParams{};
		std::array<float, 4>  uCounts{};
		std::array<float, 4>  uShadowBias{};
		std::array<float, 4>  uEnvProbeBoxMin{};
		std::array<float, 4>  uEnvProbeBoxMax{};
		std::array<float, 4>  uTexIndices0{};
		std::array<float, 4>  uTexIndices1{};
		std::array<float, 4>  uTexIndices2{};
		std::array<float, 4>  uParallaxParams{};
		std::array<float, 16> uModel{};
		std::array<float, 4>  uSkinning{};
	};
	// Skinned GBuffer constants extend PerBatch with model and skinning rows consumed by HLSL.
	static_assert(std::is_standard_layout_v<SkinnedPerDrawConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedPerDrawConstants>);
	static_assert(alignof(SkinnedPerDrawConstants) == 16);
	static_assert(sizeof(SkinnedPerDrawConstants) == 416);
	static_assert(offsetof(SkinnedPerDrawConstants, uModel) == 336);
	static_assert(offsetof(SkinnedPerDrawConstants, uSkinning) == 400);

	struct alignas(16) SkinnedSingleMatrixPassConstants
	{
		std::array<float, 16> uLightViewProj{};
		std::array<float, 16> uModel{};
		std::array<float, 4> uSkinning{};
	};
	static_assert(std::is_standard_layout_v<SkinnedSingleMatrixPassConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedSingleMatrixPassConstants>);
	static_assert(alignof(SkinnedSingleMatrixPassConstants) == 16);
	static_assert(sizeof(SkinnedSingleMatrixPassConstants) == 144);
	static_assert(offsetof(SkinnedSingleMatrixPassConstants, uModel) == 64);
	static_assert(offsetof(SkinnedSingleMatrixPassConstants, uSkinning) == 128);

	struct alignas(16) SkinnedPointShadowCubeConstants
	{
		std::array<float, 16 * 6> uFaceViewProj{};
		std::array<float, 4> uLightPosRange{};
		std::array<float, 4> uMisc{};
		std::array<float, 16> uModel{};
		std::array<float, 4> uSkinning{};
	};
	static_assert(std::is_standard_layout_v<SkinnedPointShadowCubeConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedPointShadowCubeConstants>);
	static_assert(alignof(SkinnedPointShadowCubeConstants) == 16);
	static_assert(sizeof(SkinnedPointShadowCubeConstants) == 496);
	static_assert(offsetof(SkinnedPointShadowCubeConstants, uLightPosRange) == 384);
	static_assert(offsetof(SkinnedPointShadowCubeConstants, uMisc) == 400);
	static_assert(offsetof(SkinnedPointShadowCubeConstants, uModel) == 416);
	static_assert(offsetof(SkinnedPointShadowCubeConstants, uSkinning) == 480);

	struct alignas(16) SkinnedPointShadowFaceConstants
	{
		std::array<float, 16> uFaceViewProj{};
		std::array<float, 4> uLightPosRange{};
		std::array<float, 4> uMisc{};
		std::array<float, 16> uModel{};
		std::array<float, 4> uSkinning{};
	};
	static_assert(std::is_standard_layout_v<SkinnedPointShadowFaceConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedPointShadowFaceConstants>);
	static_assert(alignof(SkinnedPointShadowFaceConstants) == 16);
	static_assert(sizeof(SkinnedPointShadowFaceConstants) == 176);
	static_assert(offsetof(SkinnedPointShadowFaceConstants, uLightPosRange) == 64);
	static_assert(offsetof(SkinnedPointShadowFaceConstants, uMisc) == 80);
	static_assert(offsetof(SkinnedPointShadowFaceConstants, uModel) == 96);
	static_assert(offsetof(SkinnedPointShadowFaceConstants, uSkinning) == 160);

	struct alignas(16) SkinnedReflectionCaptureConstants
	{
		std::array<float, 16u * 6u> uFaceViewProj{};
		std::array<float, 4> uCapturePosAmbient{};
		std::array<float, 4> uBaseColor{};
		std::array<float, 4> uParams{};
		std::array<float, 16> uModel{};
		std::array<float, 4> uSkinning{};
	};
	static_assert(std::is_standard_layout_v<SkinnedReflectionCaptureConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedReflectionCaptureConstants>);
	static_assert(alignof(SkinnedReflectionCaptureConstants) == 16);
	static_assert(sizeof(SkinnedReflectionCaptureConstants) == 512);
	static_assert(offsetof(SkinnedReflectionCaptureConstants, uModel) == 432);
	static_assert(offsetof(SkinnedReflectionCaptureConstants, uSkinning) == 496);

	struct alignas(16) SkinnedReflectionCaptureFaceConstants
	{
		std::array<float, 16> uViewProj{};
		std::array<float, 4>  uCapturePosAmbient{};
		std::array<float, 4>  uBaseColor{};
		std::array<float, 4>  uParams{};
		std::array<float, 16> uModel{};
		std::array<float, 4>  uSkinning{};
	};
	static_assert(std::is_standard_layout_v<SkinnedReflectionCaptureFaceConstants>);
	static_assert(std::is_trivially_copyable_v<SkinnedReflectionCaptureFaceConstants>);
	static_assert(alignof(SkinnedReflectionCaptureFaceConstants) == 16);
	static_assert(sizeof(SkinnedReflectionCaptureFaceConstants) == 192);
	static_assert(offsetof(SkinnedReflectionCaptureFaceConstants, uModel) == 112);
	static_assert(offsetof(SkinnedReflectionCaptureFaceConstants, uSkinning) == 176);

	struct EditorSelectionDraw
	{
		const rendern::MeshRHI* mesh{};
		const rendern::SkinnedMeshRHI* skinnedMesh{};
		InstanceData instance{};
		mathUtils::Mat4 model{ 1.0f };
		std::uint32_t paletteOffset{ 0 };
		std::uint32_t boneCount{ 0 };
		float outlineWorldOffset = 0.025f;
		bool isTransparent = false;
		bool isSkinned = false;
	};

	struct SSAOConstants
	{
		std::array<float, 16> uInvViewProj{};
		mathUtils::Vec4 uParams{};  // radius, bias, strength, power
		mathUtils::Vec4 uInvSize{}; // 1/w, 1/h, 0,0
	};
	static_assert(sizeof(SSAOConstants) % 16 == 0);

	struct SSAOBlurConstants
	{
		mathUtils::Vec4 uInvSize{};
		mathUtils::Vec4 uParams{}; // depthThreshold
	};
	static_assert(sizeof(SSAOBlurConstants) % 16 == 0);

	struct alignas(16) FogConstants
	{
		std::array<float, 16> uInvViewProj{};
		std::array<float, 4> uCameraPos{}; // xyz + pad
		std::array<float, 4> uFogParams{}; // start, end, density, mode
		std::array<float, 4> uFogColor{};  // rgb + enabled(0/1)
	};
	static_assert(sizeof(FogConstants) % 16 == 0);

	struct alignas(16) BloomExtractConstants
	{
		mathUtils::Vec4 uInvSourceSize{}; // 1/w, 1/h
		mathUtils::Vec4 uParams{}; // threshold, softKnee, clampMax, pad
	};
	static_assert(sizeof(BloomExtractConstants) % 16 == 0);

	struct alignas(16) BloomBlurConstants
	{
		mathUtils::Vec4 uInvSourceSize{}; // 1/w, 1/h
		mathUtils::Vec4 uDirection{};     // x/y axis * radius
	};
	static_assert(sizeof(BloomBlurConstants) % 16 == 0);

	struct alignas(16) BloomCompositeConstants
	{
		mathUtils::Vec4 uParams{}; // intensity
	};
	static_assert(sizeof(BloomCompositeConstants) % 16 == 0);

	struct alignas(16) ToneMapConstants
	{
		mathUtils::Vec4 uParams{}; // exposure, mode, gamma, enableHDR
	};
	static_assert(sizeof(ToneMapConstants) % 16 == 0);

	struct alignas(16) FXAAConstants
	{
		mathUtils::Vec4 uInvSourceSize{}; // x=1/width, y=1/height
		mathUtils::Vec4 uParams{};        // x=subpix, y=edgeThreshold, z=edgeThresholdMin, w=pad
	};
	static_assert(sizeof(FXAAConstants) % 16 == 0);

	struct FrameCameraData
	{
		mathUtils::Mat4 proj{ 1.0f };
		mathUtils::Mat4 view{ 1.0f };
		mathUtils::Mat4 viewProj{ 1.0f };
		mathUtils::Mat4 invViewProj{ 1.0f };
		mathUtils::Mat4 invViewProjT{ 1.0f };
		mathUtils::Vec3 camPos{ 0.0f, 0.0f, 0.0f };
		mathUtils::Vec3 camForward{ 0.0f, 0.0f, -1.0f };
	};

	struct ResolvedMaterialEnvBinding
	{
		rhi::TextureDescIndex descIndex{};
		rhi::TextureHandle arrayTexture{};
		bool usingReflectionProbeEnv = false;
	};

	struct alignas(16) DeferredLightingConstants
	{
		std::array<float, 16> uInvViewProj{};      // transpose(invViewProj)
		std::array<float, 4>  uCameraPosAmbient{}; // xyz + ambientStrength
		std::array<float, 4>  uCameraForward{};    // xyz + pad
		std::array<float, 4>  uShadowBias{};       // x=dirBaseBiasTexels, y=spotBaseBiasTexels, z=pointBaseBiasTexels, w=slopeScaleTexels
		std::array<float, 4>  uCounts{};           // x = lightCount, y = spotShadowCount, z = pointShadowCount, w = activeReflectionProbeCount
	};
	// Mirrors cbuffer Deferred in DeferredLighting_dx12.hlsl.
	static_assert(std::is_standard_layout_v<DeferredLightingConstants>);
	static_assert(std::is_trivially_copyable_v<DeferredLightingConstants>);
	static_assert(alignof(DeferredLightingConstants) == 16);
	static_assert(sizeof(DeferredLightingConstants) == 128);
	static_assert(offsetof(DeferredLightingConstants, uInvViewProj) == 0);
	static_assert(offsetof(DeferredLightingConstants, uCameraPosAmbient) == 64);
	static_assert(offsetof(DeferredLightingConstants, uCameraForward) == 80);
	static_assert(offsetof(DeferredLightingConstants, uShadowBias) == 96);
	static_assert(offsetof(DeferredLightingConstants, uCounts) == 112);

	struct ParticleDrawBatch
	{
		rhi::TextureDescIndex textureDescIndex{ 0 };
		std::uint32_t instanceOffset{ 0 };
		std::uint32_t instanceCount{ 0 };
	};
}