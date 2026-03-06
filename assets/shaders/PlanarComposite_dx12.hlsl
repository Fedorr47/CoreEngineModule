// PlanarComposite_dx12.hlsl
// Fullscreen composite: overlay planar reflection color where mask alpha > 0.

SamplerState gLinear : register(s0);

Texture2D gPlanarMask : register(t0);
Texture2D gPlanarColor : register(t1);

struct VSOut
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut VS_Fullscreen(uint vid : SV_VertexID)
{
    // Fullscreen triangle
	float2 verts[3] = { float2(-1.0, -1.0), float2(-1.0, 3.0), float2(3.0, -1.0) };
	float2 p = verts[vid];

	VSOut o;
	o.pos = float4(p, 0.0, 1.0);
	// IMPORTANT: produce texture-space UV where (0,0) is TOP-left for Texture2D sampling
	// This matches DeferredLighting_dx12.hlsl and CopyToSwapChain_dx12.hlsl.
	o.uv = float2((p.x + 1.0f) * 0.5f, 1.0f - (p.y + 1.0f) * 0.5f);
	return o;
}

float4 PS_PlanarComposite(VSOut i) : SV_Target0
{
    // Mask is written as RGBA8, alpha channel holds 0..1 mirror coverage.
	const float mask = saturate(gPlanarMask.Sample(gLinear, i.uv).a);
	const float3 refl = gPlanarColor.Sample(gLinear, i.uv).rgb;

    // Output alpha drives blending (SrcAlpha, InvSrcAlpha).
	return float4(refl, mask);
}