// ReflectionCapture_dx12.hlsl (fallback: render ONE face per pass)
// Save as UTF-8 without BOM.

SamplerState gLinear : register(s0);
Texture2D gAlbedo : register(t0);

struct GPULight
{
    float4 p0; // pos.xyz, type
    float4 p1; // dir.xyz, intensity
    float4 p2; // color.rgb, range
    float4 p3; // cosInner, cosOuter, attLin, attQuad
};
StructuredBuffer<GPULight> gLights : register(t2);

cbuffer ReflectionCaptureFaceCB : register(b0)
{
    float4x4 uViewProj;
    float4 uCapturePosAmbient; // xyz + ambientStrength
    float4 uBaseColor; // rgba
    float4 uParams; // x=lightCount, y=flags(asfloat)
};

static const uint FLAG_USE_TEX = 1u << 0;
static const int LIGHT_DIR = 0;
static const int LIGHT_POINT = 1;
static const int LIGHT_SPOT = 2;

float4x4 MakeMatRows(float4 r0, float4 r1, float4 r2, float4 r3)
{
    return float4x4(r0, r1, r2, r3);
}

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

VSOut VS_ReflectionCapture(VSIn IN)
{
    VSOut OUT;

    float4x4 model = MakeMatRows(IN.i0, IN.i1, IN.i2, IN.i3);
    float4 world = mul(float4(IN.pos, 1.0f), model);

    OUT.worldPos = world.xyz;
    OUT.nrmW = normalize(mul(float4(IN.nrm, 0.0f), model).xyz);
    OUT.uv = IN.uv;

    float4x4 vp = uViewProj;
    OUT.posH = mul(world, vp);
    
    return OUT;
}

float3 EvalDirLight(float3 N, float3 baseColor)
{
    // Kept for backward compatibility with older captures, but now we use EvalLights().
    return 0.0.xxx;
}

float3 EvalLights(float3 worldPos, float3 N, float3 baseColor)
{
    const uint lightCount = (uint) uParams.x;
    float3 Lo = 0.0.xxx;

    [loop]
    for (uint i = 0; i < lightCount; ++i)
    {
        const GPULight L = gLights[i];
        const int type = (int) L.p0.w;

        const float3 lightColor = L.p2.rgb * L.p1.w;

        if (type == LIGHT_DIR)
        {
            const float3 dirFromLight = normalize(L.p1.xyz);
            const float3 Ldir = -dirFromLight; // to light
            const float ndl = saturate(dot(N, Ldir));
            Lo += baseColor * lightColor * ndl;
        }
        else if (type == LIGHT_POINT)
        {
            const float3 lightPos = L.p0.xyz;
            const float3 toLight = lightPos - worldPos;
            const float dist = length(toLight);
            const float range = max(L.p2.w, 1e-3f);
            if (dist < range)
            {
                const float3 Ldir = toLight / max(dist, 1e-3f);
                const float ndl = saturate(dot(N, Ldir));

                // Smooth range fade + standard quadratic attenuation.
                const float attLin = L.p3.z;
                const float attQuad = L.p3.w;
                const float att = 1.0f / (1.0f + attLin * dist + attQuad * dist * dist);
                const float fade = saturate(1.0f - (dist / range));

                Lo += baseColor * lightColor * ndl * att * fade;
            }
        }
        else if (type == LIGHT_SPOT)
        {
            const float3 lightPos = L.p0.xyz;
            const float3 toLight = lightPos - worldPos;
            const float dist = length(toLight);
            const float range = max(L.p2.w, 1e-3f);
            if (dist < range)
            {
                const float3 Ldir = toLight / max(dist, 1e-3f);
                const float ndl = saturate(dot(N, Ldir));

                const float3 dirFromLight = normalize(L.p1.xyz); // FROM light
                const float3 toPointFromLight = normalize(worldPos - lightPos);
                const float cosAng = dot(dirFromLight, toPointFromLight);

                const float cosInner = L.p3.x;
                const float cosOuter = L.p3.y;
                const float spot = saturate((cosAng - cosOuter) / max(cosInner - cosOuter, 1e-3f));

                const float attLin = L.p3.z;
                const float attQuad = L.p3.w;
                const float att = 1.0f / (1.0f + attLin * dist + attQuad * dist * dist);
                const float fade = saturate(1.0f - (dist / range));

                Lo += baseColor * lightColor * ndl * spot * att * fade;
            }
        }
    }

    return Lo;
}

float4 PS_ReflectionCapture(VSOut IN) : SV_Target0
{
    const uint flags = asuint(uParams.y);
    float3 baseColor = uBaseColor.rgb;
    float alphaOut = uBaseColor.a;

    if ((flags & FLAG_USE_TEX) != 0)
    {
        float4 tex = gAlbedo.Sample(gLinear, IN.uv);
        baseColor *= tex.rgb;
        alphaOut *= tex.a;
    }

    // Lightweight forward diffuse lighting to match the book's DynamicCubeMap look.
    // This reduces harsh seams compared to storing raw albedo.
    const float ambientStrength = uCapturePosAmbient.w;
    const float3 ambient = baseColor * ambientStrength;
    const float3 Lo = EvalLights(IN.worldPos, normalize(IN.nrmW), baseColor);
    return float4(ambient + Lo, alphaOut);
}
