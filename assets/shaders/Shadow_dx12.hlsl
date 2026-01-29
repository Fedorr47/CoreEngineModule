// Shadow_dx12.hlsl
// Depth-only pass. We still provide PSMain to satisfy pipeline creation,
// but it returns void (no render targets).

struct VSIn
{
    float3 posL : POSITION;
};

struct VSOut
{
    float4 posH : SV_Position;
};

cbuffer ShadowCB : register(b0)
{
    float4x4 uMVP; // lightProj * lightView * model
};

VSOut VSMain(VSIn vin)
{
    VSOut o;
    o.posH = mul(uMVP, float4(vin.posL, 1.0)); // IMPORTANT: M * v
    return o;
}

// No color outputs, depth comes from SV_Position
void PSMain()
{
}
