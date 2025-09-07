#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#define NUM_NEE_SAMPLES 4

struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
    vec3 lightPos;
    uint rngState;
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

struct EmissiveTriangle
{
    vec3 v0;
    vec3 v1;
    vec3 v2;
    vec2 uv0;
    vec2 uv1;
    vec2 uv2;
    float area;
    int matIndex;
};

// clang-format off
layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; }; // Triangle indices

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3) buffer SceneDesc { Mesh i[]; } sceneDesc;
layout(set = 0, binding = 4, scalar) buffer GlobalMaterials { Material i[]; } materials;
// Scene textures
layout(set = 0, binding = 5) uniform sampler2D textures[];
// Emissive objects triangles
layout(set = 0, binding = 6, scalar) buffer EmissiveTriangles {EmissiveTriangle L[];} emissiveTriangles;
// clang-format on

// --------------------------- math helpers -----------------------------------
const float PI = 3.14159265359;

uint lcg(inout uint s) { s = 1664525u * s + 1013904223u; return s; }
float rng(inout uint s) { return (lcg(s) >> 8) * (1.0 / 16777216.0); } // [0,1)

float halton(uint index, uint base) {
    float f = 1.0;
    float r = 0.0;
    uint i = index;
    while(i > 0u) {
        f = f / float(base);
        r = r + f * float(i % base);
        i = i / base;
    }
    return r;
}

// Wang hash (32-bit) - simple scramble for decorrelation
uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

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
    mat4x3 objectToWorld,
    vec3 lightDir_world,          // normalized (from P toward light)
    vec3 lightRadiance,           // RGB radiance at P (includes intensity & attenuation)
    float shadowVisibility,       // 0..1 (1 = unshadowed). For hard shadow: 0 or 1.
    float ambientOcclusion,       // If you have SSAO/etc. Multiply with AO map if both.
    vec3 N_world,
    vec4 base,
    vec2 mr,
    vec3 emissive
) {
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
    
    //TODO: Transparency disable for now
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
    float af = 0.7; //0.03 by default. Increased because of dark result
    // Simple ambient term (you can replace with IBL)
    vec3 ambient = I.baseColor * (1.0 - I.metallic) * af * ao;

    // Alpha (if you need it for blending/masking)
    // float alpha = base.a;

    return Lo + ambient + emissive;
}

// Get a random emissive triangle that faces the view direction V
// V - reversed ray direction (origin - hit position)
// e1, e2 - triangle edges (v1-v0, v2-v0)
EmissiveTriangle getEmissiveTriangle(int sampleIdx, vec3 P, out vec2 uv, out vec3 n, out vec3 e1, out vec3 e2, out vec3 xL)
{
    EmissiveTriangle result;
    for(int i = 0; i < 10; i++)
    {
        // Pick a random emissive triangle (uniform over triangles for now)

#if 1
        uvec2 launchID = gl_LaunchIDEXT.xy;
        uvec2 launchSize = gl_LaunchSizeEXT.xy;
        uint pixelIndex = launchID.x + launchID.y * launchSize.x;

        uint pixHash = wangHash(pixelIndex);
        uint baseSeq = pixHash * 1664525u + 1013904223u; // mixed base seed

        // then for each sample, build a small sequence index
        uint seqForTri = baseSeq + uint(sampleIdx) * 4u + 0u; // +0 for tri index
        uint seqForBary1 = baseSeq + uint(sampleIdx) * 4u + 1u; // +1 for bary u
        uint seqForBary2 = baseSeq + uint(sampleIdx) * 4u + 2u; // +2 for bary v

        float u_tri = halton(seqForTri, 7u);   // choose base 7 (avoid 2/3 correlation)
        uv.x = halton(seqForBary1, 2u); // base 2
        uv.y = halton(seqForBary2, 3u); // base 3

        // Pick emissive triangle index using u
        uint triIdx = uint(u_tri * float(emissiveTriangles.L.length()));
#else
        uv = vec2(rng(prd.rngState), rng(prd.rngState));
        uint triIdx = uint(floor(rng(prd.rngState) * float(emissiveTriangles.L.length())));
#endif
        triIdx = clamp(triIdx, 0u, uint(emissiveTriangles.L.length() - 1));
        //triIdx = 10;
        //uv = vec2(0.5);
        // Barycentric coords inside triangle using (u,v)
        float su = sqrt(uv.y);
        float b0 = 1.0 - su;
        float b1 = uv.x * su;
        float b2 = 1.0 - b0 - b1;

        result = emissiveTriangles.L[triIdx];

        uv = result.uv0 * b0 + result.uv1 * b1 + result.uv2 * b2;
        xL = result.v0 * b0 + result.v1 * b1 + result.v2 * b2;
        //xL = result.v0;
        vec3 v0 = result.v0;
        vec3 v1 = result.v1;
        vec3 v2 = result.v2;
        e1 = v1 - v0;
        e2 = v2 - v0;

        n = normalize(cross(e1, e2));
        if(dot(normalize(P - xL), n) > 0.0)
        {
            break;
        }
    }

    return result;
}

//Next event estimation for emissive triangles
vec3 nee(int sampleIdx, vec3 P, vec3 N, vec3 V, vec4 base, vec2 mr)
{
    vec3 Lo_direct = vec3(0.0);

    // --- NEXT-EVENT ESTIMATION (single sample) ---
    if(emissiveTriangles.L.length() > 0)
    {
        vec3 e1;
        vec3 e2;
        vec3 xL;
        vec2 uv;
        vec3 nTri;
        EmissiveTriangle tri = getEmissiveTriangle(sampleIdx, P, uv, nTri, e1, e2, xL);

        Material emissiveTriMat = materials.i[tri.matIndex];
        vec3 lightEmissive = sampleEmissive(emissiveTriMat, uv) * 3.5;

        // direction to light sample
        vec3 wi = normalize(xL - P);
        float dist = length(xL - P);
        float dist2 = max(dist * dist, 1e-6);

        // 3) Visibility test (shadow ray to xL)
        //    Set tMax to dist - eps so we only count blockers strictly before the light
        bool visible = true;
        {
            const uint flags = gl_RayFlagsTerminateOnFirstHitEXT |
                gl_RayFlagsOpaqueEXT |
                gl_RayFlagsSkipClosestHitShaderEXT;

            // payload at location=1 is a boolean 'isShadowed' as in your code
            isShadowed = true; // assume blocked
            //traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1,
            //    P, 0.001, wi, dist - 0.002, 1);

            traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1,
                P, dist * 0.05, wi, dist * 0.93, 1);
            visible = !isShadowed;
        }

        if(visible)
        {
            // 4) Geometry term (need light normal; approximate from triangle)
            float cosLo = max(dot(N, wi), 0.0);
            float cosLi = max(dot(nTri, -wi), 0.0);

            // 5) BRDF at the hit point toward the light
            //    Use your existing eval for GGX/Lambert:
            PBRInputs I;
            I.N = normalize(N);
            I.V = normalize(V);
            I.L = normalize(wi);
            I.radiance = lightEmissive;
            I.baseColor = base.rgb;
            I.metallic = mr.x;
            I.roughness = mr.y;
            vec3 f = BRDF_PBR(I);

            // 6) PDF for this sampling strategy:
            //    uniform over triangles + uniform over area of chosen tri
            //    pdf_A = (1 / numTris) * (1 / area)
            float numTris = float(emissiveTriangles.L.length());
            float pdfA = (1.0 / numTris) * (1.0 / max(tri.area, 1e-8));

            // Convert area-PDF to solid-angle factor via geometry term
            //Lo_direct += lightEmissive * f * (cosLo * cosLi) / (max(dist2 * pdfA, 1e-6));
            vec3 contrib = f * cosLi / max(dist2 * pdfA, 1e-6);

            Lo_direct += contrib;

            //return f;
            //return vec3(dist2) / 20.0;
            //return vec3(1.0);
        }
        /*else
        {
            return vec3(1.0, 0.0, 0.0);
        }*/
    }

    return Lo_direct;
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
	vec3 lightPos = vec3(1.5, 0.8, 0.5);
	// To light direction
	vec3 L = normalize(lightPos - P);

	float NdotL = dot(N_world, L);

	float metallness = sampleMetallicRoughness(mat, uv).x;
	// Tracing shadow ray only if the light is visible from the surface
    // TODO: learn more about back fack check. Not shure if we need it here
	if(NdotL > 0.0)
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

    vec3 V_world = normalize(gl_WorldRayOriginEXT - P);
    vec4 base = sampleBaseColor(mat, uv);
    vec2 mr = sampleMetallicRoughness(mat, uv);
    vec3 emissive = sampleEmissive(mat, uv);
    float shadowVisibility = 1.0 - (isShadowed ? 1.0 : 0.0);
    vec3 lightRadiance = vec3(1.0, 1.0, 0.9) * 2.5; // light radiance at P (includes intensity & attenuation)
    prd.radiance = shadeGLTF(
        mat, uv, P, V_world, gl_ObjectToWorldEXT,
        L, lightRadiance, // light direction & radiance
        shadowVisibility, // shadow visibility (1 = unshadowed)
        1.0, // ambient occlusion
        N_world,
        base,
        mr,
        emissive
    );

    vec3 emissiveExt = vec3(0.0);
    for(int i = 0; i < NUM_NEE_SAMPLES; i++)
    {
        emissiveExt += nee(i, P, N_world, V_world, base, mr);
    }
    emissiveExt /= float(NUM_NEE_SAMPLES);
    //emissiveExt += vec3(float(effectiveSamplesCount) / float(NUM_NEE_SAMPLES));
    prd.radiance += emissiveExt;
    prd.radiance *= prd.attenuation;
    //prd.radiance = min(prd.radiance, vec3(1e3)); // avoid fireflies (depends on scene)

	// Reflect
	vec3 rayDir = reflect(gl_WorldRayDirectionEXT, N_world);
    prd.attenuation *= 0.8 * (metallness);
	prd.rayOrigin = P;
	prd.rayDir = rayDir;
}

