#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 tex;

layout(push_constant) uniform PushModel {
	mat4 model;
	mat4 view;
	mat4 projection;
} pm;

layout(location = 0) out vec2 fragTex;

void main()
{
	gl_Position = pm.projection * pm.view * pm.model * vec4(pos, 1.0);
	fragTex = tex;
}
