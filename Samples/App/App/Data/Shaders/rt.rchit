#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

struct Material
{
	vec3 mSpecular;
	float mShininess;
	int mDiffuseMap;
};

struct Vertex
{
	vec3 mPos;
	vec3 mNormal;
	vec3 mTangent;
	vec2 mTC;
};

struct Mesh
{
	uint64_t mVertices;
	uint64_t mIndices;
	int mMaterialIndex;
};

// clang-format off
layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; }; // Triangle indices

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3) buffer SceneDesc { Mesh i[]; } sceneDesc;
layout(set = 0, binding = 4) buffer GlobalMaterials { Material i[]; } materials;
layout(set = 0, binding = 5) uniform sampler2D textures[];
// clang-format on

vec3 computeSpecular(Material mat, vec3 V, vec3 L, vec3 N)
{
	const float kPi = 3.14159265;
	const float kShininess = max(mat.mShininess, 4.0);

	// Specular
	const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	V = normalize(-V);
	vec3  R = reflect(-L, N);
	float specular = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return vec3(mat.mSpecular * specular);
}

void main()
{
	// When contructing the TLAS, we stored the model id in InstanceCustomIndexEXT, so the
	// the instance can quickly have access to the data

	// Object data
	Mesh mesh = sceneDesc.i[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(mesh.mIndices);
	Vertices vertices = Vertices(mesh.mVertices);

	//Mesh material
	Material mat = materials.i[mesh.mMaterialIndex];

	// Indices of the triangle
	uvec3 ind = indices.i[gl_PrimitiveID];

	// Vertex of the triangle
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];

	// Barycentric coordinates of the triangle
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the normal at hit position
	vec3 N = v0.mNormal.xyz * barycentrics.x + v1.mNormal.xyz * barycentrics.y + v2.mNormal.xyz * barycentrics.z;
	N = normalize(vec3(N.xyz * gl_WorldToObjectEXT));        // Transforming the normal to world space

	// Computing the coordinates of the hit position
	vec3 P = v0.mPos.xyz * barycentrics.x + v1.mPos.xyz * barycentrics.y + v2.mPos.xyz * barycentrics.z;
	P = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

	// Hardocded (to) light direction
	vec3 L = normalize(vec3(1, 1, 1));

	float NdotL = dot(N, L);

	// Fake Lambertian to avoid black
	vec2 uv0 = v0.mTC;
	vec2 uv1 = v1.mTC;
	vec2 uv2 = v2.mTC;

	vec2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

	vec3 materialDiffuse = texture(textures[mat.mDiffuseMap], uv).rgb;

	vec3 diffuse = materialDiffuse * max(NdotL, 0.3);
	vec3 specular = vec3(0);

	// Tracing shadow ray only if the light is visible from the surface
	if(NdotL > 0)
	{
		float tMin = 0.001;
		float tMax = 1e32;        // infinite
		vec3  origin = P;
		vec3  rayDir = L;
		uint  flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
		isShadowed = true;

		traceRayEXT(topLevelAS,        // acceleration structure
			flags,             // rayFlags
			0xFF,              // cullMask
			0,                 // sbtRecordOffset
			0,                 // sbtRecordStride
			1,                 // missIndex
			origin,            // ray origin
			tMin,              // ray min range
			rayDir,            // ray direction
			tMax,              // ray max range
			1                  // payload (location = 1)
		);

		if(isShadowed)
			diffuse *= 0.3;
		else
			// Add specular only if not in shadow
			specular = computeSpecular(mat, gl_WorldRayDirectionEXT, L, N);
	}
	prd.radiance = (diffuse + specular) * (1 - mat.mShininess) * prd.attenuation;

	// Reflect
	vec3 rayDir = reflect(gl_WorldRayDirectionEXT, N);
	prd.attenuation *= vec3(mat.mShininess);
	prd.rayOrigin = P;
	prd.rayDir = rayDir;
}

