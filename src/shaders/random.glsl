#version 430 core

#define PI radians(180.0)
#define ONE_OVER_PI 1.0/PI
#define ONE_OVER_2PI 1.0/(2 * PI)

#define spatialrand vec2

#define MAX_LIGHTS 1000

uniform int numEmitters;

struct triangle {
  vec3 v0, v1, v2;
  vec3 n0, n1, n2;
  int mtlIndex;
  vec2 uv0, uv1, uv2;
  int objId;
};
layout(std430, binding = 6) readonly buffer Emitters {
  triangle[] emitters;
};


struct mtl {
  vec3 Kd;
  float Ns;
  vec3 Ke;
  bool emitter;
};
layout(std430, binding = 5) readonly buffer Mtls {
  mtl[] mtls;
};

vec3 ortho(vec3 v)
{
    return normalize(abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0) : vec3(0.0, -v.z, v.y));
}

uint hash2(uint x, uint y)
{
    x += x >> 11;
    x ^= x << 7;
    x += y;
    x ^= x << 6;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

uint hash3(uint x, uint y, uint z)
{
    x += x >> 11;
    x ^= x << 7;
    x += y;
    x ^= x << 3;
    x += z ^ (x >> 14);
    x ^= x << 6;
    x += x >> 15;
    x ^= x << 5;
    x += x >> 12;
    x ^= x << 9;
    return x;
}

float random3(vec3 f)
{
    uint mantissaMask = 0x007FFFFFu;
    uint one = 0x03F800000u;
    uvec3 u = floatBitsToUint(f);
    uint h = hash3(u.x, u.y, u.z);
    return uintBitsToFloat((h & mantissaMask) | one) - 1.0;
}

float random2(vec2 f)
{
    uint mantissaMask = 0x007FFFFFu;
    uint one = 0x3F800000u;
    uvec2 u = floatBitsToUint(f);
    uint h = hash2(u.x, u.y);
    return uintBitsToFloat((h & mantissaMask) | one) - 1.0;
}

vec3 around(vec3 v, vec3 z)
{
    vec3 t = ortho(z), b = cross(z, t);
    return t * v.x + b * v.y + z * v.z;
}

vec3 isotropic(float rp, float c)
{
    float p = 2 * PI * rp, s = sqrt(1.0 - c*c);
    return vec3(cos(p) * s, sin(p) * s, c);
}

vec4 randomHemispherePoint(vec3 n, spatialrand rand)
{
    return vec4(around(isotropic(rand.x, rand.y), n), ONE_OVER_2PI);
}

float hemisphereProbability(vec3 n, vec3 v)
{
    return step(0.0, dot(v, n)) * ONE_OVER_2PI;
}

vec4 randomDiskPoint(vec3 n, float d, float r, spatialrand rand)
{
    float D = r/d, c = inversesqrt(1.0 + D * D), pr = ONE_OVER_2PI / (1.0 - c);
    return vec4(around(isotropic(rand.x, 1.0 - rand.y * (1.0 - c)), n), pr);
}

float diskProbability(vec3 n, float d, float r, vec3 v)
{
    float D = r/d, c = inversesqrt(1.0 + D * D);
    return step(c, dot(n, v)) * ONE_OVER_2PI / (1.0 - c);
}

vec4 randomCosineWeightedHemispherePoint(vec3 n, spatialrand rand)
{
    float c = sqrt(rand.y);
    return vec4(around(isotropic(rand.x, c), n), c * ONE_OVER_PI);
}

float randomCosineWeightedHemispherePDF(vec3 n, vec3 v)
{
    return max(0.0, dot(n, v)) * ONE_OVER_PI;
}

vec4 randomPhongWeightedHemispherePoint(vec3 r, float a, spatialrand rand)
{
    float ai = 1.0 / (a + 1.0), pr = (a + 1.0) * pow(rand.y, a * ai) * ONE_OVER_2PI;
    return vec4(around(isotropic(rand.x, pow(rand.y, ai)), r), pr);
}

float randomPhongWeightedHemispherePDF(vec3 r, float a, vec3 v)
{
    if( dot(r, v) < 0.0)
        return 0.0;
    return (a + 1.0) * pow(max(0.0, dot(r, v)), a) * ONE_OVER_2PI;
}

vec4 randomRectangleAreaDirection(vec3 p, vec3 c, vec3 x, vec3 y, spatialrand rand)
{
    vec3 s = c + rand.x * x + rand.y * y, sp = p -s;
    float len2 = dot(sp, sp);
    vec3 d = sp * inversesqrt(len2);
    return vec4(-d, len2/max(0.0, dot(d, cross(x,y))));
}

bool inrect(vec3 p, vec3 c, vec3 x, vec3 y)
{
    vec2 xyp = vec2(dot(p-c, x), dot(p-c, y));
    return all(greaterThanEqual(xyp, vec2(0.0)))
        && all(lessThan(xyp, vec2(dot(x,x), dot(y,y))));
}

float randomRectangleAreaDirectionPDF(vec3 p, vec3 c, vec3 x, vec3 y, vec3 v)
{
    vec3 n = cross(x, y);
    float den = dot(n, v), t = dot(c - p, n) / den;
    vec3 s = p + t * v, sp = p- s;
    float len2 = dot(sp, sp);
    vec3 d = sp * inversesqrt(len2);
    return float(den < 0.0 && t > 0.0 && inrect(s, c, x, y)) * len2 / dot(d,n);
}

vec4 randomTriangleAreaDirection(vec3 p, triangle t, spatialrand rand)
{
    float one_minus_u = sqrt(rand.x);
    vec2 uv = vec2(1-one_minus_u, rand.y * one_minus_u);
    vec3 s = (1 - uv.x - uv.y) * t.v0 + uv.x * t.v1 + uv.y * t.v2;
    vec3 sp = p - s;
    float d2 = dot(sp, sp);
    vec3 d = sp * inversesqrt(d2);
    vec3 e1 = t.v1 - t.v0;
    vec3 e2 = t.v2 - t.v0;
    return vec4(-d, d2/max(0.0, abs(dot(d, 0.5 * cross(e1, e2)))));
}

bool inTri(vec3 p, triangle t)
{
    vec3 e1 = t.v1 - t.v0;
    vec3 e2 = t.v2 - t.v0;
    vec3 n = cross(e1, e2);

    vec3 c;

    vec3 edge0 = e1;
    vec3 vp0 = p - t.v0;
    c = cross(edge0, vp0);
    if(dot(n, c) < 0)
        return false;

    vec3 edge1 = e2 - e1;
    vec3 vp1 = p - t.v1;
    c = cross(edge1, vp1);
    if(dot(n, c) < 0)
        return false;

    vec3 edge2 = -e2;
    vec3 vp2 = p - t.v2;
    c = cross(edge2, vp2);
    if(dot(n, c) < 0)
        return false;
    return true;

}

float randomTriangleAreaDirectionPDF(vec3 p, triangle tri, vec3 v)
{
    vec3 n = cross(tri.v1 - tri.v0, tri.v2 - tri.v0);
    float den = dot(n, v), t = dot(tri.v0 - p, n) / den;
    vec3 s = p + t * v, sp = p- s;
    float len2 = dot(sp, sp);
    vec3 d = sp * inversesqrt(len2);
    return float(den < 0.0 && t > 0.0 && inTri(s, tri)) * len2 / abs(dot(d,n));
}

int randomEmitterSample(vec3 origin, float rand, out float pdf)
{
    /*float totalWeight = 0;
    float weights_[4];
    for(int i = 0; i < numEmitters; i++)
    {
        float weight = 0.0;

        vec3 d = emitters[i].v0 - origin;
        vec3 e1 = emitters[i].v1 - emitters[i].v0;
        vec3 e2 = emitters[i].v2 - emitters[i].v0;
        vec3 n = cross(e1, e2);
        if(dot(d, n) < 0)
        {
            weight = length(n) * length(mtls[emitters[i].mtlIndex].Ke) / dot(d, d);
        }
        weights_[i] = weight;
        totalWeight += weight;
    }
    float curWeight = 0;
    int i;
    for(i = 0; i < numEmitters; i++)
    {
        curWeight += weights_[i] / totalWeight;
        if(curWeight > rand)
            break;
    }*/
    //pdf = weights_[i]/totalWeight;
    pdf = 1.0/numEmitters;
    return min(int(rand * numEmitters), numEmitters - 1);
}