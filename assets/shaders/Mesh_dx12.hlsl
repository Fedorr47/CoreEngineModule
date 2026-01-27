cbuffer PerDraw : register(b0)
{
    float4x4 uMVP;
    float4 uColor;
    int uUseTex;
    float3 _pad; // выровнять до 16 байт
};

Texture2D gTex0 : register(t0);
SamplerState gSamp0 : register(s0);

struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOut VSMain(VSIn i)
{
    VSOut o;
    o.pos = mul(uMVP, float4(i.pos, 1.0f));
    o.uv = i.uv;
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    float4 c = uColor;
    if (uUseTex != 0)
    {
        float4 tex = gTex0.Sample(gSamp0, i.uv);
        c *= tex;
    }
    return c;
}
