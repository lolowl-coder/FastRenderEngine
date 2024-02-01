#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 tex;

layout(set = 0, binding = 0) uniform UboViewProjection {
	mat4 projection;
	mat4 view;
} uboViewProjection;

layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec2 fragTex;

void main()
{
	vec4 worldPos = pushModel.model * vec4(pos, 1.0);
	gl_Position = uboViewProjection.projection * uboViewProjection.view * worldPos;
	fragPos = worldPos.xyz;
	fragNormal = mat3(pushModel.model) * normal;
	fragTangent = mat3(pushModel.model) * tangent;
	fragTex = tex;
}
