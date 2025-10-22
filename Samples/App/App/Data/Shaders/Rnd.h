#ifndef _RND_
#define _RND_

uint lcg(inout uint s) { s = 1664525u * s + 1013904223u; return s; }
float rng(inout uint s) { return (lcg(s) >> 8) * (1.0 / 16777216.0); } // [0,1)

// Generates quasi-random value in range [0, 1)
// index: any int value (usually sample index, frame index, etc.)
// base: any prime number
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

// Generates pseudorandom but deterministic scrambled 32-bit integer
uint wangHash(uint x) {
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return x;
}

// Generates random point on upper part of semisphere
vec3 cosineHemisphere(int sampleIdx) {
    uvec2 launchID = gl_LaunchIDEXT.xy;
    uvec2 launchSize = gl_LaunchSizeEXT.xy;
    uint pixelIndex = launchID.x + launchID.y * launchSize.x;
    uint pixHash = wangHash(pixelIndex);
    uint baseSeq = pixHash * 1664525u + 1013904223u; // mixed base seed
    // then for each sample, build a small sequence index
    uint seqForU = baseSeq + uint(sampleIdx) * 4u + 3u; // +0 for tri index

    float u1 = halton(seqForU, 11u);
    float u2 = halton(seqForU, 15u);

    float r = sqrt(u1);
    float phi = 2.0 * 3.14159265 * u2;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));

    return vec3(x, y, z);
}

#endif