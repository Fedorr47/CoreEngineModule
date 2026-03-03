// CoreEngineModule OpenGL mesh fragment shader (Phong/Blinn-Phong baseline).
// Supports optional albedo/normal/specular/gloss maps.
// Designed to work without tangent attributes (derivative-based TBN).

in VS_OUT
{
    vec3 worldPos;
    vec3 normalWS;
    vec2 uv;
} fs_in;

out vec4 FragColor;

uniform vec4 uColor;

// Samplers (units are set by OpenGLRenderer.cppm)
uniform sampler2D uTex;        // unit 0
uniform sampler2D uNormalTex;  // unit 1
uniform sampler2D uSpecTex;    // unit 2
uniform sampler2D uGlossTex;   // unit 3

uniform int uUseTex;
uniform int uUseNormalTex;
uniform int uUseSpecTex;
uniform int uUseGlossTex;

// ---------------- Lighting (world-space) ----------------
// NOTE: our GL command list currently supports only int / vec4 / mat4 uniforms.
// So we pack data into vec4 to avoid extra uniform setters.
uniform vec4 uCameraPos;      // xyz: camera position
uniform vec4 uDirLightDirI;   // xyz: direction (FROM light towards the scene), w: intensity
uniform vec4 uDirLightColor;  // rgb: color

const int kMaxPointLights = 8;
uniform int uPointLightCount;
uniform vec4 uPointPosI[kMaxPointLights];   // xyz: position, w: intensity
uniform vec4 uPointColR[kMaxPointLights];   // rgb: color, w: range
uniform vec4 uPointAtt[kMaxPointLights];    // x: attConst, y: attLin, z: attQuad

// --- Helper: cotangent frame from derivatives (no tangents required) ---
mat3 CotangentFrame(vec3 N, vec3 p, vec2 uv)
{
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);

    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invMax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invMax, B * invMax, N);
}

vec3 SampleNormalWS()
{
    vec3 N = normalize(fs_in.normalWS);

    if (uUseNormalTex == 0)
        return N;

    // Normal map is assumed to be in tangent space, encoded as [0..1] -> [-1..1].
    vec3 nTS = texture(uNormalTex, fs_in.uv).xyz * 2.0 - 1.0;

    // Derivative-based TBN.
    mat3 TBN = CotangentFrame(N, fs_in.worldPos, fs_in.uv);
    
    return normalize(TBN * nTS);
}

float PointAttenuation(float dist, float range, vec3 att)
{
	// Classic attenuation + soft range fade.
	float denom = att.x + att.y * dist + att.z * dist * dist;
	float a = (denom > 1e-6) ? (1.0 / denom) : 1.0;
	if (range > 0.0)
	{
		float t = clamp(1.0 - dist / range, 0.0, 1.0);
		a *= t * t;
	}
	return a;
}

void main()
{
    // Base color
    vec3 base = uColor.rgb;
    float alpha = uColor.a;

    if (uUseTex != 0)
    {
        vec4 albedo = texture(uTex, fs_in.uv);
        base *= albedo.rgb;
        alpha *= albedo.a;
    }

    // Lighting (world-space)
    vec3 N = SampleNormalWS();

    vec3 V = normalize(uCameraPos.xyz - fs_in.worldPos);

    // Specular intensity from map (or 1.0)
    vec3 specColor = vec3(1.0);
    if (uUseSpecTex != 0)
        specColor = texture(uSpecTex, fs_in.uv).rgb;

    // Gloss controls shininess (0..1 -> 8..256)
    float gloss = 1.0;
    if (uUseGlossTex != 0)
        gloss = texture(uGlossTex, fs_in.uv).r;

    float shininess = mix(8.0, 256.0, clamp(gloss, 0.0, 1.0));

    vec3 color = 0.10 * base; // ambient

    // Directional
    {
        vec3 Ld = normalize(uDirLightDirI.xyz);
        float intensity = uDirLightDirI.w;
        vec3 lc = uDirLightColor.rgb;
    
        float NdotL = max(dot(N, -Ld), 0.0);
        vec3 H = normalize((-Ld) + V);
        float specTerm = pow(max(dot(N, H), 0.0), shininess);
    
        color += base * lc * intensity * NdotL;
        color += specColor * lc * intensity * specTerm * (NdotL > 0.0 ? 1.0 : 0.0);
    }
    
    // Points
    int pointCount = clamp(uPointLightCount, 0, kMaxPointLights);
    for (int i = 0; i < pointCount; ++i)
    {
        vec3 lp = uPointPosI[i].xyz;
        float intensity = uPointPosI[i].w;
        vec3 lc = uPointColR[i].rgb;
        float range = uPointColR[i].w;
        vec3 att = vec3(uPointAtt[i].x, uPointAtt[i].y, uPointAtt[i].z);
    
        vec3 toL = lp - fs_in.worldPos;
        float dist = length(toL);
        vec3 L = (dist > 1e-6) ? (toL / dist) : vec3(0.0, 1.0, 0.0);
    
        float atten = PointAttenuation(dist, range, att);
        float NdotL = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float specTerm = pow(max(dot(N, H), 0.0), shininess);
    
        float li = intensity * atten;
        color += base * lc * li * NdotL;
        color += specColor * lc * li * specTerm * (NdotL > 0.0 ? 1.0 : 0.0);
    }

    FragColor = vec4(color, alpha);
}