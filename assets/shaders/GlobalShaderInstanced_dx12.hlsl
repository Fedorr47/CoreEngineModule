// Resource slots must match DirectX12RHI root signature (t0..t11, s0..s1).

SamplerState gLinear : register(s0);
SamplerComparisonState gShadowCmp : register(s1);

Texture2D gAlbedo : register(t0);

// Directional shadow map (depth)
Texture2D<float> gDirShadow : register(t1);

// Lights buffer (StructuredBuffer<GPULight>)
struct GPULight
{
    float4 p0; // pos.xyz, type
    float4 p1; // dir.xyz, intensity
    float4 p2; // color.rgb, range
    float4 p3; // cosInner, cosOuter, attLin, attQuad
};
StructuredBuffer<GPULight> gLights : register(t2);

// Spot shadow maps (depth)
Texture2D<float> gSpotShadow0 : register(t3);
Texture2D<float> gSpotShadow1 : register(t4);
Texture2D<float> gSpotShadow2 : register(t5);
Texture2D<float> gSpotShadow3 : register(t6);

// Point shadow distance cubemaps (R32_FLOAT)
TextureCube<float> gPointShadow0 : register(t7);
TextureCube<float> gPointShadow1 : register(t8);
TextureCube<float> gPointShadow2 : register(t9);
TextureCube<float> gPointShadow3 : register(t10);

// Shadow metadata buffer (one element).
struct ShadowDataSB
{
    float4 spotVPRows[16]; // 4 matrices * 4 rows
    float4 spotInfo[4]; // { lightIndexBits, bias, 0, 0 }

    float4 pointPosRange[4]; // { pos.xyz, range }
    float4 pointInfo[4]; // { lightIndexBits, bias, 0, 0 }
};
StructuredBuffer<ShadowDataSB> gShadowData : register(t11);

cbuffer PerBatchCB : register(b0)
{
    float4x4 uViewProj;
    float4x4 uLightViewProj; // directional shadow VP
    float4 uCameraAmbient; // xyz + ambient
    float4 uBaseColor; // fallback baseColor
    float4 uMaterialFlags; // shininess, specStrength, shadowBias, flagsBits
    float4 uCounts; // lightCount, spotShadowCount, pointShadowCount, unused
};

// Flags (must match C++).
static const uint FLAG_USE_TEX = 1u << 0;
static const uint FLAG_USE_SHADOW = 1u << 1;

float4x4 MakeMatRows(float4 r0, float4 r1, float4 r2, float4 r3)
{
    return float4x4(r0, r1, r2, r3);
}

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
    float4 shadowPos : TEXCOORD3; // directional shadow
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

float Shadow2D(Texture2D<float> shadowMap, float4 shadowPos, float bias)
{
    float3 p = shadowPos.xyz / max(shadowPos.w, 1e-6f);

    if (p.x < -1.0f || p.x > 1.0f || p.y < -1.0f || p.y > 1.0f || p.z < 0.0f || p.z > 1.0f)
        return 1.0f;

    float2 uv = float2(p.x, -p.y) * 0.5f + 0.5f;

    uint w, h;
    shadowMap.GetDimensions(w, h);
    float2 texel = 1.0f / float2(max(w, 1u), max(h, 1u));

    float z = p.z - bias;

    float s = 0.0f;
    s += shadowMap.SampleCmpLevelZero(gShadowCmp, uv + texel * float2(-0.5f, -0.5f), z);
    s += shadowMap.SampleCmpLevelZero(gShadowCmp, uv + texel * float2(0.5f, -0.5f), z);
    s += shadowMap.SampleCmpLevelZero(gShadowCmp, uv + texel * float2(-0.5f, 0.5f), z);
    s += shadowMap.SampleCmpLevelZero(gShadowCmp, uv + texel * float2(0.5f, 0.5f), z);
    return s * 0.25f;
}

float ShadowPoint(TextureCube<float> distCube, float3 lightPos, float range, float3 worldPos, float bias)
{
    float3 v = worldPos - lightPos;
    float d = length(v);
    float nd = saturate(d / max(range, 1e-3f));

    float3 dir = (d > 1e-5f) ? (v / d) : float3(0, 0, 1);
    float stored = distCube.SampleLevel(gLinear, dir, 0).r;

    return (nd - bias <= stored) ? 1.0f : 0.0f;
}

float SpotShadowFactor(uint si, ShadowDataSB sd, float3 worldPos)
{
    float4 r0 = sd.spotVPRows[si * 4 + 0];
    float4 r1 = sd.spotVPRows[si * 4 + 1];
    float4 r2 = sd.spotVPRows[si * 4 + 2];
    float4 r3 = sd.spotVPRows[si * 4 + 3];
    float4x4 vp = MakeMatRows(r0, r1, r2, r3);

    float4 sp = mul(float4(worldPos, 1.0f), vp);
    float bias = sd.spotInfo[si].y;

    if (si == 0)
        return Shadow2D(gSpotShadow0, sp, bias);
    if (si == 1)
        return Shadow2D(gSpotShadow1, sp, bias);
    if (si == 2)
        return Shadow2D(gSpotShadow2, sp, bias);
    return Shadow2D(gSpotShadow3, sp, bias);
}

float PointShadowFactor(uint pi, ShadowDataSB sd, float3 worldPos)
{
    float3 lp = sd.pointPosRange[pi].xyz;
    float range = sd.pointPosRange[pi].w;
    float bias = sd.pointInfo[pi].y;

    if (pi == 0)
        return ShadowPoint(gPointShadow0, lp, range, worldPos, bias);
    if (pi == 1)
        return ShadowPoint(gPointShadow1, lp, range, worldPos, bias);
    if (pi == 2)
        return ShadowPoint(gPointShadow2, lp, range, worldPos, bias);
    return ShadowPoint(gPointShadow3, lp, range, worldPos, bias);
}

float4 PSMain(VSOut IN) : SV_Target0
{
    const uint flags = asuint(uMaterialFlags.w);
    const bool useTex = (flags & FLAG_USE_TEX) != 0;
    const bool useShadow = (flags & FLAG_USE_SHADOW) != 0;

    float3 base = uBaseColor.rgb;
    if (useTex)
    {
        base *= gAlbedo.Sample(gLinear, IN.uv).rgb;
    }

    float3 N = normalize(IN.nrmW);
    float3 V = normalize(uCameraAmbient.xyz - IN.worldPos);

    const float shininess = uMaterialFlags.x;
    const float specStrength = uMaterialFlags.y;
    const float dirBias = uMaterialFlags.z;

    float3 color = base * uCameraAmbient.w;

    const uint lightCount = (uint) uCounts.x;
    const uint spotShadowCount = (uint) uCounts.y;
    const uint pointShadowCount = (uint) uCounts.z;

    ShadowDataSB sd = gShadowData[0];

    float dirShadow = 1.0f;
    if (useShadow)
    {
        dirShadow = Shadow2D(gDirShadow, IN.shadowPos, dirBias);
    }

    for (uint i = 0; i < lightCount; ++i)
    {
        GPULight Ld = gLights[i];
        const uint type = (uint) Ld.p0.w;

        float3 Lpos = Ld.p0.xyz;
        float3 Ldir = normalize(Ld.p1.xyz);
        float intensity = Ld.p1.w;
        float3 Lcolor = Ld.p2.rgb;
        float range = Ld.p2.w;

        float shadow = 1.0f;

        if (useShadow)
        {
            if (type == 0) // Directional
            {
                shadow = dirShadow;
            }
            else if (type == 2) // Spot
            {
                for (uint si = 0; si < spotShadowCount; ++si)
                {
                    uint idx = asuint(sd.spotInfo[si].x);
                    if (idx == i)
                    {
                        shadow = SpotShadowFactor(si, sd, IN.worldPos);
                        break;
                    }
                }
            }
            else if (type == 1) // Point
            {
                for (uint pi = 0; pi < pointShadowCount; ++pi)
                {
                    uint idx = asuint(sd.pointInfo[pi].x);
                    if (idx == i)
                    {
                        shadow = PointShadowFactor(pi, sd, IN.worldPos);
                        break;
                    }
                }
            }
        }

        float3 L = 0;
        float att = 1.0f;

        if (type == 0)
        {
            // Directional: direction is FROM light towards scene
            L = normalize(-Ldir);
        }
        else
        {
            float3 toL = Lpos - IN.worldPos;
            float dist = length(toL);
            if (dist > range)
                continue;
            L = toL / max(dist, 1e-6f);
            att = saturate(1.0f - dist / max(range, 1e-3f));
        }

        float NdotL = max(dot(N, L), 0.0f);
        float3 diffuse = base * NdotL;

        float3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0f), max(shininess, 1.0f));
        float3 specular = specStrength * spec;

        color += (diffuse + specular) * (Lcolor * intensity * att) * shadow;
    }

    return float4(color, 1.0f);
}

