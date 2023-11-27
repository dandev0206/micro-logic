#version 450 core

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in struct {
	vec4 color;
	vec2 uv;
} In;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = In.color * texture(sTexture, In.uv);
}