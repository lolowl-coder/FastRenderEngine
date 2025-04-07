#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 tex;

layout(set = 0, binding = 0) uniform UboViewProjection {
	mat4 projection;
	mat4 view;
} uboViewProjection;

//not used
/*layout(set = 0, binding = 1) uniform UboModel {
	mat4 model;
} uboModel;*/

layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 0) out vec2 fragTex;

void main()
{
	gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);
	fragTex = tex;
}
