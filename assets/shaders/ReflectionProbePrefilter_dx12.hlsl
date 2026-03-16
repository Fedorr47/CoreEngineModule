Texture2DArray<float4> gSourceEnv : register(t0);
SamplerState gLinearClamp : register(s0);

cbuffer PrefilterCB : register(b0)
{
    float4 uFaceRoughnessMip; // x=faceIndex, y=roughness, z=mipLevel, w=copyBase(1/0)
};

static const float PI = 3.14159265359f;

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut VS_Fullscreen(uint id : SV_VertexID)
{
    float2 pos = float2((id == 2u) ? 3.0f : -1.0f, (id == 1u) ? 3.0f : -1.0f);
    float2 uv = float2((pos.x + 1.0f) * 0.5f, 1.0f - (pos.y + 1.0f) * 0.5f);

    VSOut o;
    o.pos = float4(pos, 0.0f, 1.0f);
    o.uv = uv;
    return o;
}

struct CubeFaceUV
{
    uint face;
    float2 uv;
};

CubeFaceUV CubeDirToFaceUV_Env(float3 dir)
{
    dir = normalize(dir);

    const float3 kF[6] =
    {
        float3(1, 0, 0),
        float3(-1, 0, 0),
        float3(0, 1, 0),
        float3(0, -1, 0),
        float3(0, 0, 1),
        float3(0, 0, -1)
    };

    const float3 kU[6] =
    {
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 0, -1),
        float3(0, 0, 1),
        float3(0, 1, 0),
        float3(0, 1, 0)
    };

    const float3 a = abs(dir);
    uint face = 0u;
    if (a.x >= a.y && a.x >= a.z)
        face = (dir.x >= 0.0f) ? 0u : 1u;
    else if (a.y >= a.x && a.y >= a.z)
        face = (dir.y >= 0.0f) ? 2u : 3u;
    else
        face = (dir.z >= 0.0f) ? 4u : 5u;

    const float3 F = kF[face];
    const float3 U = kU[face];
    const float3 R = normalize(cross(F, U));
    const float denom = max(abs(dot(dir, F)), 1e-6f);

    float2 st;
    st.x = dot(dir, R) / denom;
    st.y = dot(dir, U) / denom;

    CubeFaceUV o;
    o.face = face;
    o.uv = float2(st.x, -st.y) * 0.5f + 0.5f;
    return o;
}

float3 FaceUVToDir(uint face, float2 uv)
{
    const float3 kF[6] =
    {
        float3(1, 0, 0),
        float3(-1, 0, 0),
        float3(0, 1, 0),
        float3(0, -1, 0),
        float3(0, 0, 1),
        float3(0, 0, -1)
    };

    const float3 kU[6] =
    {
        float3(0, 1, 0),
        float3(0, 1, 0),
        float3(0, 0, -1),
        float3(0, 0, 1),
        float3(0, 1, 0),
        float3(0, 1, 0)
    };

    face = min(face, 5u);
    const float3 F = kF[face];
    const float3 U = kU[face];
    const float3 R = normalize(cross(F, U));

    const float2 st = float2(2.0f * uv.x - 1.0f, 1.0f - 2.0f * uv.y);
    return normalize(F + st.x * R + st.y * U);
}

float3 SampleSourceEnvArray(float3 dir, float lod)
{
    CubeFaceUV fu = CubeDirToFaceUV_Env(dir);
    if (fu.uv.x < 0.0f || fu.uv.x > 1.0f || fu.uv.y < 0.0f || fu.uv.y > 1.0f)
        return 0.0f;
    return gSourceEnv.SampleLevel(gLinearClamp, float3(fu.uv, (float)fu.face), lod).rgb;
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    const float a = max(roughness * roughness, 1.0e-4f);
    const float phi = 2.0f * PI * Xi.x;
    const float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    const float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    const float3 up = (abs(N.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(up, N));
    const float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float4 PS_PrefilterEnv(VSOut IN) : SV_Target
{
    const uint face = min((uint)round(uFaceRoughnessMip.x), 5u);
    const float roughness = saturate(uFaceRoughnessMip.y);
    const bool copyBase = (uFaceRoughnessMip.w > 0.5f);

    const float3 N = FaceUVToDir(face, IN.uv);

    if (copyBase || roughness <= 1.0e-4f)
    {
        return float4(SampleSourceEnvArray(N, 0.0f), 1.0f);
    }

    const float3 R = N;
    const float3 V = R;

    float3 prefilteredColor = 0.0f;
    float totalWeight = 0.0f;

    static const uint SAMPLE_COUNT = 64u;
    [loop]
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        const float2 Xi = Hammersley(i, SAMPLE_COUNT);
        const float3 H = ImportanceSampleGGX(Xi, N, roughness);
        const float3 L = normalize(2.0f * dot(V, H) * H - V);

        const float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0f)
        {
            prefilteredColor += SampleSourceEnvArray(L, 0.0f) * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 1.0e-4f);
    return float4(prefilteredColor, 1.0f);
}
