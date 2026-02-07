// GlobalShaderInstanced_dx12.hlsl
// NOTE: Save as UTF-8 without BOM to keep FXC happy.

// Samplers (must match root signature)
SamplerState gLinear : register(s0);
SamplerComparisonState gShadowCmp : register(s1);
SamplerState gPointClamp : register(s2);

// Textures / SRVs (must match root signature tables per register)
Texture2D gAlbedo : register(t0);

// Directional shadow map (depth SRV)
Texture2D<float> gDirShadow : register(t1);

// Lights buffer
struct GPULight
{
    float4 p0; // pos.xyz, type
    float4 p1; // dir.xyz, intensity
    float4 p2; // color.rgb, range
    float4 p3; // cosInner, cosOuter, attLin, attQuad
};
StructuredBuffer<GPULight> gLights : register(t2);

// Spot shadow maps (depth) - NO ARRAYS (root sig uses 1-descriptor tables per tN)
Texture2D<float> gSpotShadow0 : register(t3);
Texture2D<float> gSpotShadow1 : register(t4);
Texture2D<float> gSpotShadow2 : register(t5);
Texture2D<float> gSpotShadow3 : register(t6);

// Point distance cubemaps (normalized distance)
TextureCube<float> gPointShadow0 : register(t7);
TextureCube<float> gPointShadow1 : register(t8);
TextureCube<float> gPointShadow2 : register(t9);
TextureCube<float> gPointShadow3 : register(t10);

// Shadow metadata buffer (one element)
struct ShadowDataSB
{
    float4 spotVPRows[16]; // 4 matrices * 4 rows (row-major rows)
    float4 spotInfo[4]; // { lightIndexBits, 0, extraBiasTexels, 0 }

    float4 pointPosRange[4]; // { pos.xyz, range }
    float4 pointInfo[4]; // { lightIndexBits, 0, extraBiasTexels, 0 }
};
StructuredBuffer<ShadowDataSB> gShadowData : register(t11);

// DX12 PBR maps
Texture2D gNormal : register(t12);
Texture2D gMetalness : register(t13);
Texture2D gRoughness : register(t14);
Texture2D gAO : register(t15);
Texture2D gEmissive : register(t16);
TextureCube gEnv : register(t17);

cbuffer PerBatchCB : register(b0)
{
    float4x4 uViewProj;
    float4x4 uLightViewProj; // directional shadow VP (rows)

    float4 uCameraAmbient; // xyz + ambientStrength
    float4 uBaseColor; // fallback baseColor

    // x=shininess, y=specStrength, z=materialShadowBiasTexels, w=flags (bitpacked as float)
    float4 uMaterialFlags;


    // x=metallic, y=roughness, z=ao, w=emissiveStrength
    float4 uPbrParams;
    // x=lightCount, y=spotShadowCount, z=pointShadowCount, w=unused
    float4 uCounts;

    // x=dirBaseBiasTexels, y=spotBaseBiasTexels, z=pointBaseBiasTexels, w=slopeScaleTexels
    float4 uShadowBias;
};

// Flags (must match C++)
static const uint FLAG_USE_TEX = 1u << 0;
static const uint FLAG_USE_SHADOW = 1u << 1;
static const uint FLAG_USE_NORMAL = 1u << 2;
static const uint FLAG_USE_METAL_TEX = 1u << 3;
static const uint FLAG_USE_ROUGH_TEX = 1u << 4;
static const uint FLAG_USE_AO_TEX = 1u << 5;
static const uint FLAG_USE_EMISSIVE_TEX = 1u << 6;
static const uint FLAG_USE_ENV = 1u << 7;

static const uint kMaxSpotShadows = 4;
static const uint kMaxPointShadows = 4;

// Light types (must match rendern::LightType in C++)
static const int LIGHT_DIR   = 0;
static const int LIGHT_POINT = 1;
static const int LIGHT_SPOT  = 2;

// Helpers
float4x4 MakeMatRows(float4 r0, float4 r1, float4 r2, float4 r3)
{
    return float4x4(r0, r1, r2, r3);
}

float SmoothStep01(float t)
{
    t = saturate(t);
    return t * t * (3.0f - 2.0f * t);
}

static const float PI = 3.14159265359f;

float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * Pow5(1.0f - saturate(cosTheta));
}

float DistributionGGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / max(PI * denom * denom, 1e-6f);
}

float GeometrySchlickGGX(float NdotX, float k)
{
    return NdotX / max(NdotX * (1.0f - k) + k, 1e-6f);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float ggxV = GeometrySchlickGGX(NdotV, k);
    float ggxL = GeometrySchlickGGX(NdotL, k);
    return ggxV * ggxL;
}

float3 GetNormalMapped(float3 N, float3 worldPos, float2 uv)
{
    // Derivative-based TBN (no explicit tangents in the mesh).
    float3 dp1 = ddx(worldPos);
    float3 dp2 = ddy(worldPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 T = dp1 * duv2.y - dp2 * duv1.y;
    float3 B = -dp1 * duv2.x + dp2 * duv1.x;

    T = normalize(T - N * dot(N, T));
    B = normalize(B - N * dot(N, B));

    float3 nTS = gNormal.Sample(gLinear, uv).xyz * 2.0f - 1.0f;
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(nTS, TBN));
}

float SlopeScaleTerm(float NdotL)
{
    NdotL = max(NdotL, 1e-4f);
    return sqrt(max(1.0f - NdotL * NdotL, 0.0f)) / NdotL;
}

float ComputeBiasTexels(float NdotL,
                        float baseBiasTexels,
                        float slopeScaleTexels,
                        float materialBiasTexels,
                        float extraBiasTexels)
{
    return baseBiasTexels
         + SlopeScaleTerm(NdotL) * slopeScaleTexels
         + materialBiasTexels
         + extraBiasTexels;
}

int FindSpotShadowSlot(uint lightIndex, uint spotShadowCount)
{
    [unroll]
    for (uint s = 0; s < kMaxSpotShadows; ++s)
    {
        if (s >= spotShadowCount)
            break;
        uint stored = asuint(gShadowData[0].spotInfo[s].x);
        if (stored == lightIndex)
            return (int) s;
    }
    return -1;
}

int FindPointShadowSlot(uint lightIndex, uint pointShadowCount)
{
    [unroll]
    for (uint s = 0; s < kMaxPointShadows; ++s)
    {
        if (s >= pointShadowCount)
            break;
        uint stored = asuint(gShadowData[0].pointInfo[s].x);
        if (stored == lightIndex)
            return (int) s;
    }
    return -1;
}

// Vertex IO
struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;

    // Instance matrix rows in TEXCOORD1..4
    float4 i0 : TEXCOORD1;
    float4 i1 : TEXCOORD2;
    float4 i2 : TEXCOORD3;
    float4 i3 : TEXCOORD4;
};

struct VSOut
{
    float4 posH : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 nrmW : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 shadowPos : TEXCOORD3; // directional shadow clip
};

VSOut VSMain(VSIn IN)
{
    VSOut OUT;

    float4x4 model = MakeMatRows(IN.i0, IN.i1, IN.i2, IN.i3);

    float4 world = mul(float4(IN.pos, 1.0f), model);
    OUT.worldPos = world.xyz;

    float3 nrmW = mul(float4(IN.nrm, 0.0f), model).xyz;
    OUT.nrmW = normalize(nrmW);

    OUT.uv = IN.uv;
    OUT.posH = mul(world, uViewProj);

    OUT.shadowPos = mul(world, uLightViewProj);
    return OUT;
}

// Shadow sampling
float Shadow2D(Texture2D<float> shadowMap, float4 shadowClip, float biasTexels)
{
    float3 p = shadowClip.xyz / max(shadowClip.w, 1e-6f);
        
    // Only reject invalid depth. For XY we rely on BORDER addressing on the comparison sampler,
    // so PCF near the frustum edge fades naturally instead of producing a hard cut.
	if (p.z < 0.0f || p.z > 1.0f)
	{
		return 1.0f;
	}

    // NDC -> UV (note: flip Y)
    float2 uv = float2(p.x, -p.y) * 0.5f + 0.5f;
    
	uint w, h;
	shadowMap.GetDimensions(w, h);
	float2 texel = 1.0f / float2(max(w, 1u), max(h, 1u));
    
	float biasDepth = biasTexels * max(texel.x, texel.y);
	float z = p.z - biasDepth;
    
        // 3x3 PCF
	float s = 0.0f;
    [unroll] for (int y = -1; y <= 1; ++y)
	{
            [unroll]
		for (int x = -1; x <= 1; ++x)
		{
			s += shadowMap.SampleCmpLevelZero(gShadowCmp, uv + texel * float2(x, y), z);
		}
	}
    
	float shadow = s / 9.0f;
    
        // Edge guard-band: smoothly fade out the shadow near the shadow-map boundary to avoid a visible seam
        // when the spotlight still contributes light but the shadow frustum ends.
	float edge = min(min(uv.x, uv.y), min(1.0f - uv.x, 1.0f - uv.y));
	float fade = saturate(edge / (2.0f * max(texel.x, texel.y))); // ~2 texels
    
	return lerp(1.0f, shadow, fade);
    
 }

// Wrapper that matches the CPU call-site signature (shadowClip, materialBiasTexels, baseBiasTexels, slopeScaleTexels).
// NOTE: slopeScaleTexels is currently ignored here; kept for ABI stability with C++.
float Shadow2D(float4 shadowClip, float materialBiasTexels, float baseBiasTexels, float slopeScaleTexels)
{
    const float biasTexels = materialBiasTexels + baseBiasTexels;
    return Shadow2D(gDirShadow, shadowClip, biasTexels);
}

float ShadowPoint(TextureCube<float> distCube,
                  float3 lightPos, float range,
                  float3 worldPos, float biasTexels)
{
    float3 v = worldPos - lightPos;
    float d = length(v);
    float3 dir = v / max(d, 1e-6f);

    float nd = saturate(d / max(range, 1e-3f));

    uint w, h, levels;
    distCube.GetDimensions(0, w, h, levels);

    // Bias is expressed in "shadow texels" by the CPU. Convert to normalized [0..1] distance.
    const float invRes = 1.0f / float(max(w, h));
    const float biasNorm = biasTexels * invRes;

    const float compare = nd - biasNorm;

    // Manual PCF for distance-cubemap shadows.
    // We can't use a comparison sampler here because the map is R32_FLOAT (distance), not a depth texture.
    // We approximate a 2D kernel on the cubemap face by perturbing the lookup direction in a tangent basis.
    float3 up = (abs(dir.y) < 0.99f) ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, dir));
    float3 B = cross(dir, T);

    // ~1-2 texels in "face space". Tune if you want softer/harder point-light shadow edges.
    const float radius = 1.5f * invRes;

    // Poisson-ish disk (8 taps) + center.
    const float2 taps[8] = {
        float2(-0.326f, -0.406f), float2(-0.840f, -0.074f),
        float2(-0.696f,  0.457f), float2(-0.203f,  0.621f),
        float2( 0.962f, -0.195f), float2( 0.473f, -0.480f),
        float2( 0.519f,  0.767f), float2( 0.185f, -0.893f)
    };

    float lit = 0.0f;
    {
        float stored = distCube.SampleLevel(gPointClamp, dir, 0).r;
        lit += (compare <= stored) ? 1.0f : 0.0f;
    }

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float3 ddir = normalize(dir + (T * taps[i].x + B * taps[i].y) * radius);
        float stored = distCube.SampleLevel(gPointClamp, ddir, 0).r;
        lit += (compare <= stored) ? 1.0f : 0.0f;
    }

    return lit / 9.0f;
}

float SpotShadowFactor(uint slot, ShadowDataSB sd, float3 worldPos, float biasTexels)
{
    if (slot >= 4)
        return 1.0f;

    float4 r0 = sd.spotVPRows[slot * 4 + 0];
    float4 r1 = sd.spotVPRows[slot * 4 + 1];
    float4 r2 = sd.spotVPRows[slot * 4 + 2];
    float4 r3 = sd.spotVPRows[slot * 4 + 3];
    float4x4 VP = float4x4(r0, r1, r2, r3);

    float4 clip = mul(float4(worldPos, 1.0f), VP);

    if (slot == 0)
        return Shadow2D(gSpotShadow0, clip, biasTexels);
    if (slot == 1)
        return Shadow2D(gSpotShadow1, clip, biasTexels);
    if (slot == 2)
        return Shadow2D(gSpotShadow2, clip, biasTexels);
    return Shadow2D(gSpotShadow3, clip, biasTexels);
}

// Wrapper matching CPU call-site signature: (slot, worldPos, materialBiasTexels, baseBiasTexels, slopeScaleTexels)
// NOTE: slopeScaleTexels is currently ignored here; kept for ABI stability with C++.
float SpotShadowFactor(uint slot, float3 worldPos, float materialBiasTexels, float baseBiasTexels, float slopeScaleTexels)
{
    if (slot >= 4)
        return 1.0f;
    ShadowDataSB sd = gShadowData[0];
    const float extraBiasTexels = sd.spotInfo[slot].z;
    const float biasTexels = materialBiasTexels + baseBiasTexels + extraBiasTexels;
    return SpotShadowFactor(slot, sd, worldPos, biasTexels);
}

float PointShadowFactor(uint slot, ShadowDataSB sd, float3 worldPos, float biasTexels)
{
    if (slot >= 4)
        return 1.0f;

    float3 lp = sd.pointPosRange[slot].xyz;
    float range = sd.pointPosRange[slot].w;

    if (slot == 0)
        return ShadowPoint(gPointShadow0, lp, range, worldPos, biasTexels);
    if (slot == 1)
        return ShadowPoint(gPointShadow1, lp, range, worldPos, biasTexels);
    if (slot == 2)
        return ShadowPoint(gPointShadow2, lp, range, worldPos, biasTexels);
    return ShadowPoint(gPointShadow3, lp, range, worldPos, biasTexels);
}

// Wrapper matching CPU call-site signature: (slot, worldPos, materialBiasTexels, baseBiasTexels, slopeScaleTexels)
// NOTE: slopeScaleTexels is currently ignored here; kept for ABI stability with C++.
float PointShadowFactor(uint slot, float3 worldPos, float materialBiasTexels, float baseBiasTexels, float slopeScaleTexels)
{
    if (slot >= 4)
        return 1.0f;
    ShadowDataSB sd = gShadowData[0];
    const float extraBiasTexels = sd.pointInfo[slot].z;
    const float biasTexels = materialBiasTexels + baseBiasTexels + extraBiasTexels;
    return PointShadowFactor(slot, sd, worldPos, biasTexels);
}

// Pixel Shader
float4 PSMain(VSOut IN) : SV_Target0
{
    const uint flags = asuint(uMaterialFlags.w);

    const bool useTex = (flags & FLAG_USE_TEX) != 0;
    const bool useShadow = (flags & FLAG_USE_SHADOW) != 0;
    const bool useNormal = (flags & FLAG_USE_NORMAL) != 0;
    const bool useMetalTex = (flags & FLAG_USE_METAL_TEX) != 0;
    const bool useRoughTex = (flags & FLAG_USE_ROUGH_TEX) != 0;
    const bool useAOTex = (flags & FLAG_USE_AO_TEX) != 0;
    const bool useEmissiveTex = (flags & FLAG_USE_EMISSIVE_TEX) != 0;
    const bool useEnv = (flags & FLAG_USE_ENV) != 0;

    float3 baseColor = uBaseColor.rgb;
    float alphaOut = uBaseColor.a;

    if (useTex)
    {
        const float4 tex = gAlbedo.Sample(gLinear, IN.uv);
        baseColor *= tex.rgb;
        alphaOut *= tex.a;
    }

    float metallic = saturate(uPbrParams.x);
    float roughness = saturate(uPbrParams.y);
    float ao = saturate(uPbrParams.z);
    const float emissiveStrength = max(uPbrParams.w, 0.0f);

    if (useMetalTex)
    {
        metallic *= gMetalness.Sample(gLinear, IN.uv).r;
    }
    if (useRoughTex)
    {
        roughness *= gRoughness.Sample(gLinear, IN.uv).r;
    }
    if (useAOTex)
    {
        ao *= gAO.Sample(gLinear, IN.uv).r;
    }

    roughness = clamp(roughness, 0.04f, 1.0f);

    float3 N = normalize(IN.nrmW);
    if (useNormal)
    {
        N = GetNormalMapped(N, IN.worldPos, IN.uv);
    }

    const float3 V = normalize(uCameraAmbient.xyz - IN.worldPos);
    const float NdotV = max(dot(N, V), 0.0f);

    // Fresnel reflectance at normal incidence
    const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);

    float3 Lo = 0.0f;

    const int lightCount = (int)uCounts.x;
    const int spotShadowCount = (int)uCounts.y;
    const int pointShadowCount = (int)uCounts.z;

    [loop]
    for (int i = 0; i < lightCount; ++i)
    {
        const GPULight Ld = gLights[i];
        const int type = (int)Ld.p0.w;

        float3 L = 0.0f;
        float attenuation = 1.0f;
        float shadowFactor = 1.0f;

        if (type == LIGHT_DIR)
        {
            L = normalize(-Ld.p1.xyz);

            if (useShadow)
            {
                shadowFactor = Shadow2D(IN.shadowPos, uMaterialFlags.z, uShadowBias.x, uShadowBias.w);
            }
        }
        else if (type == LIGHT_POINT)
        {
            const float3 toLight = Ld.p0.xyz - IN.worldPos;
            const float dist = length(toLight);
            if (dist > Ld.p2.w)
            {
                continue;
            }

            L = toLight / max(dist, 1e-6f);

            const float attLin = Ld.p3.z;
            const float attQuad = Ld.p3.w;
            attenuation = 1.0f / max(1.0f + attLin * dist + attQuad * dist * dist, 1e-6f);

            if (useShadow && pointShadowCount > 0)
            {
                // Use first point shadow cubemap (index 0) for now.
                shadowFactor = PointShadowFactor(0, IN.worldPos, uMaterialFlags.z, uShadowBias.z, uShadowBias.w);
            }
        }
        else if (type == LIGHT_SPOT)
        {
            const float3 toLight = Ld.p0.xyz - IN.worldPos;
            const float dist = length(toLight);
            if (dist > Ld.p2.w)
            {
                continue;
            }

            L = toLight / max(dist, 1e-6f);

            // Angular falloff
            const float3 spotDir = normalize(Ld.p1.xyz); // FROM light
            const float cosTheta = dot(-L, spotDir);
            const float cosInner = Ld.p3.x;
            const float cosOuter = Ld.p3.y;
            const float spotT = saturate((cosTheta - cosOuter) / max(cosInner - cosOuter, 1e-5f));
            const float spotAtt = SmoothStep01(spotT);

            const float attLin = Ld.p3.z;
            const float attQuad = Ld.p3.w;
            attenuation = spotAtt / max(1.0f + attLin * dist + attQuad * dist * dist, 1e-6f);

            if (useShadow && spotShadowCount > 0)
            {
                shadowFactor = SpotShadowFactor(0, IN.worldPos, uMaterialFlags.z, uShadowBias.y, uShadowBias.w);
            }
        }

        const float NdotL = saturate(dot(N, L));
        if (NdotL <= 0.0f)
        {
            continue;
        }

        const float3 H = normalize(V + L);
        const float NdotH = saturate(dot(N, H));
        const float VdotH = saturate(dot(V, H));

        const float alphaR = roughness * roughness;
        const float D = DistributionGGX(NdotH, alphaR);
        const float G = GeometrySmith(NdotV, NdotL, roughness);
        const float3 F = FresnelSchlick(VdotH, F0);

        const float3 numerator = D * G * F;
        const float denom = max(4.0f * NdotV * NdotL, 1e-6f);
        const float3 specular = numerator / denom;

        const float3 kS = F;
        const float3 kD = (1.0f - kS) * (1.0f - metallic);
        const float3 diffuse = kD * baseColor / PI;

        const float3 radiance = Ld.p2.xyz * (Ld.p1.w * attenuation);

        Lo += (diffuse + specular) * radiance * NdotL * shadowFactor;
    }

    // Ambient / IBL
    float3 ambient = 0.0f;
    if (useEnv)
    {
        // Assume env has mips (generated on load). Use a conservative mipMax; sampling out of range clamps.
        const float mipMax = 9.0f;

        const float3 R = reflect(-V, N);

        // "Diffuse irradiance" approximation: very blurry env sample.
        const float3 envDiffuse = gEnv.SampleLevel(gLinear, N, mipMax).rgb;

        // Specular IBL approximation: env prefiltered by mip level.
        const float3 envSpec = gEnv.SampleLevel(gLinear, R, roughness * mipMax).rgb;

        const float3 F = FresnelSchlick(NdotV, F0);
        const float3 kS = F;
        const float3 kD = (1.0f - kS) * (1.0f - metallic);

        ambient = (kD * baseColor * envDiffuse) + (envSpec * kS);
        ambient *= (ao * uCameraAmbient.w);
    }
    else
    {
        ambient = baseColor * (ao * uCameraAmbient.w);
    }

    float3 emissive = 0.0f;
    if (useEmissiveTex)
    {
        emissive = gEmissive.Sample(gLinear, IN.uv).rgb * emissiveStrength;
    }

    const float3 color = ambient + Lo + emissive;
    return float4(color, saturate(alphaOut));
}
