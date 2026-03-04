SamplerState gLinearClamp : register(s3);

Texture2D gGBuffer0 : register(t0);
Texture2D gGBuffer1 : register(t1);
Texture2D gGBuffer2 : register(t2);
Texture2D<float> gDepth : register(t3);

struct VSOut
{
	float4 posH : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut VS_Fullscreen(uint vid : SV_VertexID)
{
	VSOut o;
	// Fullscreen triangle
	float2 p = (vid == 0) ? float2(-1, -1) : (vid == 1) ? float2(-1, 3) : float2(3, -1);
	float2 u = (vid == 0) ? float2(0, 1) : (vid == 1) ? float2(0, -1) : float2(2, 1);
	o.posH = float4(p, 0, 1);
	o.uv = u;
	return o;
}

float4 PS_DeferredLighting(VSOut i) : SV_Target0
{
	// Debug resolve: show albedo (gbuf0.rgb). Background stays black; skybox pass fills it.
	const float d = gDepth.SampleLevel(gLinearClamp, i.uv, 0);
	if (d >= 0.999999f)
	{
		return float4(0, 0, 0, 1);
	}
	const float3 albedo = gGBuffer0.SampleLevel(gLinearClamp, i.uv, 0).rgb;
	return float4(albedo, 1);
}