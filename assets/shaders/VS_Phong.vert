// CoreEngineModule OpenGL mesh vertex shader (Phong baseline).
// Optional texture usage is controlled via uniforms (uUseTex/uUseNormalTex/uUseSpecTex/uUseGlossTex).
// A compile-time USE_TEX=1 permutation may be provided by ShaderSystem.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uMVP;
uniform mat4 uModel;

out VS_OUT
{
    vec3 worldPos;
    vec3 normalWS;
    vec2 uv;
} vs_out;

void main()
{
    vec4 wp = uModel * vec4(aPos, 1.0);
    vs_out.worldPos = wp.xyz;

    mat3 nrmMat = mat3(transpose(inverse(uModel)));
    vs_out.normalWS = normalize(nrmMat * aNormal);

    vs_out.uv = aUV;

    gl_Position = uMVP * vec4(aPos, 1.0);
}