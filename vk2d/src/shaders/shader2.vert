#version 450 core


layout(push_constant) uniform uPushConstant {
	mat4 transform;
	vec4 color;
	vec2 vertices[4];
	vec2 uvs[4];
} PC;

layout(location = 0) out struct {
	vec4 color;
	vec2 uv;
} Out;

void main()
{
	Out.color = PC.color;
	Out.uv    = PC.uvs[gl_VertexIndex];

	gl_Position = PC.transform * vec4(PC.vertices[gl_VertexIndex], 0, 1);
}