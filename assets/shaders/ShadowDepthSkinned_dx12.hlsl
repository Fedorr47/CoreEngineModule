#include "SkinningCommon_dx12.hlsli"

cbuffer ShadowCB : register(b0)
{
    float4x4 uLightViewProj;
    float4x4 uModel;
    float4   uSkinning; // x=paletteOffset, y=boneCount
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
    float4 posH : SV_Position;
};

VSOut VS_Shadow(VSIn IN)
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
    OUT.posH = mul(world, uLightViewProj);
    return OUT;
}

void PS_Shadow(VSOut IN)
{
}
