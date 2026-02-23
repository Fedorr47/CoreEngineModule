// DebugCubeAtlas_dx12.hlsl
// NOTE: Save as UTF-8 without BOM.

SamplerState gPointClamp : register(s0);
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

uint GlyphRow5x7(uint glyph, uint y)
{
    // 5x7 bitmap font rows encoded in lower 5 bits (bit0 = leftmost pixel)
    // glyph: 0='+' , 1='-', 2='X', 3='Y', 4='Z'
	y = min(y, 6u);

	if (glyph == 0u) // +
	{
		switch (y)
		{
			case 1u:
				return 4u;
			case 2u:
				return 4u;
			case 3u:
				return 31u;
			case 4u:
				return 4u;
			case 5u:
				return 4u;
			default:
				return 0u;
		}
	}
	if (glyph == 1u) // -
	{
		return (y == 3u) ? 31u : 0u;
	}
	if (glyph == 2u) // X
	{
		switch (y)
		{
			case 0u:
				return 17u;
			case 1u:
				return 17u;
			case 2u:
				return 10u;
			case 3u:
				return 4u;
			case 4u:
				return 10u;
			case 5u:
				return 17u;
			default:
				return 17u;
		}
	}
	if (glyph == 3u) // Y
	{
		switch (y)
		{
			case 0u:
				return 17u;
			case 1u:
				return 17u;
			case 2u:
				return 10u;
			default:
				return 4u;
		}
	}

    // Z
	switch (y)
	{
		case 0u:
			return 31u;
		case 1u:
			return 1u;
		case 2u:
			return 2u;
		case 3u:
			return 4u;
		case 4u:
			return 8u;
		case 5u:
			return 16u;
		default:
			return 31u;
	}
}

float GlyphMask5x7(float2 uv, uint glyph)
{
    // uv in glyph-local space [0..1]^2
	if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0)
		return 0.0;

	uint x = min((uint) floor(uv.x * 5.0), 4u);
	uint y = min((uint) floor(uv.y * 7.0), 6u);
	uint rowBits = GlyphRow5x7(glyph, y);
	uint bit = (rowBits >> x) & 1u;
	return (bit != 0u) ? 1.0 : 0.0;
}

float FaceLabelMask(float2 uvLocal, uint face)
{
    // Draw compact label in top-left corner of each tile: +X, -X, +Y, -Y, +Z, -Z
    // Atlas is sampled as stored (Texture2DArray slice), so this helps verify exact face/slice mapping.
	uint signGlyph = 0u; // '+'
	uint axisGlyph = 2u; // X
	switch (face)
	{
		case 0u:
			signGlyph = 0u;
			axisGlyph = 2u;
			break; // +X
		case 1u:
			signGlyph = 1u;
			axisGlyph = 2u;
			break; // -X
		case 2u:
			signGlyph = 0u;
			axisGlyph = 3u;
			break; // +Y
		case 3u:
			signGlyph = 1u;
			axisGlyph = 3u;
			break; // -Y
		case 4u:
			signGlyph = 0u;
			axisGlyph = 4u;
			break; // +Z
		default:
			signGlyph = 1u;
			axisGlyph = 4u;
			break; // -Z
	}

    // Label box in tile-local UV. Kept small enough to not hide much content.
	const float2 boxMin = float2(0.03, 0.03);
	const float2 boxSize = float2(0.22, 0.11);
	float2 p = (uvLocal - boxMin) / boxSize;
	if (p.x < 0.0 || p.x >= 1.0 || p.y < 0.0 || p.y >= 1.0)
		return 0.0;

    // Inner padding
	p = (p - float2(0.06, 0.10)) / float2(0.88, 0.80);

    // Two glyphs side-by-side with fixed spacing
	float2 pSign = float2((p.x - 0.00) / 0.42, p.y);
	float2 pAxis = float2((p.x - 0.52) / 0.42, p.y);
	float m = max(GlyphMask5x7(pSign, signGlyph), GlyphMask5x7(pAxis, axisGlyph));
	return m;
}

float FaceLabelBgMask(float2 uvLocal)
{
	const float2 boxMin = float2(0.03, 0.03);
	const float2 boxMax = boxMin + float2(0.22, 0.11);
	return (uvLocal.x >= boxMin.x && uvLocal.x < boxMax.x && uvLocal.y >= boxMin.y && uvLocal.y < boxMax.y) ? 1.0 : 0.0;
}

float4 PSMain(VSOut i) : SV_Target
{
    // SV_Position in pixel coords (top-left origin in D3D)
    float2 uv = (i.pos.xy - uViewportOrigin) * uInvViewportSize;
    //uv = saturate(uv);
	uv = clamp(uv, 0.0, 0.999999);

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
			
		float3 outColor = float3(v, v, v);
		float bg = FaceLabelBgMask(uvLocal);
		float label = FaceLabelMask(uvLocal, face);
		outColor = lerp(outColor, outColor * 0.35, bg * 0.85);
		outColor = lerp(outColor, float3(1.0, 0.95, 0.2), label);
		return float4(outColor, 1.0);
	}
    else
    {
        // color (reflection capture)
        float3 c = saturate(sampled.rgb);
        if (abs(uGamma - 1.0) > 1e-3)
            c = pow(c, 1.0 / max(uGamma, 1e-3));
		
		float bg = FaceLabelBgMask(uvLocal);
		float label = FaceLabelMask(uvLocal, face);
		c = lerp(c, c * 0.35, bg * 0.85);
		c = lerp(c, float3(1.0, 0.95, 0.2), label);
        return float4(c, 1.0);
    }
}