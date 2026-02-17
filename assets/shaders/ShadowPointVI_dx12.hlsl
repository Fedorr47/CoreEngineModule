// ShadowPointVI_dx12.hlsl
// Shader Model 6.1 (DXC) vertex+pixel shader for point light shadow cubemap.
// Uses SV_ViewID + View-Instancing to render all 6 faces in a single pass.

cbuffer PointShadowCB : register(b0)
{
    float4x4 uFaceViewProj[6];
    float4   uLightPosRange; // xyz + range
    float4   uMisc;          // x = bias (reserved)
};

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv  : TEXCOORD0;

    float4 i0  : TEXCOORD1;
    float4 i1  : TEXCOORD2;
    float4 i2  : TEXCOORD3;
    float4 i3  : TEXCOORD4;

    uint   viewId : SV_ViewID;
};

float4x4 MakeMatRows(float4 r0, float4 r1, float4 r2, float4 r3)
{
    return float4x4(r0, r1, r2, r3);
}

struct VSOut
{
    float4 posH     : SV_Position;
    float3 worldPos : TEXCOORD0;
};

VSOut VS_ShadowPointVI(VSIn IN)
{
    VSOut OUT;

    float4x4 model = MakeMatRows(IN.i0, IN.i1, IN.i2, IN.i3);
    float4 world = mul(float4(IN.pos, 1.0f), model);

    OUT.worldPos = world.xyz;
    const uint face = (IN.viewId < 6u) ? IN.viewId : 0u;
    OUT.posH = mul(world, uFaceViewProj[face]);
    return OUT;
}

// R32_FLOAT distance (normalized by range).
float PS_ShadowPointVI(VSOut IN) : SV_Target0
{
    float3 L = IN.worldPos - uLightPosRange.xyz;
    float dist = length(L);
    float norm = saturate(dist / max(uLightPosRange.w, 0.001f));
    return norm;
}
