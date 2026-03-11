#include "SkinningCommon_dx12.hlsli"

cbuffer PointShadowCB : register(b0)
{
    float4x4 uFaceViewProj;
    float4   uLightPosRange; // xyz + range
    float4   uMisc;          // x = bias
    float4x4 uModel;
    float4   uSkinning;      // x=paletteOffset, y=boneCount
};

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv  : TEXCOORD0;
    float4 tangent : TANGENT;
    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct VSOut
{
    float4 posH     : SV_Position;
    float3 worldPos : TEXCOORD0;
};

VSOut VS_ShadowPoint(VSIn IN)
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
    OUT.posH = mul(world, uFaceViewProj);
    return OUT;
}

float PS_ShadowPoint(VSOut IN) : SV_Target0
{
    const float3 L = IN.worldPos - uLightPosRange.xyz;
    const float dist = length(L);
    return saturate(dist / max(uLightPosRange.w, 0.001f));
}
