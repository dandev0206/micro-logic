#version 450 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

layout(push_constant) uniform uPushConstant {
	mat4 transform;
	vec2 texture_size;
} PC;

layout(location = 0) out struct {
	vec4 color;
	vec2 uv;
} Out;

void main()
{
	Out.color   = color;
	Out.uv      = uv / PC.texture_size;

	gl_Position = PC.transform * vec4(pos, 0, 1);
}