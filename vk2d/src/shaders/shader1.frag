#version 450 core

layout(location = 0) in struct {
	vec4 color;
} In;

layout(location = 0) out vec4 fColor;

void main()
{
	fColor = In.color;
}