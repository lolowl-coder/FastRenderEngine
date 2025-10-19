#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "Common.h"
#include "Payload.h"
#include "Rnd.h"
#include "Transform.h"

layout(location = 0) rayPayloadInEXT Payload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec3 attribs;

// clang-format off
layout(buffer_reference, scalar) buffer Vertices { Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices { uvec3 i[]; }; // Triangle indices

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 5, scalar) uniform DynamicData { DynamicDataBlock dynamicData; };
layout(set = 0, binding = 6, scalar) buffer SceneDesc { Mesh i[]; } sceneDesc;
layout(set = 0, binding = 7, scalar) buffer GlobalMaterials { Material i[]; } materials;
// Scene textures
layout(set = 0, binding = 8) uniform sampler2D textures[];
// Emissive objects triangles
layout(set = 0, binding = 9, scalar) buffer EmissiveTriangles {EmissiveTriangle L[];} emissiveTriangles;
// clang-format on

// --------------------------- math helpers -----------------------------------
const float PI = 3.14159265359;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturate(vec3  v) { return clamp(v, vec3(0.0), vec3(1.0)); }

float getLod(int texId)
{
    // Compute mip level
    const float distanceRatio = dynamicData.mRTSettings.w;
    float lod = clamp(log2(gl_HitTEXT * distanceRatio), 0.0, float(textureQueryLevels(textures[texId]) - 1));

    return lod;
}

// --------------------------- normal mapping ---------------------------------
// Inputs: interpolated geometric normal/tangent in OBJECT space and UV
// Tangent.w is expected to be the handedness (+1 / -1). If you don't have it, use +1.
vec3 getWorldNormal(Material mat, int normalTexIndex,
    vec3 objN, vec4 objT, vec2 uv,
    mat4x3 objectToWorld, out vec3 T, out vec3 B, out vec3 N, out vec3 nTex, out float nLen)
{
    // Orthonormalize T against N
    N = normalize(objN);
    T = normalize(objT.xyz - N * dot(objT.xyz, N));
    B = normalize(cross(N, T)) * (objT.w >= 0.0 ? 1.0 : -1.0);

    mat3 TBN = mat3(T, B, N);

    nTex = vec3(0.0, 0.0, 1.0);
    if(normalTexIndex >= 0) {
        // Tangent-space normal in [0,1] -> [-1,1]
		float lod = getLod(normalTexIndex);
        nTex = textureLod(textures[normalTexIndex], uv, lod).xyz * 2.0 - 1.0;
		nLen = length(nTex);
        nTex.xy *= mat.mNormalScale;
        nTex = normalize(nTex);
    }

    // To OBJECT space
    vec3 nObj = normalize(TBN * nTex);
    // To WORLD space (w = 0 for direction)
    vec3 nWorld = normalize(inverse(transpose(mat3(gl_ObjectToWorldEXT))) * nObj);
    //vec3 nWorld = normalize((objectToWorld * vec4(nObj, 0.0)).xyz);
    return nWorld;
}

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

// --------------------------- texture sampling --------------------------------
vec4 sampleBaseColor(const Material m, vec2 uv) {
    vec4 base = m.mBaseColorFactor;
    if(m.mBaseColorTex >= 0) {
        // Base color is typically authored in sRGB; ensure your sampler/format handles sRGB -> linear
        float lod = getLod(m.mBaseColorTex);
        base *= textureLod(textures[m.mBaseColorTex], uv, lod);
    }
    return base;
}

vec2 sampleMetallicRoughness(const Material m, vec2 uv) {
    float metallic = m.mMetallicFactor;
    float roughness = m.mRoughnessFactor;
    if(m.mMetallicRoughnessTex >= 0)
    {
		float lod = getLod(m.mMetallicRoughnessTex);
        vec4 mr = textureLod(textures[m.mMetallicRoughnessTex], uv, lod);
        roughness *= mr.g;
        metallic *= mr.b;
    }
    // Avoid zero roughness (can cause fireflies); clamp to a small floor
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = saturate(metallic);
    return vec2(metallic, roughness);
}

float sampleAO(const Material m, vec2 uv) {
    //In ray tracing AO is calculated automatically.
    //Later, we can use this texture to reduce samples count.
    return 1.0;
    if(m.mOcclusionTex >= 0)
    {
		float lod = getLod(m.mOcclusionTex);
        float ao = textureLod(textures[m.mOcclusionTex], uv, lod).r;
        return mix(1.0, ao, m.mOcclusionStrength);
    }
    return 1.0;
}

vec3 sampleEmissive(const Material m, vec2 uv) {
    vec3 e = m.mEmissiveFactor;
    if(m.mEmissiveTex >= 0) {
        // Emissive is authored in sRGB; ensure your sampler/format linearizes
		float lod = getLod(m.mEmissiveTex);
        e *= textureLod(textures[m.mEmissiveTex], uv, lod).rgb;
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

void clampFireflies(inout vec3 color)
{
    float lum = dot(color, vec3(1.0F / 3.0F));
    if(lum > dynamicData.mFireflyThreshold)
    {
        color *= dynamicData.mFireflyThreshold / lum;
    }
}

// --------------------------- Full shading entry ------------------------------
// Example directional light. For point/spot, compute L and radiance accordingly.
vec3 shadeGLTF(
    Material m,
    vec2 objUV,
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
    float aoTex = sampleAO(m, objUV);
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
    float af = getAmbientLightIntensity(dynamicData);
    // Simple ambient term (can be replaced with IBL)
    vec3 ambient = I.baseColor * (1.0 - I.metallic) * af * ao;

    // Alpha (if you need it for blending/masking)
    // float alpha = base.a;

    vec3 result = Lo + ambient + emissive;
    clampFireflies(result);
    return result;
}

// Get a random emissive triangle that faces the view direction V
// V - reversed ray direction (origin - hit position)
// e1, e2 - triangle edges (v1-v0, v2-v0)
EmissiveTriangle getEmissiveTriangle(int sampleIdx, vec3 P, out vec2 lightUV, out vec3 n, out vec3 e1, out vec3 e2, out vec3 xL)
{
    EmissiveTriangle result;
    //for(int i = 0; i < 10; i++)
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
        lightUV.x = halton(seqForBary1, 2u); // base 2
        lightUV.y = halton(seqForBary2, 3u); // base 3

        // Pick emissive triangle index using u
        uint triIdx = uint(u_tri * float(emissiveTriangles.L.length()));
#else
        //lightUV = vec2(rng(payload.rngState), rng(payload.rngState));
        //uint triIdx = uint(floor(rng(payload.rngState) * float(emissiveTriangles.L.length())));
#endif
        triIdx = clamp(triIdx, 0u, uint(emissiveTriangles.L.length() - 1));
        //triIdx = 10;
        //lightUV = vec2(0.5);
        // Barycentric coords inside triangle using (u,v)
        float su = sqrt(lightUV.y);
        float b0 = 1.0 - su;
        float b1 = lightUV.x * su;
        float b2 = 1.0 - b0 - b1;

        result = emissiveTriangles.L[triIdx];

        lightUV = result.uv0 * b0 + result.uv1 * b1 + result.uv2 * b2;
        xL = result.v0 * b0 + result.v1 * b1 + result.v2 * b2;
        //xL = result.v0;
        vec3 v0 = result.v0;
        vec3 v1 = result.v1;
        vec3 v2 = result.v2;
        e1 = v1 - v0;
        e2 = v2 - v0;

        n = normalize(cross(e1, e2));
        /*if(dot(normalize(P - xL), n) > 0.0)
        {
            break;
        }*/
    }

    return result;
}

//Next event estimation for emissive triangles
vec3 nee(
    int sampleIdx, Material objMat, vec2 objUV, vec3 P, vec3 N, vec3 V, vec4 base,
    vec3 emissive, vec2 mr)
{
    vec3 Lo_direct = vec3(0.0);

    // --- NEXT-EVENT ESTIMATION (single sample) ---
    if(emissiveTriangles.L.length() > 0)
    {
        vec3 e1;
        vec3 e2;
        vec3 xL;
        vec2 lightUV;
        vec3 nTri;
        // 1) Get random emissive triangle
        EmissiveTriangle tri = getEmissiveTriangle(sampleIdx, P, lightUV, nTri, e1, e2, xL);

        // 2) Direction to light sample
        vec3 wi = normalize(xL - P);
        float dist = length(xL - P);

        // 3) Visibility test (shadow ray to xL)
        //    Set tMax to dist - eps so we only count blockers strictly before the light
        bool visible = true;
        {
            const uint flags =
                gl_RayFlagsTerminateOnFirstHitEXT |
                //gl_RayFlagsOpaqueEXT |
                gl_RayFlagsCullBackFacingTrianglesEXT |
                gl_RayFlagsSkipClosestHitShaderEXT;

            // payload at location=1 is a boolean 'isShadowed' as in your code
            isShadowed = true; // assume blocked

            traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1,
                P, 0.002, wi, dist * 0.93, 1);
            visible = !isShadowed;
        }

        if(visible)
        {
            // 4) Geometry term (need light normal; approximate from triangle)
            float cosLo = max(dot(N, wi), 0.0);
            float cosLi = max(dot(nTri, -wi), 0.0);

            // 5) BRDF at the hit point toward the light
            Material emissiveTriMat = materials.i[tri.matIndex];
            vec3 lightEmissive = sampleEmissive(emissiveTriMat, lightUV);
            vec3 f = shadeGLTF(
                objMat, objUV, P, V, gl_ObjectToWorldEXT,
                wi, lightEmissive, // light direction & radiance
                float(!isShadowed), // shadow visibility (1 = unshadowed)
                1.0, // ambient occlusion
                N,
                base,
                mr,
                emissive
            );

            // 6) PDF for this sampling strategy:
            //    uniform over triangles + uniform over area of chosen tri
            //    pdf_A = (1 / numTris) * (1 / area)
            float numTris = float(emissiveTriangles.L.length());
            float pdfA = (1.0 / numTris) * (1.0 / max(tri.area, 1e-8));

            // Convert area-PDF to solid-angle factor via geometry term
            float dist2 = max(dist * dist, 1e-6);
            Lo_direct += f * (cosLo * cosLi) / (max(dist2 * pdfA, 1e-6));
            clampFireflies(Lo_direct);
        }
    }

    return Lo_direct;
}

void processEmissives(Material objMat, vec2 objUV, vec3 P, vec3 N_world, vec3 V_world, vec4 base, vec3 emissive, vec2 mr)
{
    vec3 neeEmissiveContrib = vec3(0.0);
    for(int i = 0; i < int(dynamicData.mRTSettings.z); i++)
    {
        neeEmissiveContrib += nee(i, objMat, objUV, P, N_world, V_world, base, emissive, mr);
    }
    neeEmissiveContrib /= dynamicData.mRTSettings.z;
    payload.radiance += neeEmissiveContrib;
}

void processGI(vec3 P, vec3 T, vec3 B, vec3 N)
{
    vec3 giContrib = vec3(0.0);
    Payload old = payload;
    payload.depth++;
    for(int i = 0; i < int(dynamicData.mRTSettings.y); i++)
    {
        float tMin = 0.001;
        // infinite
        float tMax = 1e32;
        vec3 origin = P;

        vec3 rayDir = normalize(mat3(gl_ObjectToWorldEXT) * toTBNSpace(cosineHemisphere(i), N, T, B));
        uint flags =
            gl_RayFlagsTerminateOnFirstHitEXT |
            gl_RayFlagsCullBackFacingTrianglesEXT;
            //gl_RayFlagsOpaqueEXT;

        payload.radiance = vec3(0.0);
        payload.attenuation = vec3(1.0);

        traceRayEXT(topLevelAS,        // acceleration structure
            flags,             // rayFlags
            0xFF,              // cullMask
            0,                 // sbtRecordOffset
            0,                 // sbtRecordStride
            0,                 // missIndex
            origin,            // ray origin
            tMin,              // ray min range
            rayDir,            // ray direction
            tMax,              // ray max range
            0                  // payload (location = 0, 1)
        );
        giContrib += min(vec3(2.5), payload.radiance);
        clampFireflies(giContrib);
    }
    giContrib /= dynamicData.mRTSettings.y;
    payload = old;
    payload.radiance += giContrib;
}

void main()
{
    if(payload.depth > 1)
    {
        return;
    }

    // When contructing the TLAS, we stored the model id in InstanceCustomIndexEXT, so the
    // the instance can quickly have access to the data

    // Object data
    Mesh mesh = sceneDesc.i[gl_InstanceCustomIndexEXT];
    Indices indices = Indices(mesh.mIndices);
    Vertices vertices = Vertices(mesh.mVertices);

    //Mesh material
    Material objMat = materials.i[mesh.mMaterialIndex];

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

    vec2 objUV = uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

    // Computing the normal at hit position
    vec3 vNormal = v0.mNormal.xyz * barycentrics.x + v1.mNormal.xyz * barycentrics.y + v2.mNormal.xyz * barycentrics.z;
    vec3 vTangent = v0.mTangent.xyz * barycentrics.x + v1.mTangent.xyz * barycentrics.y + v2.mTangent.xyz * barycentrics.z;
    vec3 T;
    vec3 B;
    vec3 N;
    vec3 nTex;
    float nLen;
    vec3 N_world = getWorldNormal(objMat, objMat.mNormalTex, vNormal, vec4(vTangent, 1.0), objUV, gl_ObjectToWorldEXT, T, B, N, nTex, nLen);

    // Computing the coordinates of the hit position
    vec3 P = v0.mPos.xyz * barycentrics.x + v1.mPos.xyz * barycentrics.y + v2.mPos.xyz * barycentrics.z;
    P = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

    // Hardocded light position
    vec3 lightPos = dynamicData.mLightPos.xyz;
    // To light direction
    vec3 L = normalize(lightPos - P);

    float NdotL = dot(N_world, L);
    vec2 mr = sampleMetallicRoughness(objMat, objUV);
    float metallness = mr.x;
    float roughness = mr.y;

    /*if(dynamicData.mUseToksvig > 0)
    {
        float variance = dot(nTex, nTex) - 1.0; // local deviation measure
        float filteredRoughness = sqrt(roughness * roughness + variance);
        mr.y = filteredRoughness;
    }

    float toksvigFactor = (1.0 - nLen) / max(nLen, 1e-6);
    roughness = sqrt(roughness * roughness + dynamicData.useToksvig * toksvigFactor);

    // Clamp to valid range
    roughness = clamp(roughness, 0.0, 1.0);

    mr.y = roughness;*/

    // Tracing shadow ray only if the light is visible from the surface
    // TODO: learn more about back face check. Not shure if we need it here
    if(NdotL > 0.0)
    {
        float tMin = 0.001;
        float tMax = 1e32;        // infinite
        vec3  origin = P;
        vec3  rayDir = L;
        uint  flags = gl_RayFlagsTerminateOnFirstHitEXT /*| gl_RayFlagsOpaqueEXT*/ | gl_RayFlagsSkipClosestHitShaderEXT;
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
    vec4 base = sampleBaseColor(objMat, objUV);
    payload.albedo = base.rgb;
    vec3 emissive = sampleEmissive(objMat, objUV);
    vec3 lightRadiance = dynamicData.mLightColor.rgb * getMainLightIntensity(dynamicData); // light radiance at P (includes intensity & attenuation)
    vec3 directLightContrib = shadeGLTF(
        objMat, objUV, P, V_world, gl_ObjectToWorldEXT,
        L, lightRadiance, // light direction & radiance
        float(!isShadowed), // shadow visibility (1 = unshadowed)
        1.0, // ambient occlusion
        N_world,
        base,
        mr,
        emissive
    );

    payload.radiance += directLightContrib;
    
    if(getAreaLightsEnabled(dynamicData) && payload.depth == 0)
    {
        processEmissives(objMat, objUV, P, N_world, V_world, base, emissive, mr);
    }

    if(getGIEnabled(dynamicData) && payload.depth == 0)
    {
        processGI(P, T, B, N);
    }

    payload.radiance *= payload.attenuation;

	// Reflect
	vec3 rayDir = reflect(gl_WorldRayDirectionEXT, N_world);
    if(payload.depth == 0)
    {
        payload.attenuation *= 0.5 * metallness;
    }
    payload.rayOrigin = P;
    payload.rayDir = rayDir;
    payload.normal = N_world;

    if(dynamicData.mDebugMode == 1)
    {
		payload.radiance = N_world;
    }
    else if(dynamicData.mDebugMode == 2)
    {
        payload.radiance = payload.albedo;
    }
    else if(dynamicData.mDebugMode == 3)
    {
        payload.radiance = vec3(mr.y);
	}
    else if(dynamicData.mDebugMode == 4)
    {
        payload.radiance = vec3(mr.x);
	}
    else if(dynamicData.mDebugMode == 5)
    {
        payload.radiance = emissive;
	}
    else if(dynamicData.mDebugMode == 6)
    {
        payload.radiance = vec3(gl_HitTEXT / 10.0);
	}
}