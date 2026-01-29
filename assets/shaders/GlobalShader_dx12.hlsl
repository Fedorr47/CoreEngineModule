// Mesh_dx12.hlsl
// b0: PerDrawMainConstants (240 bytes)
// t0: albedo
// t1: shadow map (R32_FLOAT SRV)
// t2: StructuredBuffer lights

struct VSIn
{
    float3 posL : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 posH : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 worldN : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 lightPosH : TEXCOORD3; // light clip position
};

cbuffer PerDraw : register(b0)
{
    float4x4 uMVP; // 64
    float4x4 uLightMVP; // 64

    float4 uModelRow0; // 48
    float4 uModelRow1;
    float4 uModelRow2;

    float4 uCameraAmbient; // cam.xyz, ambientStrength
    float4 uBaseColor; // rgba
    float4 uMaterialFlags; // shininess, specStrength, shadowBias, flagsPacked(float)
    float4 uCounts; // x = lightCount
};

Texture2D gAlbedo : register(t0);
Texture2D<float> gShadowMap : register(t1);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct GPULight
{
    float4 p0; // pos.xyz, type (0=dir,1=point,2=spot)
    float4 p1; // dir.xyz (FROM light), intensity
    float4 p2; // color.rgb, range
    float4 p3; // cosInner, cosOuter, attLin, attQuad
};

StructuredBuffer<GPULight> gLights : register(t2);

static const uint LIGHT_TYPE_DIR = 0u;
static const uint LIGHT_TYPE_POINT = 1u;
static const uint LIGHT_TYPE_SPOT = 2u;

static const uint FLAG_USE_TEX = 1u << 0;
static const uint FLAG_USE_SHADOW = 1u << 1;

float3 MulModelPos(float3 pL)
{
    float4 p = float4(pL, 1.0);
    return float3(dot(uModelRow0, p), dot(uModelRow1, p), dot(uModelRow2, p));
}

float3 MulModelNormal(float3 nL)
{
    // Approx: ignore inverse-transpose (ok for uniform scale)
    float3x3 M = float3x3(uModelRow0.xyz, uModelRow1.xyz, uModelRow2.xyz);
    return normalize(mul(M, nL));
}

VSOut VSMain(VSIn vin)
{
    VSOut o;
    o.posH = mul(uMVP, float4(vin.posL, 1.0)); // IMPORTANT: M * v
    o.lightPosH = mul(uLightMVP, float4(vin.posL, 1.0)); // IMPORTANT: M * v
    o.worldPos = MulModelPos(vin.posL);
    o.worldN = MulModelNormal(vin.normal);
    o.uv = vin.uv;
    return o;
}

float ShadowPCF3x3(float4 lightPosH, float bias)
{
    // Project
    float3 proj = lightPosH.xyz / max(lightPosH.w, 1e-6);

    // Outside clip? -> lit
    if (proj.z <= 0.0 || proj.z >= 1.0)
        return 1.0;

    // NDC -> UV (D3D viewport flips Y)
    float2 uv;
    uv.x = proj.x * 0.5 + 0.5;
    uv.y = -proj.y * 0.5 + 0.5;

    // Outside shadow map -> lit
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 1.0;

    uint w, h;
    gShadowMap.GetDimensions(w, h);
    float2 texel = 1.0 / float2(max(w, 1u), max(h, 1u));

    float depthRef = saturate(proj.z - bias);

    float sum = 0.0;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 uvTap = uv + float2(x, y) * texel;
            sum += gShadowMap.SampleCmpLevelZero(gShadowSampler, uvTap, depthRef);
        }
    }
    return sum / 9.0;
}

float AttenuationPoint(float dist, float range, float attLin, float attQuad)
{
    // Range falloff
    float rangeAtt = 1.0;
    if (range > 0.0)
    {
        rangeAtt = saturate(1.0 - dist / range);
        rangeAtt *= rangeAtt;
    }

    float denom = 1.0 + attLin * dist + attQuad * dist * dist;
    return rangeAtt / max(denom, 1e-4);
}

float SpotCone(float3 L_fromPointToLight, float3 spotDirFromLight, float cosInner, float cosOuter)
{
    // Need direction from light -> point:
    float3 lightToPoint = normalize(-L_fromPointToLight);
    float cd = dot(lightToPoint, normalize(spotDirFromLight)); // compare with FROM-light dir

    // Smoothstep between outer/inner
    float t = saturate((cd - cosOuter) / max(cosInner - cosOuter, 1e-4));
    return t;
}

float4 PSMain(VSOut pin) : SV_Target
{
    // Flags/material
    uint flags = (uint) (uMaterialFlags.w + 0.5);
    float shininess = uMaterialFlags.x;
    float specStrength = uMaterialFlags.y;
    float shadowBias = uMaterialFlags.z;

    float3 camPos = uCameraAmbient.xyz;
    float ambientK = uCameraAmbient.w;

    float4 base = uBaseColor;
    float4 tex = gAlbedo.Sample(gSampler, pin.uv);

    float4 albedo = base;
    if ((flags & FLAG_USE_TEX) != 0u)
        albedo *= tex;

    float3 N = normalize(pin.worldN);
    float3 V = normalize(camPos - pin.worldPos);

    // Shadow factor (only for direct lights)
    float shadow = 1.0;
    if ((flags & FLAG_USE_SHADOW) != 0u)
        shadow = ShadowPCF3x3(pin.lightPosH, shadowBias);

    float3 ambient = albedo.rgb * ambientK;

    float3 direct = 0.0;

    uint lightCount = (uint) (uCounts.x + 0.5);
    lightCount = min(lightCount, 64u);

    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        GPULight Ld = gLights[i];

        uint type = (uint) (Ld.p0.w + 0.5);

        float3 lightColor = Ld.p2.rgb;
        float intensity = Ld.p1.w;

        float3 L; // direction from point -> light
        float att = 1.0;

        if (type == LIGHT_TYPE_DIR)
        {
            // Stored dir is FROM light (light -> scene), so point->light is opposite
            L = normalize(-Ld.p1.xyz);
            att = 1.0;
        }
        else
        {
            float3 lightPos = Ld.p0.xyz;
            float3 toLight = lightPos - pin.worldPos;
            float dist = length(toLight);
            L = (dist > 1e-6) ? (toLight / dist) : float3(0, 1, 0);

            float range = Ld.p2.w;
            float attLin = Ld.p3.z;
            float attQuad = Ld.p3.w;

            att = AttenuationPoint(dist, range, attLin, attQuad);

            if (type == LIGHT_TYPE_SPOT)
            {
                float cosInner = Ld.p3.x;
                float cosOuter = Ld.p3.y;
                float cone = SpotCone(L, Ld.p1.xyz, cosInner, cosOuter);
                att *= cone;
            }
        }

        float NdotL = saturate(dot(N, L));
        float3 diffuse = albedo.rgb * NdotL;

        // Blinn-Phong
        float3 H = normalize(L + V);
        float spec = pow(saturate(dot(N, H)), max(shininess, 1.0));
        float3 specular = specStrength * spec * lightColor;

        // Directional light uses shadows; you can decide differently per light if you want.
        float shadowK = (type == LIGHT_TYPE_DIR) ? shadow : 1.0;

        direct += (diffuse * lightColor + specular) * (intensity * att * shadowK);
    }

    float3 outRgb = ambient + direct;
    return float4(outRgb, albedo.a);
}
