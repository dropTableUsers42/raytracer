#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

uniform float time;

uniform float blendFactor;

uniform bool multipleImportanceSampled;

uniform bool importanceSampled;

uniform float phongExponent;

uniform float specularFactor;

struct box {
    vec3 min;
    vec3 max;
    vec3 col;
};

struct sphere {
    vec3 c;
    float r;
};

#define NUM_BOXES 11
#define NUM_SPHERES 1
#define LARGE_FLOAT 1E+10
#define EPSILON 0.0001

#define PI radians(180.0)
#define ONE_OVER_PI 1.0/PI
#define ONE_OVER_2PI 1.0/(2*PI)

#define LIGHT_INTENSITY 2000.0
#define PROBABILITY_OF_LIGHT_SAMPLE 0.7
#define SKY_COLOR vec3(0.89, 0.96, 1.00)

float random(vec3 f);
vec4 randomHemispherePoint(vec3 n, vec2 rand);
float hemisphereProbability(vec3 n, vec3 v);
vec4 randomDiskPoint(vec3 n, float d, float r, vec2 rand);
float diskProbability(vec3 n, float d, float r, vec3 v);
vec4 randomCosineWeightedHemispherePoint(vec3 n, vec2 rand);
vec4 randomPhongWeightedHemispherePoint(vec3 r, float a, vec2 rand);

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

const sphere spheres[NUM_SPHERES] = {
    {vec3(0.0, 3.0, 0.0), 0.1}
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

struct hitinfo {
    vec3 normal;
    float near;
    int bi;
    bool isSphere;
};

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
            info.isSphere = false;
            info.normal = normal;
            smallest = lambda.x;
            found = true;
        }
    }

    for(int i=0; i < NUM_SPHERES; i++) {
        vec2 lambda = intersectSphere(origin, dir, spheres[i]);
        if(lambda.y >= 0.0 && lambda.x < lambda.y && lambda.x < smallest)
        {
            info.near = lambda.x;
            info.bi = i;
            info.isSphere = true;
            smallest = lambda.x;
            found = true;
        }
    }
    return found;
}

vec3 normalForSphere(vec3 hit, const sphere s)
{
    return normalize(hit - s.c);
}

vec3 randvec3(int s)
{
    return vec3(
        random(vec3(px + ivec2(s), time)),
        random(vec3(px + ivec2(s), time + 1.1)),
        random(vec3(px + ivec2(s), time + 0.3))
    );
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

vec3 trace(vec3 origin, vec3 dir) {
  /*
   * Since we are tracing a light ray "back" from the eye/camera into
   * the scene, everytime we hit a box surface, we need to remember the
   * attenuation the light would take because of the albedo (fraction of
   * light not absorbed) of the surface it reflected off of. At the
   * beginning, this value will be 1.0 since when we start, the ray has
   * not yet touched any surface.
   */
  vec3 att = vec3(1.0);
  /*
   * We are following a ray a fixed number of bounces through the scene.
   * Since every time the ray intersects a box, the steps that need to
   * be done at every such intersection are the same, we use a simple
   * for loop. The first iteration where `bounce == 0` will process our
   * primary ray starting directly from the eye/camera through the
   * framebuffer pixel into the scene. Let's go...
   */
  for (int bounce = 0; bounce < 3; bounce++) {
    /*
     * The ray now travels through the scene of boxes and eventually
     * either hits a box or escapes through the open ceiling. So, test
     * which case it is going to be.
     */
    hitinfo hinfo;
    if (!intersectObjects(origin, dir, hinfo)) {
      /*
       * The ray did not hit any box in the scene, so it escaped through
       * the open ceiling to the outside world, which is assumed to
       * consist of nothing but light coming in from everywhere. Return
       * the attenuated sky color.
       */
        return vec3(0.0);
    }
    /*
     * When we are here, the ray we are currently processing hit a box.
     * Obtain the box from the index in the hitinfo.
     */
    /*
     * Next, we need the actual point of intersection. So evaluate the
     * ray equation `point = origin + t * dir` with 't' being the value
     * returned by intersectBoxes() in the hitinfo.
     */
    vec3 point = origin + hinfo.near * dir;
    vec3 normal, albedo;
    if (hinfo.isSphere)
    {
        const sphere s = spheres[hinfo.bi];
        return att * LIGHT_INTENSITY * dot(normalForSphere(point, s), -dir);
    } else
    {
        const box b = boxes[hinfo.bi];
        normal = hinfo.normal;
        albedo = b.col;
    }
    
    origin = point + normal * EPSILON;
    /*
     * We have the new origin of the ray and now we just need its new
     * direction. Since we have the normal of the hemisphere within
     * which we want to generate the new direction, we can call a
     * function which generates such vector based on the normal and some
     * pseudo-random numbers we need to generate as well.
     */
    vec4 s;
    vec3 rand = randvec3(bounce);

    if(multipleImportanceSampled)
    {
        sphere li = spheres[0];
        vec3 d = li.c - origin, n = normalize(d);

        if(rand.z  < PROBABILITY_OF_LIGHT_SAMPLE)
        {
            s = randomDiskPoint(n, length(d), li.r, rand.xy);
            float p = hemisphereProbability(normal, s.xyz);
            s.w = (s.w + p) * PROBABILITY_OF_LIGHT_SAMPLE;
        } else
        {
            s = randomHemispherePoint(normal, rand.xy);
            float p = diskProbability(n, length(d), li.r, s.xyz);
            s.w = (s.w + p) * (1.0 - PROBABILITY_OF_LIGHT_SAMPLE);
        }
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
  /*
   * When followed a ray for the maximum number of bounces and that ray
   * still did not escape through the ceiling into the light, that ray
   * does therefore not transport any light back to the eye/camera and
   * its contribution to this Monte Carlo iteration will be 0.
   */
  return vec3(0.0);
}

layout (local_size_x = 16, local_size_y = 16) in;

void main(void) {
    px = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(framebuffer);
    if (any(greaterThanEqual(px, size)))
        return;

    vec2 pos = (vec2(px) + vec2(0.5)) / vec2(size);
    vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x);

    vec3 color = trace(eye, normalize(dir));
    vec3 oldColor = vec3(0.0);
    if(blendFactor > 0.0)
        oldColor = imageLoad(framebuffer, px).rgb;
    vec3 finalColor = mix(color, oldColor, blendFactor);
    imageStore(framebuffer, px, vec4(finalColor, 1.0));
}