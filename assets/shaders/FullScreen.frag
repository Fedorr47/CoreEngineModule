#version 330 core
in vec2 vUV;
out vec4 oColor;

uniform sampler2D uTex;
uniform int uUseTex;
uniform vec4 uColor;

void main()
{
  vec4 c = uColor;
  if (uUseTex != 0) c = texture(uTex, vUV);
  oColor = c;
}