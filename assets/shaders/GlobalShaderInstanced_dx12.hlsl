// GlobalShaderInstanced_dx12.hlsl
// MainPass instancing: per-vertex attributes (slot0) + per-instance model matrix (slot1).
//
// Root signature expectations:
//  b0 : PerBatch constants
//  t0 : Albedo texture
//  t1 : Shadow map
//  t2 : StructuredBuffer lights
//  s0 : Linear sampler
//  s1 : Comparison sampler (shadow)

cbuffer PerBatch : register(b0)
{
    float4x4 uViewProj;
    float4x4 uLightViewProj;
    float4   uCameraAmbient;
    float4   uBaseColor;
    float4   uMaterialFlags;
    float4   uCounts;
};

Texture2D<float4> AlbedoTex : register(t0);
Texture2D<float>  ShadowMap : register(t1);
StructuredBuffer<float4> LightsRaw : register(t2);

SamplerState           LinearSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

static const uint LIGHT_DIRECTIONAL = 0;
static const uint LIGHT_POINT       = 1;
static const uint LIGHT_SPOT        = 2;

struct GPULight
{
    float4 p0;
    float4 p1;
    float4 p2;
    float4 p3;
};

GPULight LoadLight(uint i)
{
    GPULight L;
    uint base = i * 4;
    L.p0 = LightsRaw[base + 0];
    L.p1 = LightsRaw[base + 1];
    L.p2 = LightsRaw[base + 2];
    L.p3 = LightsRaw[base + 3];
    return L;
}

struct VSIn
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD0;
    float4 i0     : TEXCOORD1;
    float4 i1     : TEXCOORD2;
    float4 i2     : TEXCOORD3;
    float4 i3     : TEXCOORD4;
};

struct VSOut
{
    float4 posH      : SV_POSITION;
    float3 worldPos  : TEXCOORD0;
    float3 worldN    : TEXCOORD1;
    float2 uv        : TEXCOORD2;
    float4 lightPosH : TEXCOORD3;
};

VSOut VSMain(VSIn IN)
{
    VSOut OUT;
    float4x4 model = float4x4(IN.i0, IN.i1, IN.i2, IN.i3);
    float4 wpos = mul(model, float4(IN.pos, 1));
    float3 wN   = mul((float3x3)model, IN.normal);
    OUT.worldPos = wpos.xyz;
    OUT.worldN   = normalize(wN);
    OUT.uv       = IN.uv;
    OUT.posH      = mul(uViewProj, wpos);
    OUT.lightPosH = mul(uLightViewProj, wpos);
    return OUT;
}

float ComputeShadow(float4 lightPosH, float shadowBias)
{
    float3 proj = lightPosH.xyz / max(lightPosH.w, 1e-6);
    if (proj.z < 0 || proj.z > 1) return 1;
    float2 uv = float2(proj.x * 0.5 + 0.5, -proj.y * 0.5 + 0.5);
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) return 1;
    return ShadowMap.SampleCmpLevelZero(ShadowSampler, uv, proj.z - shadowBias);
}

float Attenuation(float dist, float attLin, float attQuad)
{
    return 1.0 / max(1.0 + attLin * dist + attQuad * dist * dist, 1e-4);
}

float3 EvalDirectional(GPULight L, float3 N, float3 V, float shininess, float specStrength)
{
    float3 Ldir = normalize(-L.p1.xyz);
    float ndl = max(dot(N, Ldir), 0);
    float3 H = normalize(Ldir + V);
    float spec = pow(max(dot(N, H), 0), shininess) * specStrength;
    return (ndl + spec) * (L.p2.xyz * L.p1.w);
}

float3 EvalPoint(GPULight L, float3 P, float3 N, float3 V, float shininess, float specStrength)
{
    float3 toL = L.p0.xyz - P;
    float d = length(toL);
    if (d <= 1e-4 || d > L.p2.w) return 0;
    float3 Ldir = toL / d;
    float ndl = max(dot(N, Ldir), 0);
    float3 H = normalize(Ldir + V);
    float spec = pow(max(dot(N, H), 0), shininess) * specStrength;
    float att = Attenuation(d, L.p3.z, L.p3.w);
    return (ndl + spec) * (L.p2.xyz * L.p1.w) * att;
}

float3 EvalSpot(GPULight L, float3 P, float3 N, float3 V, float shininess, float specStrength)
{
    float3 toL = L.p0.xyz - P;
    float d = length(toL);
    if (d <= 1e-4 || d > L.p2.w) return 0;
    float3 Ldir = toL / d;

    float3 lightToPoint = normalize(P - L.p0.xyz);
    float3 spotDir = normalize(L.p1.xyz);
    float cosA = dot(lightToPoint, spotDir);
    float spot = saturate((cosA - L.p3.y) / max(L.p3.x - L.p3.y, 1e-4));
    if (spot <= 0) return 0;

    float ndl = max(dot(N, Ldir), 0);
    float3 H = normalize(Ldir + V);
    float spec = pow(max(dot(N, H), 0), shininess) * specStrength;
    float att = Attenuation(d, L.p3.z, L.p3.w);
    return (ndl + spec) * (L.p2.xyz * L.p1.w) * att * spot;
}

struct PSOut { float4 color : SV_Target0; };

PSOut PSMain(VSOut IN)
{
    PSOut OUT;
    float3 camPos = uCameraAmbient.xyz;
    float ambient = uCameraAmbient.w;
    float shininess = uMaterialFlags.x;
    float specStrength = uMaterialFlags.y;
    float shadowBias = uMaterialFlags.z;
    uint flags = asuint(uMaterialFlags.w);

    bool useTex = (flags & 1u) != 0;
    bool useShadow = (flags & 2u) != 0;

    float4 albedo = uBaseColor;
    if (useTex) albedo *= AlbedoTex.Sample(LinearSampler, IN.uv);

    float3 N = normalize(IN.worldN);
    float3 V = normalize(camPos - IN.worldPos);
    float shadow = useShadow ? ComputeShadow(IN.lightPosH, shadowBias) : 1;

    float3 lit = albedo.rgb * ambient;
    uint lightCount = (uint)uCounts.x;

    [loop] for (uint i = 0; i < lightCount; ++i)
    {
        GPULight L = LoadLight(i);
        uint type = (uint)L.p0.w;
        float3 c = 0;
        if (type == LIGHT_DIRECTIONAL) c = EvalDirectional(L, N, V, shininess, specStrength);
        else if (type == LIGHT_POINT)  c = EvalPoint(L, IN.worldPos, N, V, shininess, specStrength);
        else if (type == LIGHT_SPOT)   c = EvalSpot(L, IN.worldPos, N, V, shininess, specStrength);
        lit += albedo.rgb * c * shadow;
    }

    OUT.color = float4(lit, albedo.a);
    return OUT;
}