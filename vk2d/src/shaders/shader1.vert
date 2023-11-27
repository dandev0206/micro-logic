#version 450 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;

layout(push_constant) uniform uPushConstant {
	mat4 transform;
} PC;

layout(location = 0) out struct {
	vec4 color;
} Out;

void main()
{
	Out.color   = color;

	gl_Position = PC.transform * vec4(pos, 0, 1);
}