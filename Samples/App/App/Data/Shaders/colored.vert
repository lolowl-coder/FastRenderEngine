#version 460

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

layout(location = 0) out float fragPosZ;

void main()
{
	vec4 worldPos = pushModel.model * vec4(vec3(pos.x, pos.y, pos.z * 5.0), 1.0);
	gl_Position = uboViewProjection.projection * uboViewProjection.view * worldPos;
	fragPosZ = pos.z;
}
