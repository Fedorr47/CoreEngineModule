cbuffer PerDraw : register(b0)
{
	float4x4 uMVP; // lightMVP
};

struct VSIn
{
	float3 pos : POSITION;
	float3 nrm : NORMAL;
	float2 uv : TEXCOORD0;
};

struct VSOut
{
	float4 posH : SV_Position;
};

VSOut VSMain(VSIn vin)
{
	VSOut o;
	o.posH = mul(float4(vin.pos, 1.0f), uMVP);
	return o;
}

// Depth-only: no color output required
void PSMain()
{
}
