cbuffer ShadowCB : register(b0)
{
    float4x4 uLightViewProj;
};

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv  : TEXCOORD0;

    // Instance matrix rows in TEXCOORD1..4
    float4 i0  : TEXCOORD1;
    float4 i1  : TEXCOORD2;
    float4 i2  : TEXCOORD3;
    float4 i3  : TEXCOORD4;
};

struct VSOut
{
    float4 posH : SV_Position;
};

float4x4 MakeMatRows(float4 r0, float4 r1, float4 r2, float4 r3)
{
    return float4x4(r0, r1, r2, r3);
}

VSOut VS_Shadow(VSIn IN)
{
    VSOut OUT;

    // Rows provided by CPU, use row-vector mul().
    
    float4x4 model = MakeMatRows(IN.i0, IN.i1, IN.i2, IN.i3);
    float4 world = mul(float4(IN.pos, 1.0f), model);

    OUT.posH = mul(world, uLightViewProj);
    return OUT;
}

// Depth-only: no color output needed (PS still required for DX12 PSO).
void PS_Shadow(VSOut IN)
{
}
