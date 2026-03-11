#ifndef CORE_SKINNING_COMMON_DX12_HLSLI
#define CORE_SKINNING_COMMON_DX12_HLSLI

// Shared DX12 GPU skinning helpers.
// CPU uploads column-major skin matrices into a single structured buffer.
// Each draw selects its palette slice via:
//   uSkinning.x = paletteOffset
//   uSkinning.y = boneCount
StructuredBuffer<float4x4> gSkinMatrices : register(t19);

float4x4 Identity4x4()
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

float4x4 LoadSkinMatrix(uint paletteOffset, uint boneCount, uint boneIndex)
{
    if (boneIndex >= boneCount)
    {
        return Identity4x4();
    }
    return gSkinMatrices[paletteOffset + boneIndex];
}

float4x4 BuildSkinMatrix(uint paletteOffset, uint boneCount, uint4 boneIndices, float4 boneWeights)
{
    float4x4 skin = 0.0f;

    if (boneWeights.x > 0.0f)
    {
        skin += LoadSkinMatrix(paletteOffset, boneCount, boneIndices.x) * boneWeights.x;
    }
    if (boneWeights.y > 0.0f)
    {
        skin += LoadSkinMatrix(paletteOffset, boneCount, boneIndices.y) * boneWeights.y;
    }
    if (boneWeights.z > 0.0f)
    {
        skin += LoadSkinMatrix(paletteOffset, boneCount, boneIndices.z) * boneWeights.z;
    }
    if (boneWeights.w > 0.0f)
    {
        skin += LoadSkinMatrix(paletteOffset, boneCount, boneIndices.w) * boneWeights.w;
    }

    return skin;
}

void ApplySkinning(
    uint paletteOffset,
    uint boneCount,
    uint4 boneIndices,
    float4 boneWeights,
    float3 inPos,
    float3 inNrm,
    float4 inTangent,
    out float3 outPos,
    out float3 outNrm,
    out float4 outTangent)
{
    const float4x4 skin = BuildSkinMatrix(paletteOffset, boneCount, boneIndices, boneWeights);
    const float4 skinnedPos = mul(skin, float4(inPos, 1.0f));
    const float3x3 skin3x3 = (float3x3) skin;

    outPos = skinnedPos.xyz;
    outNrm = normalize(mul(skin3x3, inNrm));

    float3 tangent = normalize(mul(skin3x3, inTangent.xyz));
    tangent = normalize(tangent - outNrm * dot(outNrm, tangent));
    outTangent = float4(tangent, inTangent.w);
}

#endif
