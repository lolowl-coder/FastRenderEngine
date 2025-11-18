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
    // then for each sample, build a small sequence index
    uint seq = pixHash + sampleIdx;

    float u1 = halton(seq, 2u);
    float u2 = halton(seq, 3u);

    float r = sqrt(u1);
    float phi = 2.0 * 3.14159265 * u2;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));

    return vec3(x, y, z);
}

// Hash function: fast and good enough for value noise
precise float hash(vec2 p)
{
    // Large prime-like numbers
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

// Smooth interpolation
float smoothstep_interp(float t)
{
    return t * t * (3.0 - 2.0 * t);
}

// 2D Value Noise in [0, 1]
float valueNoise2D(vec2 uv)
{
    vec2 i = floor(uv);        // integer cell id
    vec2 f = fract(uv);        // fractional part

    // Smooth interpolation curve
    vec2 u = vec2(
        smoothstep_interp(f.x),
        smoothstep_interp(f.y)
    );

    // Corner values
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Bilinear interpolation
    return mix(
        mix(a, b, u.x),
        mix(c, d, u.x),
        u.y
    );
}

float fbm(vec2 uv)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; ++i)
    {
        value += amplitude * valueNoise2D(uv * frequency);
        frequency *= 1.5;
        amplitude *= 0.5;
    }

    return value;
}

#endif