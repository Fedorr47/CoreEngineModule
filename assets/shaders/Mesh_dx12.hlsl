cbuffer PerDraw : register(b0)
{
	float4x4 uMVP;
	float4x4 uLightMVP;
	float4 uColor;

	int uUseTex;
	int uUseShadow;
	float uShadowBias;
	float _pad0;
};

Texture2D gAlbedo : register(t0);
Texture2D<float> gShadow : register(t1);

SamplerState gSamp : register(s0);
SamplerComparisonState gShadowSamp : register(s1);

struct VSIn
{
	float3 pos : POSITION;
	float3 nrm : NORMAL;
	float2 uv : TEXCOORD0;
};

struct VSOut
{
	float4 posH : SV_Position;
	float2 uv : TEXCOORD0;
	float4 shadowPos : TEXCOORD1;
};

VSOut VSMain(VSIn vin)
{
	VSOut o;
	o.posH = mul(float4(vin.pos, 1.0f), uMVP);
	o.uv = vin.uv;
	o.shadowPos = mul(float4(vin.pos, 1.0f), uLightMVP);
	return o;
}

float SampleShadow(float4 shadowPos)
{
	float3 proj = shadowPos.xyz / shadowPos.w;

    // NDC xy -> UV
	float2 uv = proj.xy * 0.5f + 0.5f;
	float depth = proj.z;

    // outside = fully lit
	if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || depth < 0 || depth > 1)
		return 1.0f;

    // Compare sampler: returns [0..1]
	return gShadow.SampleCmpLevelZero(gShadowSamp, uv, depth - uShadowBias);
}

float4 PSMain(VSOut pin) : SV_Target0
{
	float4 base = uColor;

	if (uUseTex != 0)
		base *= gAlbedo.Sample(gSamp, pin.uv);

	if (uUseShadow != 0)
	{
		float sh = SampleShadow(pin.shadowPos);
        // simple shadow tint
		base.rgb *= (0.25 + 0.75 * sh);
	}

	return base;
}
