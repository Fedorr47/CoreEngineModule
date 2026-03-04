SamplerState gLinear : register(s0);

// Material SRVs (match existing root signature slots)
Texture2D gAlbedo : register(t0);
Texture2D gNormal : register(t12);
Texture2D gMetalness : register(t13);
Texture2D gRoughness : register(t14);
Texture2D gAO : register(t15);
Texture2D gEmissive : register(t16);

cbuffer PerBatchCB : register(b0)
{
	float4x4 uViewProj;
	float4x4 uLightViewProj;
	float4 uCameraAmbient;
	float4 uCameraForward;
	float4 uBaseColor;
	float4 uMaterialFlags; // w = flags bits (asfloat)
	float4 uPbrParams; // x=metallic, y=roughness, z=ao, w=emissiveStrength
	float4 uCounts;
	float4 uShadowBias;
	float4 uEnvProbeBoxMin;
	float4 uEnvProbeBoxMax;
};

static const uint FLAG_USE_TEX = 1u << 0;
static const uint FLAG_USE_NORMAL = 1u << 2;
static const uint FLAG_USE_METAL_TEX = 1u << 3;
static const uint FLAG_USE_ROUGH_TEX = 1u << 4;
static const uint FLAG_USE_AO_TEX = 1u << 5;
static const uint FLAG_USE_EMISSIVE_TEX = 1u << 6;

struct VSIn
{
	float3 pos : POSITION;
	float3 nrm : NORMAL;
	float2 uv : TEXCOORD0;
	float4 i0 : TEXCOORD1;
	float4 i1 : TEXCOORD2;
	float4 i2 : TEXCOORD3;
	float4 i3 : TEXCOORD4;
};

struct VSOut
{
	float4 posH : SV_Position;
	float3 worldPos : TEXCOORD0;
	float3 nrmW : TEXCOORD1;
	float2 uv : TEXCOORD2;
};

VSOut VS_GBuffer(VSIn vin)
{
	VSOut o;

	float4x4 M = float4x4(vin.i0, vin.i1, vin.i2, vin.i3);
	float4 wp = mul(float4(vin.pos, 1.0f), M);
	float3 nw = mul(float4(vin.nrm, 0.0f), M).xyz;

	o.worldPos = wp.xyz;
	o.nrmW = normalize(nw);
	o.uv = vin.uv;
	o.posH = mul(wp, uViewProj);
	return o;
}

float3 GetNormalMapped(float3 N, float3 worldPos, float2 uv)
{
	float3 dp1 = ddx(worldPos);
	float3 dp2 = ddy(worldPos);
	float2 duv1 = ddx(uv);
	float2 duv2 = ddy(uv);

	float3 T = dp1 * duv2.y - dp2 * duv1.y;
	float3 B = -dp1 * duv2.x + dp2 * duv1.x;

	T = normalize(T - N * dot(N, T));
	B = normalize(B - N * dot(N, B));

	float3 nTS = gNormal.Sample(gLinear, uv).xyz * 2.0f - 1.0f;
	float3x3 TBN = float3x3(T, B, N);
	return normalize(mul(nTS, TBN));
}

struct PSOut
{
	float4 o0 : SV_Target0; // baseColor.rgb, roughness
	float4 o1 : SV_Target1; // normal.xyz (0..1), metallic
	float4 o2 : SV_Target2; // emissive.rgb, ao
};

PSOut PS_GBuffer(VSOut i)
{
	PSOut o;

	const uint flags = asuint(uMaterialFlags.w);

	float4 base = uBaseColor;
	if ((flags & FLAG_USE_TEX) != 0)
	{
		float4 a = gAlbedo.Sample(gLinear, i.uv);
		base *= a;
	}

	float metallic = uPbrParams.x;
	if ((flags & FLAG_USE_METAL_TEX) != 0)
	{
		metallic = gMetalness.Sample(gLinear, i.uv).r;
	}

	float roughness = uPbrParams.y;
	if ((flags & FLAG_USE_ROUGH_TEX) != 0)
	{
		roughness = gRoughness.Sample(gLinear, i.uv).r;
	}

	float ao = uPbrParams.z;
	if ((flags & FLAG_USE_AO_TEX) != 0)
	{
		ao = gAO.Sample(gLinear, i.uv).r;
	}

	float3 emissive = 0.0f.xxx;
	if ((flags & FLAG_USE_EMISSIVE_TEX) != 0)
	{
		emissive = gEmissive.Sample(gLinear, i.uv).rgb * uPbrParams.w;
	}

	float3 N = normalize(i.nrmW);
	if ((flags & FLAG_USE_NORMAL) != 0)
	{
		N = GetNormalMapped(N, i.worldPos, i.uv);
	}

	float3 nEnc = N * 0.5f + 0.5f;

	o.o0 = float4(base.rgb, saturate(roughness));
	o.o1 = float4(saturate(nEnc), saturate(metallic));
	o.o2 = float4(emissive, saturate(ao));
	return o;
}