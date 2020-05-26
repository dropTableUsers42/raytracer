#version 430 core

#if GL_NV_gpu_shader5
#extension GL_NV_gpu_shader5 : enable
#elif GL_EXT_shader_8bit_storage
#extension GL_EXT_shader_8bit_storage : enable
#endif

layout(location = 0, binding = 0, rgba32f) uniform image2D framebuffer;
layout(location = 1, binding = 1, rgba8_snorm) uniform image2D normbuffer;
layout(location = 2, binding = 2, rgba16f) uniform image2D posbuffer;
uniform sampler2D blueNoiseTex;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

uniform float time;

uniform float blendFactor;

uniform bool multipleImportanceSampled;

uniform bool importanceSampled;

uniform bool useBlueNoise;

uniform float phongExponent;

uniform float specularFactor;

uniform int frameIndex;

struct box {
    vec3 min;
    vec3 max;
    vec3 col;
};

struct rectangle {
    vec3 c;
    vec3 x;
    vec3 y;
};

struct sphere {
    vec3 c;
    float r;
};

#define NUM_BOXES 11
#define NUM_SPHERES 1
#define NUM_RECTANGLES 1
#define LARGE_FLOAT 1E+10
#define EPSILON 1E-4
#define BOUNCES 5

#define PI radians(180.0)
#define ONE_OVER_PI 1.0/PI
#define ONE_OVER_2PI 1.0/(2*PI)

#define LIGHT_INTENSITY 80.0
#define PROBABILITY_OF_LIGHT_SAMPLE 0.6
#define SKY_COLOR vec3(0.89, 0.96, 1.00)

uint hash3(uint x, uint y, uint z);
float random3(vec3 f);
float random2(vec2 f);
vec4 randomHemispherePoint(vec3 n, vec2 rand);
float hemisphereProbability(vec3 n, vec3 v);
vec4 randomDiskPoint(vec3 n, float d, float r, vec2 rand);
float diskProbability(vec3 n, float d, float r, vec3 v);
vec4 randomCosineWeightedHemispherePoint(vec3 n, vec2 rand);
float randomCosineWeightedHemispherePDF(vec3 n, vec3 v);
vec4 randomPhongWeightedHemispherePoint(vec3 r, float a, vec2 rand);
float randomPhongWeightedHemispherePDF(vec3 r, float a, vec3 v);
vec4 randomRectangleAreaDirection(vec3 p, vec3 c, vec3 x, vec3 y, vec2 rand);
float randomRectangleAreaDirectionPDF(vec3 p, vec3 x, vec3 x, vec3 y, vec3 v);
bool inrect(vec3 p, vec3 c, vec3 x, vec3 y);

layout(std430, binding = 0) readonly buffer SobolBuffer {
    uint8_t[] sobols;
};

layout(std430, binding = 1) readonly buffer ScrambleBuffer {
    uint8_t[] scrambles;
};

layout(std430, binding = 2) readonly buffer RankingBuffer {
    uint8_t[] rankings;
};

const box boxes[] = {
    {vec3(-5.0,-1.0,-5.0), vec3(5.0,0.0,5.0), vec3(0.5, 0.45, 0.33)}, // floor
    {vec3(-5.1, 0.0, -5.0), vec3(-5.0, 5.0, 5.0), vec3(0.4, 0.4, 0.4)}, // left wall
    {vec3(5.0, 0.0, -5.0), vec3(5.1, 5.0, 5.0), vec3(0.4, 0.4, 0.4)}, // right wall
    {vec3(-5.0, 0.0, -5.1), vec3(5.0, 5.0, -5.0), vec3(0.43, 0.52, 0.27)}, // back wall
    {vec3(-5.0, 0.0, 5.0), vec3(5.0, 5.0, 5.1), vec3(0.5, 0.2, 0.09)}, //front wall
    {vec3(-1.0, 1.0, -1.0), vec3(1.0, 1.1, 1.0), vec3(0.3, 0.23, 0.15)}, // table top
    {vec3(-1.0, 0.0, -1.0), vec3(-0.8, 1.0, -0.8), vec3(0.4, 0.3, 0.15)}, // table foot
    {vec3(-1.0, 0.0, 0.8), vec3(-0.8, 1.0, 1.0), vec3(0.4, 0.3, 0.15)}, // table foot
    {vec3(0.8, 0.0, -1.0), vec3(1.0, 1.0, -0.8), vec3(0.4, 0.3, 0.15)}, // table foot
    {vec3(0.8, 0.0, 0.8), vec3(1.0, 1.0, 1.0), vec3(0.4, 0.3, 0.15)}, // table foot
    {vec3(3.0, 0.0, -4.9), vec3(3.3, 2.0, -4.6), vec3(1.0, 0.0, 0.0)}, // a pillar
};

const rectangle rectangles[NUM_RECTANGLES] = {
    {vec3(-3.0, 0.3, 3.0), vec3(6.0, 0.0, 0.0), vec3(0.0, -0.2, 0.05)}
};

const sphere spheres[NUM_SPHERES] = {
    {vec3(0.0, 3.0, 0.0), 0.1}
};

struct hitinfo {
    vec3 normal;
    float near;
    int bi;
    bool isRectangle;
};

vec2 intersectBox(vec3 origin, vec3 dir, const box b, out vec3 normal)
{
    vec3 tMin = (b.min - origin) / dir;
    vec3 tMax = (b.max - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    normal = vec3(equal(t1, vec3(tNear))) * sign(-dir);
    return vec2(tNear, tFar);
}

vec2 intersectSphere(vec3 origin, vec3 dir, const sphere s)
{
    vec3 L = s.c - origin;
    float tca = dot(L, dir);
    float d2 = dot(L, L) - tca * tca;
    if(d2 > s.r * s.r)
        return vec2(-1.0);
    float thc = sqrt(s.r * s.r - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;
    if(t0 < t1 && t1 >= 0.0)
        return vec2(t0, t1);
    return vec2(-1.0);
}

bool intersectRectangle(vec3 origin, vec3 dir, rectangle r, out hitinfo hinfo)
{
    vec3 n = cross(r.x, r.y);
    float den = dot(n, dir), t = dot(r.c - origin, n) / den;
    hinfo.near = t;
    return den < 0.0 && t > 0.0 && inrect(origin + t * dir, r.c, r.x, r.y);
}

ivec2 px;

bool intersectObjects(vec3 origin, vec3 dir, out hitinfo info) {
    float smallest = LARGE_FLOAT;
    bool found = false;
    vec3 normal;
    for(int i = 0; i < NUM_BOXES; i++) {
        vec2 lambda = intersectBox(origin, dir, boxes[i], normal);
        if(lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) {
            info.near = lambda.x;
            info.bi = i;
            info.isRectangle = false;
            info.normal = normal;
            smallest = lambda.x;
            found = true;
        }
    }

    for(int i=0; i < NUM_RECTANGLES; i++) {
        hitinfo hinfo;
        if(intersectRectangle(origin, dir, rectangles[i], hinfo) && hinfo.near < smallest)
        {
            info.near = hinfo.near;
            info.bi = i;
            info.isRectangle= true;
            smallest = hinfo.near;
            found = true;
        }
    }
    return found;
}

vec3 normalForSphere(vec3 hit, const sphere s)
{
    return normalize(hit - s.c);
}

vec3 normalForRectangle(vec3 hit, const rectangle r)
{
    return cross(r.x, r.y);
}

float sampleBlueNoise(uint sampleDimension)
{
    uint xoff = hash3(px.y >> 7u, px.x >> 7u, frameIndex >> 8u);
    uint yoff = hash3(px.x >> 7u, px.y >> 7u, frameIndex >> 8u);
    uvec2 pxo = (px + ivec2(xoff, yoff)) & 127u;
    uint sampleIndex = frameIndex & 255u;
    sampleDimension = sampleDimension & 255u;
    uint pxv = (pxo.x + (pxo.y << 7u)) << 3u;
    uint rankedSampleIndex = sampleIndex ^ rankings[sampleDimension + pxv];
    uint value = sobols[sampleDimension + (rankedSampleIndex << 8u)];
    value ^= scrambles[(sampleDimension & 7u) + pxv];
    return (0.5 + float(value)) / 256.0;
}

vec3 randBlueNoise(int s)
{
    /*
    vec2 o = vec2(
        random2(vec2(s, time)),
        random2(vec2(s, time * 1.3)));
    vec3 bn = texture(blueNoiseTex, o + vec2(px) / textureSize(blueNoiseTex, 0)).rgb;
    return fract(bn + time);
    */
    return vec3(
        sampleBlueNoise(3*s),
        sampleBlueNoise(3*s + 1),
        sampleBlueNoise(3*s + 2)
    );
}

vec3 randWhiteNoise(int s)
{
    return vec3(
        random3(vec3(px + ivec2(s), time)),
        random3(vec3(px + ivec2(s), time * 1.1)),
        random3(vec3(px + ivec2(s), time * 0.3))
    );
}

vec3 randvec3(int s)
{
    return useBlueNoise ? randBlueNoise(s) : randWhiteNoise(s);
}

vec3 brdfSpecular(vec3 i, vec3 o, vec3 n)
{
    float a = phongExponent;
    vec3 r = reflect(-i, n);
    return vec3(pow(max(0.0, dot(r, o)), a) * (a + 2.0) * ONE_OVER_2PI);
}

vec3 brdfDiffuse(vec3 albedo, vec3 i, vec3 o, vec3 n)
{
    return albedo * ONE_OVER_PI;
}

vec3 brdf(vec3 albedo, vec3 i, vec3 o, vec3 n)
{
    return  brdfSpecular(i, o , n) * specularFactor
            +
            brdfDiffuse(albedo, i, o, n) * (1.0 - specularFactor);

}

void write(vec3 p, vec3 n)
{
    imageStore(normbuffer, px, vec4(n, 0.0));
    imageStore(posbuffer, px, vec4(p, 1.0));
}

vec3 trace(vec3 origin, vec3 dir) {

  vec3 att = vec3(1.0);

  for (int bounce = 0; bounce < BOUNCES; bounce++)
  {

    hitinfo hinfo;

    if (!intersectObjects(origin, dir, hinfo)) {
        if(bounce == 0)
        {
            write(vec3(0.0), vec3(0.0));
        }
        return vec3(0.0);
    }

    vec3 point = origin + hinfo.near * dir;
    vec3 normal, albedo;

    if (hinfo.isRectangle)
    {
        const rectangle r = rectangles[hinfo.bi];
        if(bounce == 0)
        {
            write(point, normalForRectangle(point, r));
        }
        return att * LIGHT_INTENSITY;
    } else
    {
        const box b = boxes[hinfo.bi];
        normal = hinfo.normal;
        albedo = b.col;
        if(bounce == 0)
        {
            write(point, normal);
        }
    }

    origin = point + normal * EPSILON;
    rectangle li = rectangles[0];
    vec4 s;
    vec3 rand = randvec3(bounce);

    if(multipleImportanceSampled)
    {
        float wl = PROBABILITY_OF_LIGHT_SAMPLE;
        vec3 ps = vec3(wl, specularFactor, 1.0 - specularFactor);
        vec3 np = ps / (ps.x + ps.y + ps.z);
        vec2 cdf = vec2(np.x, np.x + np.y);
        vec3 p, c;
        vec3 r = reflect(dir, normal);

        if(rand.z  < cdf.x)
        {
            s = randomRectangleAreaDirection(origin, li.c, li.x, li.y, rand.xy);
            p = vec3(s.w,
                randomCosineWeightedHemispherePDF(normal, s.xyz),
                randomPhongWeightedHemispherePDF(r, phongExponent, s.xyz));
            c = np.xzy;
        } else if (rand.z < cdf.y)
        {
            s = randomPhongWeightedHemispherePoint(r, phongExponent, rand.xy);
            p = vec3(s.w,
                randomCosineWeightedHemispherePDF(normal, s.xyz),
                randomRectangleAreaDirectionPDF(origin, li.c, li.x, li.y, s.xyz));
            c = np.yzx;
        } else
        {
            s = randomCosineWeightedHemispherePoint(normal, rand.xy);
            p = vec3(s.w,
                randomPhongWeightedHemispherePDF(r, phongExponent, s.xyz),
                randomRectangleAreaDirectionPDF(origin, li.c, li.x, li.y, s.xyz));
            c = np.zyx;
        }
        s.w = dot(p, c);
    }
    else
    {
        s = randomHemispherePoint(normal, rand.xy);
    }
    att *= brdf(albedo, s.xyz, -dir, normal);
    dir = s.xyz;
    att *= max(0.0, dot(dir, normal));

    if(s.w > 0.0)
        att /= s.w;
  }
  return vec3(0.0);
}

layout (local_size_x = 16, local_size_y = 16) in;

void main(void) {
    px = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(framebuffer);
    if (any(greaterThanEqual(px, size)))
        return;

    vec2 pos = (vec2(px) + 0.5) / vec2(size);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);

    vec3 color = trace(eye, normalize(dir));
    vec3 oldColor = vec3(0.0);
    if(blendFactor > 0.0)
        oldColor = imageLoad(framebuffer, px).rgb;
    vec3 finalColor = mix(color, oldColor, blendFactor);
    imageStore(framebuffer, px, vec4(finalColor, 1.0));
}