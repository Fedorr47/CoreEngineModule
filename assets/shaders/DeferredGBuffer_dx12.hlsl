// DeferredGBuffer_dx12.hlsl
// SM6 bindless material sampling (space1).

SamplerState gLinear : register(s0);
SamplerComparisonState gShadowCmp : register(s1);
SamplerState gPointClamp : register(s2);
SamplerState gLinearClamp : register(s3);

// Bindless SRV heap view (space1) for material textures.
Texture2D gBindlessTex[] : register(t0, space1);

cbuffer PerBatch : register(b0)
{
	float4x4 uViewProj;
	float4x4 uLightViewProj;
	float4 uCameraAmbient;
	float4 uCameraForward;
	float4 uBaseColor;
	float4 uMaterialFlags;
	float4 uPbrParams;
	float4 uCounts;
	float4 uShadowBias;
	float4 uEnvProbeBoxMin;
	float4 uEnvProbeBoxMax;

    // x=albedo, y=normal, z=metalness, w=roughness
	float4 uTexIndices0;
    // x=ao, y=emissive, z=specular, w=gloss
	float4 uTexIndices1;
    // x=height, yzw unused
	float4 uTexIndices2;
    // x=heightScale, y=minSteps, z=maxSteps, w=reserved
	float4 uParallaxParams;
};

// we output a tiny "env selector" target so deferred lighting can choose betwee
// skybox IBL and reflection-capture IBL per material.
// Encoding (RGBA8_UNORM):
//   r = envSource (0.0 = Skybox, 1.0 = ReflectionCapture)
//   g = reflection cubemap descriptor byte 0 (LSB)
//   b = reflection cubemap descriptor byte 1
//   a = reflection cubemap descriptor byte 2
// Encoding (RGBA8_UNORM):
// r = envSource (0.0 = Skybox, 1.0 = ReflectionCapture)
// g/b/a = packed 24-bit reflection cubemap descriptor index

struct VSIn
{
	float3 pos : POSITION;
	float3 nrm : NORMAL;
	float2 uv : TEXCOORD0;
	float4 tangent : TANGENT;

    // Instance matrix columns (slot1): TEXCOORD1..4
	float4 i0 : TEXCOORD1;
	float4 i1 : TEXCOORD2;
	float4 i2 : TEXCOORD3;
	float4 i3 : TEXCOORD4;
};

struct VSOut
{
	float4 svPos : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 nrmW : TEXCOORD1;
	float2 uv : TEXCOORD2;
	float3 tangentW : TEXCOORD3;
	float3 bitangentW : TEXCOORD4;
};

VSOut VS_GBuffer(VSIn IN)
{
	VSOut OUT;

	const float3 worldPos = IN.pos.x * IN.i0.xyz + IN.pos.y * IN.i1.xyz + IN.pos.z * IN.i2.xyz + IN.i3.xyz;
	const float3 nrmW = normalize(IN.nrm.x * IN.i0.xyz + IN.nrm.y * IN.i1.xyz + IN.nrm.z * IN.i2.xyz);
	float3 tangentW = normalize(IN.tangent.x * IN.i0.xyz + IN.tangent.y * IN.i1.xyz + IN.tangent.z * IN.i2.xyz);
	tangentW = normalize(tangentW - nrmW * dot(nrmW, tangentW));
	const float3 bitangentW = normalize(cross(nrmW, tangentW)) * IN.tangent.w;

	OUT.worldPos = worldPos;
	OUT.nrmW = nrmW;
	OUT.tangentW = tangentW;
	OUT.bitangentW = bitangentW;
	OUT.uv = IN.uv;

	OUT.svPos = mul(float4(worldPos, 1.0f), uViewProj);
	return OUT;
}

// Derived tangent frame fallback for meshes that still have no valid tangents.
float3x3 CotangentFrame(float3 N, float3 p, float2 uv)
{
	const float3 dp1 = ddx(p);
	const float3 dp2 = ddy(p);
	const float2 duv1 = ddx(uv);
	const float2 duv2 = ddy(uv);

	const float3 dp2perp = cross(dp2, N);
	const float3 dp1perp = cross(N, dp1);
	const float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	const float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	const float invMax = rsqrt(max(dot(T, T), dot(B, B)));
	return float3x3(T * invMax, B * invMax, N);
}


float3x3 BuildShadingTBN(float3 N, float3 tangentW, float3 bitangentW, float3 worldPos, float2 uv)
{
	const float tangentLen2 = dot(tangentW, tangentW);
	const float bitangentLen2 = dot(bitangentW, bitangentW);
	if (tangentLen2 > 1e-8f && bitangentLen2 > 1e-8f)
	{
		float3 T = normalize(tangentW - N * dot(N, tangentW));
		float3 B = normalize(bitangentW - N * dot(N, bitangentW));
		return float3x3(T, B, N);
	}

	return CotangentFrame(N, worldPos, uv);
}

float SampleHeightBindless(uint heightIdx, float2 uv)
{
	return gBindlessTex[NonUniformResourceIndex(heightIdx)].SampleLevel(gLinear, uv, 0.0f).r;
}

float2 ApplyParallaxOcclusion(float2 uv, float3 viewDirTS, uint heightIdx, float heightScale, uint minSteps, uint maxSteps)
{
	if (heightIdx == 0u || heightScale <= 1e-5f)
	{
		return uv;
	}

	viewDirTS = normalize(viewDirTS);
	const float vz = abs(viewDirTS.z);
	if (vz <= 1e-4f)
	{
		return uv;
	}

	minSteps = max(minSteps, 1u);
	maxSteps = max(maxSteps, minSteps);

	const float numLayers = lerp((float) maxSteps, (float) minSteps, saturate(vz));
	const float layerDepth = 1.0f / numLayers;
	const float2 deltaUV = -(viewDirTS.xy / max(vz, 0.05f)) * heightScale / numLayers;

	float2 currentUV = uv;
	float currentLayerDepth = 0.0f;
	float currentDepthMap = 1.0f - SampleHeightBindless(heightIdx, currentUV);

	[loop]
	for (uint step = 0u; step < maxSteps; ++step)
	{
		if ((float) step >= numLayers || currentLayerDepth >= currentDepthMap)
		{
			break;
		}

		currentUV += deltaUV;
		currentLayerDepth += layerDepth;
		currentDepthMap = 1.0f - SampleHeightBindless(heightIdx, currentUV);
	}

	const float2 prevUV = currentUV - deltaUV;
	const float prevLayerDepth = currentLayerDepth - layerDepth;
	const float prevDepthMap = 1.0f - SampleHeightBindless(heightIdx, prevUV);

	const float after = currentDepthMap - currentLayerDepth;
	const float before = prevDepthMap - prevLayerDepth;
	const float weight = saturate(after / max(after - before, 1e-5f));
	return lerp(currentUV, prevUV, weight);
}


struct PSOut
{
	float4 rt0 : SV_Target0; // albedo.rgb, roughness
	float4 rt1 : SV_Target1; // normal.xyz (encoded), metalness
	float4 rt2 : SV_Target2; // emissive.rgb, ao
	float4 rt3 : SV_Target3; // env selector
};

// Flags must match C++ (DirectX12Renderer_RenderFrame_04_MainPass.inl)
static const uint kFlagUseTex = 1u << 0;
static const uint kFlagUseNormal = 1u << 2;
static const uint kFlagUseMetalTex = 1u << 3;
static const uint kFlagUseRoughTex = 1u << 4;
static const uint kFlagUseAOTex = 1u << 5;
static const uint kFlagUseEmissive = 1u << 6;

PSOut PS_GBuffer(VSOut IN)
{
	PSOut OUT;

	const uint flags = asuint(uMaterialFlags.w);

	const uint albedoIdx = (uint) uTexIndices0.x;
	const uint normalIdx = (uint) uTexIndices0.y;
	const uint metalIdx = (uint) uTexIndices0.z;
	const uint roughIdx = (uint) uTexIndices0.w;

	const uint aoIdx = (uint) uTexIndices1.x;
	const uint emissiveIdx = (uint) uTexIndices1.y;
	const uint heightIdx = (uint) uTexIndices2.x;

	const float3 baseN = normalize(IN.nrmW);
	const float3x3 TBN = BuildShadingTBN(baseN, IN.tangentW, IN.bitangentW, IN.worldPos, IN.uv);
	const float3 V = normalize(uCameraAmbient.xyz - IN.worldPos);
	const float3 viewDirTS = float3(
		dot(V, TBN[0]),
		dot(V, TBN[1]),
		dot(V, TBN[2]));
	const float2 uvParallax = ApplyParallaxOcclusion(
		IN.uv,
		viewDirTS,
		heightIdx,
		max(uParallaxParams.x, 0.0f),
		(uint) max(uParallaxParams.y, 1.0f),
		(uint) max(uParallaxParams.z, uParallaxParams.y));

	float3 albedo = uBaseColor.rgb;
	if ((flags & kFlagUseTex) != 0u && albedoIdx != 0u)
	{
        // Non-uniform indexing (bindless)
		float4 t = gBindlessTex[NonUniformResourceIndex(albedoIdx)].Sample(gLinear, uvParallax);
		albedo *= t.rgb;
	}

	float metallic = saturate(uPbrParams.x);
	if ((flags & kFlagUseMetalTex) != 0u && metalIdx != 0u)
	{
		metallic = gBindlessTex[NonUniformResourceIndex(metalIdx)].Sample(gLinear, uvParallax).r;
	}

	float roughness = saturate(uPbrParams.y);
	if ((flags & kFlagUseRoughTex) != 0u && roughIdx != 0u)
	{
		roughness = gBindlessTex[NonUniformResourceIndex(roughIdx)].Sample(gLinear, uvParallax).r;
	}

	float ao = saturate(uPbrParams.z);
	if ((flags & kFlagUseAOTex) != 0u && aoIdx != 0u)
	{
		ao = gBindlessTex[NonUniformResourceIndex(aoIdx)].Sample(gLinear, uvParallax).r;
	}

	float3 N = baseN;
	if ((flags & kFlagUseNormal) != 0u && normalIdx != 0u)
	{
		float3 nTS = gBindlessTex[NonUniformResourceIndex(normalIdx)].Sample(gLinear, uvParallax).xyz * 2.0f - 1.0f;
		N = normalize(mul(nTS, TBN));
	}

	float emissiveStrength = max(uPbrParams.w, 0.0f);
	float3 emissive = 0.0f;
	if ((flags & kFlagUseEmissive) != 0u && emissiveIdx != 0u)
	{
		emissive = gBindlessTex[NonUniformResourceIndex(emissiveIdx)].Sample(gLinear, uvParallax).rgb * emissiveStrength;
	}

    // Encode to gbuffer
	OUT.rt0 = float4(albedo, roughness);
	OUT.rt1 = float4(N * 0.5f + 0.5f, metallic);
	OUT.rt2 = float4(emissive, ao);
	
	// CPU packs:
	// uEnvProbeBoxMin.w = envSource (0 = Skybox, 1 = ReflectionCapture)
	// uEnvProbeBoxMax.w = normalized reflection probe index for deferred resolve
	const float envSource = saturate(uEnvProbeBoxMin.w);
	OUT.rt3 = float4(envSource, saturate(uEnvProbeBoxMax.w), 0.0f, 0.0f);
	
	return OUT;
}