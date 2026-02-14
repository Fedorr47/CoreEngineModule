// DebugLines_dx12.hlsl
// Minimal line shader for debug primitives.

cbuffer DebugLinesConstants : register(b0)
{
    float4x4 uViewProj; // CPU uploads TRANSPOSE(viewProj)
};

struct VSIn
{
    float3 pos : POSITION;
    float4 col : COLOR0; // R8G8B8A8_UNORM
};

struct VSOut
{
    float4 posH : SV_POSITION;
    float4 col  : COLOR0;
};

VSOut VS_DebugLines(VSIn vin)
{
    VSOut v;
    // With transposed matrix, treat float4 as row-vector.
    v.posH = mul(float4(vin.pos, 1.0f), uViewProj);
    v.col = vin.col;
    return v;
}

float4 PS_DebugLines(VSOut pin) : SV_TARGET
{
    return pin.col;
}
