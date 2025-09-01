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
    vec3 lightPos;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

struct Material
{
	vec4  mBaseColorFactor;      // RGBA
	float mMetallicFactor;       // [0,1]
	float mRoughnessFactor;      // [0,1]
	float mNormalScale;          // normal map scale (1 = as is)
	float mOcclusionStrength;    // [0,1]
	vec3  mEmissiveFactor;       // RGB
	int mBaseColorTex;         // -1 if none
	int mMetallicRoughnessTex; // -1 if none (G=roughness, B=metallic)
	int mNormalTex;            // -1 if none (tangent space)
	int mOcclusionTex;         // -1 if none (R)
	int mEmissiveTex;          // -1 if none (RGB)
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
layout(set = 0, binding = 4, scalar) buffer GlobalMaterials { Material i[]; } materials;
layout(set = 0, binding = 5) uniform sampler2D textures[];
// clang-format on

// --------------------------- math helpers -----------------------------------
const float PI = 3.14159265359;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturate(vec3  v) { return clamp(v, vec3(0.0), vec3(1.0)); }

// Trowbridge-Reitz GGX normal distribution
float D_GGX(float NdotH, float a) {
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// Smith masking-shadowing using Schlick-GGX for both terms
float G_SchlickGGX(float NdotV, float k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    // k for direct lighting (correlated Smith)
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return G_SchlickGGX(NdotV, k) * G_SchlickGGX(NdotL, k);
}

// Fresnel-Schlick (with optional roughness variant for grazing)
vec3 F_Schlick(vec3 F0, float HdotV) {
    float f = pow(1.0 - HdotV, 5.0);
    return F0 + (1.0 - F0) * f;
}

// --------------------------- normal mapping ---------------------------------
// Inputs: interpolated geometric normal/tangent in OBJECT space and UV
// Tangent.w is expected to be the handedness (+1 / -1). If you don't have it, use +1.
vec3 getWorldNormal(Material mat, int normalTexIndex,
    vec3 objN, vec4 objT, vec2 uv,
    mat4x3 objectToWorld)
{
    // Orthonormalize T against N
    vec3 N = normalize(objN);
    vec3 T = normalize(objT.xyz - N * dot(objT.xyz, N));
    vec3 B = normalize(cross(N, T)) * (objT.w >= 0.0 ? 1.0 : -1.0);

    mat3 TBN = mat3(T, B, N);

    vec3 n = vec3(0.0, 0.0, 1.0);
    if(normalTexIndex >= 0) {
        // Tangent-space normal in [0,1] -> [-1,1]
        n = texture(textures[normalTexIndex], uv).xyz * 2.0 - 1.0;
        n.xy *= mat.mNormalScale;
        n = normalize(n);
    }

    // To OBJECT space
    vec3 nObj = normalize(TBN * n);
    // To WORLD space (w = 0 for direction)
    vec3 nWorld = normalize((objectToWorld * vec4(nObj, 0.0)).xyz);
    return nWorld;
}

// --------------------------- texture sampling --------------------------------
vec4 sampleBaseColor(const Material m, vec2 uv) {
    vec4 base = m.mBaseColorFactor;
    if(m.mBaseColorTex >= 0) {
        // Base color is typically authored in sRGB; ensure your sampler/format handles sRGB -> linear
        base *= texture(textures[m.mBaseColorTex], uv);
    }
    return base;
}

vec2 sampleMetallicRoughness(const Material m, vec2 uv) {
    float metallic = m.mMetallicFactor;
    float roughness = m.mRoughnessFactor;
    if(m.mMetallicRoughnessTex >= 0) {
        vec4 mr = texture(textures[m.mMetallicRoughnessTex], uv);
        roughness *= mr.g;
        metallic *= mr.b;
    }
    // Avoid zero roughness (can cause fireflies); clamp to a small floor
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = saturate(metallic);
    return vec2(metallic, roughness);
}

float sampleAO(const Material m, vec2 uv) {
    if(m.mOcclusionTex >= 0) {
        float ao = texture(textures[m.mOcclusionTex], uv).r;
        return mix(1.0, ao, m.mOcclusionStrength);
    }
    return 1.0;
}

vec3 sampleEmissive(const Material m, vec2 uv) {
    vec3 e = m.mEmissiveFactor;
    if(m.mEmissiveTex >= 0) {
        // Emissive is authored in sRGB; ensure your sampler/format linearizes
        e *= texture(textures[m.mEmissiveTex], uv).rgb;
    }
    return e;
}

// --------------------------- BRDF core ---------------------------------------
struct PBRInputs {
    vec3 N;        // world-space normal
    vec3 V;        // world-space view dir (from P toward camera), normalized
    vec3 L;        // world-space light dir (from P toward light), normalized
    vec3 radiance; // light radiance (RGB), already includes attenuation
    vec3 baseColor;
    float metallic;
    float roughness;
};

vec3 BRDF_PBR(const PBRInputs I) {
    vec3 H = normalize(I.V + I.L);

    float NdotL = saturate(dot(I.N, I.L));
    float NdotV = saturate(dot(I.N, I.V));
    float NdotH = saturate(dot(I.N, H));
    float HdotV = saturate(dot(H, I.V));

    if(NdotL <= 0.0 || NdotV <= 0.0)
        return vec3(0.0);

    // Dielectric F0 ~ 0.04; metals use baseColor as F0
    vec3 F0 = mix(vec3(0.04), I.baseColor, I.metallic);

    float a = I.roughness * I.roughness; // perceptual -> alpha
    float D = D_GGX(NdotH, a);
    float G = G_Smith(NdotV, NdotL, I.roughness);
    vec3  F = F_Schlick(F0, HdotV);

    vec3  spec = (D * G * F) / max(4.0 * NdotL * NdotV, 1e-4);

    // Lambert diffuse, energy-conserving with metallic
    vec3 kd = (1.0 - F) * (1.0 - I.metallic);
    vec3 diff = kd * I.baseColor / PI;

    return (diff + spec) * I.radiance * NdotL;
}

// --------------------------- Full shading entry ------------------------------
// Example directional light. For point/spot, compute L and radiance accordingly.
vec3 shadeGLTF(
    Material m,
    vec2 uv,
    vec3 P_world,
    vec3 V_world,                 // from P toward camera, normalized
    vec3 geomN_obj, vec4 tangent_obj, // interpolated geometric normal/tangent in OBJECT space
    mat4x3 objectToWorld,
    vec3 lightDir_world,          // normalized (from P toward light)
    vec3 lightRadiance,           // RGB radiance at P (includes intensity & attenuation)
    float shadowVisibility,       // 0..1 (1 = unshadowed). For hard shadow: 0 or 1.
    float ambientOcclusion,       // If you have SSAO/etc. Multiply with AO map if both.
    vec3 N_world,
    out vec3 emissive
) {
    // Sample material inputs
    vec4 base = sampleBaseColor(m, uv);
    vec2 mr = sampleMetallicRoughness(m, uv);
    float metallic = mr.x;
    float roughness = mr.y;

    // Optional AO (texture * screen-space AO)
    float aoTex = sampleAO(m, uv);
    float ao = clamp(ambientOcclusion * aoTex, 0.0, 1.0);

    // Build BRDF inputs
    PBRInputs I;
    I.N = normalize(N_world);
    I.V = normalize(V_world);
    I.L = normalize(lightDir_world);
    I.radiance = lightRadiance * shadowVisibility; // shadow term here
    I.baseColor = base.rgb;
    I.metallic = metallic;
    I.roughness = roughness;

    vec3 Lo = BRDF_PBR(I);
    
    /*float alpha = base.a;
    // Alpha handling
    if(material.alphaMode == ALPHA_MASK){
        if(alpha < material.alphaCutoff) {
            // Fully discard hit
            return vec3(0.0); // Or call ignoreIntersectionEXT() if available
        }
    }
    else if(material.alphaMode == ALPHA_BLEND) {
        // For blending, return surface color but scale by alpha
        // (in a full path tracer, you'd probabilistically transmit or absorb)
        Lo *= alpha;
    }*/


    // Ambient factor
    float af = 0.7; //0.03 by default
    // Simple ambient term (you can replace with IBL)
    vec3 ambient = I.baseColor * (1.0 - I.metallic) * af * ao;

    // Emissive
    emissive = sampleEmissive(m, uv);

    // Alpha (if you need it for blending/masking)
    // float alpha = base.a;

    //return emissive;
    return Lo + ambient + emissive;
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

    // Texture coordinates at hit position
	vec2 uv0 = v0.mTC;
	vec2 uv1 = v1.mTC;
	vec2 uv2 = v2.mTC;

	vec2 uv = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

	// Computing the normal at hit position
	vec3 vNormal = v0.mNormal.xyz * barycentrics.x + v1.mNormal.xyz * barycentrics.y + v2.mNormal.xyz * barycentrics.z;
	vec3 vTangent = v0.mTangent.xyz * barycentrics.x + v1.mTangent.xyz * barycentrics.y + v2.mTangent.xyz * barycentrics.z;
    vec3 N_world = getWorldNormal(mat, mat.mNormalTex, vNormal, vec4(vTangent, 1.0), uv, gl_ObjectToWorldEXT);

	// Computing the coordinates of the hit position
	vec3 P = v0.mPos.xyz * barycentrics.x + v1.mPos.xyz * barycentrics.y + v2.mPos.xyz * barycentrics.z;
	P = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

	// Hardocded light position
	//vec3 lightPos = vec3(1.5, 0.8, 0.5);
	vec3 lightPos = vec3(1.5, 0.8, 0.5);
	// To light direction
	vec3 L = normalize(lightPos - P);

	float NdotL = dot(N_world, L);

	//vec3 materialDiffuse = texture(textures[mat.mBaseColorTex], uv).rgb;
	//vec3 diffuse = materialDiffuse * max(NdotL, 0.5);
	//vec3 specular = vec3(0.0);

	float metallness = sampleMetallicRoughness(mat, uv).x;
	// Tracing shadow ray only if the light is visible from the surface
	//if(NdotL > 0.0)
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
	}

    N_world;
    vec3 V_world = normalize(gl_WorldRayOriginEXT - P);
    vec3 emissive;
    prd.radiance = shadeGLTF(mat, uv, P, V_world, vNormal, vec4(vTangent, 1.0), gl_ObjectToWorldEXT,
        L, vec3(1.0, 1.0, 0.9) * 2.5, // light direction & radiance
        1.0 - (isShadowed ? 1.0 : 0.0), // shadow visibility (1 = unshadowed)
        1.0, // ambient occlusion
        N_world,
        emissive
    );

    //int mBaseColorTex;         // -1 if none
    //int mMetallicRoughnessTex; // -1 if none (G=roughness, B=metallic)
    //int mNormalTex;            // -1 if none (tangent space)
    //int mOcclusionTex;         // -1 if none (R)
    //int mEmissiveTex;          // -1 if none (RGB)*/
    //prd.radiance = N_world;

	// Reflect
    //prd.radiance = vec3(isEmissive);
	vec3 rayDir = reflect(gl_WorldRayDirectionEXT, N_world);
    prd.attenuation *= 0.8 * metallness;
	prd.rayOrigin = P;
	prd.rayDir = rayDir;
}

