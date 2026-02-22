// DebugCubeAtlas_dx12.hlsl
// NOTE: Save as UTF-8 without BOM.

SamplerState gPointClamp : register(s2);
Texture2DArray<float4> gCube : register(t0);

cbuffer DebugCubeAtlasCB : register(b0)
{
    float uInvRange; // 1/range if stored is world-distance, or 1 if stored is already normalized [0..1]
    float uGamma; // 1.0 = linear
    uint uInvert; // 1 -> invert (near=white)
    uint uShowGrid; // 1 -> draw tile borders
    uint uMode; // 0 = depth grayscale (use .r), 1 = color (use .rgb)
    uint _pad0;
    float2 uViewportOrigin; // in pixels (top-left)
    float2 uInvViewportSize; // (1/width, 1/height)
    float2 _pad1;
};

struct VSOut
{
    float4 pos : SV_Position;
};

// Fullscreen triangle
VSOut VSMain(uint vid : SV_VertexID)
{
    VSOut o;
    if (vid == 0)
        o.pos = float4(-1.0, -1.0, 0.0, 1.0);
    if (vid == 1)
        o.pos = float4(-1.0, 3.0, 0.0, 1.0);
    if (vid == 2)
        o.pos = float4(3.0, -1.0, 0.0, 1.0);
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    // SV_Position in pixel coords (top-left origin in D3D)
    float2 uv = (i.pos.xy - uViewportOrigin) * uInvViewportSize;
    uv = saturate(uv);

    // atlas 3x2 tiles
    uint tileX = min((uint) (uv.x * 3.0), 2u);
    uint tileY = min((uint) (uv.y * 2.0), 1u);
    uint face = tileY * 3u + tileX; // 0..5

    float2 uvLocal = frac(uv * float2(3.0, 2.0));

    // borders
    if (uShowGrid != 0)
    {
        float2 fw = fwidth(uvLocal);
        float gx = step(uvLocal.x, fw.x * 1.5) + step(1.0 - uvLocal.x, fw.x * 1.5);
        float gy = step(uvLocal.y, fw.y * 1.5) + step(1.0 - uvLocal.y, fw.y * 1.5);
        if (gx + gy > 0.0)
            return float4(1, 1, 1, 1);
    }

    // Sample the selected cubemap face as an array slice.
    // This avoids TextureCube vs Texture2DArray view-dimension mismatch and shows faces exactly as stored.
    float4 sampled = gCube.SampleLevel(gPointClamp, float3(uvLocal, (float) face), 0);
    
    if (uMode == 0u)
    {
        // depth grayscale (point shadow distance)
        float stored = sampled.r;
        float v = saturate(stored * uInvRange);
        if (uInvert != 0u)
            v = 1.0 - v;
        if (abs(uGamma - 1.0) > 1e-3)
            v = pow(v, 1.0 / max(uGamma, 1e-3));
        return float4(v, v, v, 1.0);
    }
    else
    {
        // color (reflection capture)
        float3 c = saturate(sampled.rgb);
        if (abs(uGamma - 1.0) > 1e-3)
            c = pow(c, 1.0 / max(uGamma, 1e-3));
        return float4(c, 1.0);
    }
}