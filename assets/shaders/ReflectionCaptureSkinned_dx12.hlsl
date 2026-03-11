SamplerState gLinear : register(s0);
Texture2D gAlbedo : register(t0);

#include "SkinningCommon_dx12.hlsli"

struct GPULight
{
    float4 p0; // pos.xyz, type
    float4 p1; // dir.xyz, intensity
    float4 p2; // color.rgb, range
    float4 p3; // cosInner, cosOuter, attLin, attQuad
};
StructuredBuffer<GPULight> gLights : register(t2);

cbuffer ReflectionCaptureFaceCB : register(b0)
{
    float4x4 uViewProj;
    float4 uCapturePosAmbient; // xyz + ambientStrength
    float4 uBaseColor; // rgba
    float4 uParams; // x=lightCount, y=flags(asfloat)
    float4x4 uModel;
    float4 uSkinning; // x=paletteOffset, y=boneCount
};

static const uint FLAG_USE_TEX = 1u << 0;
static const int LIGHT_DIR = 0;
static const int LIGHT_POINT = 1;
static const int LIGHT_SPOT = 2;

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tangent : TANGENT;
    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct VSOut
{
    float4 posH : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 nrmW : TEXCOORD1;
    float2 uv : TEXCOORD2;
};

VSOut VS_ReflectionCapture(VSIn IN)
{
    VSOut OUT;

    const uint paletteOffset = (uint)uSkinning.x;
    const uint boneCount = (uint)uSkinning.y;

    float3 skinnedPos = IN.pos;
    float3 skinnedNrm = IN.nrm;
    float4 skinnedTangent = IN.tangent;
    ApplySkinning(
        paletteOffset,
        boneCount,
        IN.boneIndices,
        IN.boneWeights,
        IN.pos,
        IN.nrm,
        IN.tangent,
        skinnedPos,
        skinnedNrm,
        skinnedTangent);

    const float4 world = mul(float4(skinnedPos, 1.0f), uModel);
    OUT.worldPos = world.xyz;
    OUT.nrmW = normalize(mul(float4(skinnedNrm, 0.0f), uModel).xyz);
    OUT.uv = IN.uv;
    OUT.posH = mul(world, uViewProj);
    return OUT;
}

float3 EvalLights(float3 worldPos, float3 N, float3 baseColor)
{
    const uint lightCount = (uint)uParams.x;
    float3 Lo = 0.0.xxx;

    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        const GPULight L = gLights[i];
        const int type = (int)L.p0.w;
        const float3 lightColor = L.p2.rgb * L.p1.w;

        if (type == LIGHT_DIR)
        {
            const float3 dirFromLight = normalize(L.p1.xyz);
            const float3 Ldir = -dirFromLight;
            Lo += baseColor * lightColor * saturate(dot(N, Ldir));
        }
        else if (type == LIGHT_POINT)
        {
            const float3 lightPos = L.p0.xyz;
            const float3 toLight = lightPos - worldPos;
            const float dist = length(toLight);
            const float range = max(L.p2.w, 1e-3f);
            if (dist < range)
            {
                const float3 Ldir = toLight / max(dist, 1e-3f);
                const float ndl = saturate(dot(N, Ldir));
                const float attLin = L.p3.z;
                const float attQuad = L.p3.w;
                const float att = 1.0f / (1.0f + attLin * dist + attQuad * dist * dist);
                const float fade = saturate(1.0f - (dist / range));
                Lo += baseColor * lightColor * ndl * att * fade;
            }
        }
        else if (type == LIGHT_SPOT)
        {
            const float3 lightPos = L.p0.xyz;
            const float3 toLight = lightPos - worldPos;
            const float dist = length(toLight);
            const float range = max(L.p2.w, 1e-3f);
            if (dist < range)
            {
                const float3 Ldir = toLight / max(dist, 1e-3f);
                const float ndl = saturate(dot(N, Ldir));
                const float3 dirFromLight = normalize(L.p1.xyz);
                const float3 toPointFromLight = normalize(worldPos - lightPos);
                const float cosAng = dot(dirFromLight, toPointFromLight);
                const float cosInner = L.p3.x;
                const float cosOuter = L.p3.y;
                const float spot = saturate((cosAng - cosOuter) / max(cosInner - cosOuter, 1e-3f));
                const float attLin = L.p3.z;
                const float attQuad = L.p3.w;
                const float att = 1.0f / (1.0f + attLin * dist + attQuad * dist * dist);
                const float fade = saturate(1.0f - (dist / range));
                Lo += baseColor * lightColor * ndl * spot * att * fade;
            }
        }
    }

    return Lo;
}

float4 PS_ReflectionCapture(VSOut IN) : SV_Target0
{
    const uint flags = asuint(uParams.y);
    float3 baseColor = uBaseColor.rgb;
    float alphaOut = uBaseColor.a;

    if ((flags & FLAG_USE_TEX) != 0)
    {
        const float4 tex = gAlbedo.Sample(gLinear, IN.uv);
        baseColor *= tex.rgb;
        alphaOut *= tex.a;
    }

    const float ambientStrength = uCapturePosAmbient.w;
    const float3 ambient = baseColor * ambientStrength;
    const float3 Lo = EvalLights(IN.worldPos, normalize(IN.nrmW), baseColor);
    return float4(ambient + Lo, alphaOut);
}
