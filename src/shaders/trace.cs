#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;

uniform vec3 eye;
uniform vec3 ray00;
uniform vec3 ray10;
uniform vec3 ray01;
uniform vec3 ray11;

uniform float time;

uniform float blendFactor;

uniform bool importanceSampled;

uniform float phongExponent;

uniform float specularFactor;

struct box {
    vec3 min;
    vec3 max;
    vec3 col;
};

#define NUM_BOXES 11
#define LARGE_FLOAT 1E+10
#define EPSILON 0.0001

#define PI radians(180.0)
#define ONE_OVER_PI 1.0/PI
#define ONE_OVER_2PI 1.0/(2*PI)

#define LIGHT_INTENSITY 4.0
#define SKY_COLOR vec3(0.89, 0.96, 1.00)

float random(vec3 f);
vec4 randomHemispherePoint(vec3 n, vec2 rand);
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

vec2 intersectBox(vec3 origin, vec3 dir, const box b, out vec3 normal) {
    vec3 tMin = (b.min - origin) / dir;
    vec3 tMax = (b.max - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    normal = vec3(equal(t1, vec3(tNear))) * sign(-dir);
    return vec2(tNear, tFar);
}

struct hitinfo {
    vec3 normal;
    float near;
    int bi;
};

ivec2 px;

bool intersectBoxes(vec3 origin, vec3 dir, out hitinfo info) {
    float smallest = LARGE_FLOAT;
    bool found = false;
    vec3 normal;
    for(int i = 0; i < NUM_BOXES; i++) {
        vec2 lambda = intersectBox(origin, dir, boxes[i], normal);
        if(lambda.x > 0.0 && lambda.x < lambda.y && lambda.x < smallest) {
            info.near = lambda.x;
            info.bi = i;
            info.normal = normal;
            smallest = lambda.x;
            found = true;
        }
    }
    return found;
}

vec3 randvec3(int s)
{
    return vec3(
        random(vec3(px + ivec2(s), time)),
        random(vec3(px + ivec2(s), time + 1.1)),
        random(vec3(px + ivec2(s), time + 0.3))
    );
}

vec3 brdfSpecular(box b, vec3 i, vec3 o, vec3 n)
{
    float a = phongExponent;
    vec3 r = reflect(-i, n);
    return vec3(pow(max(0.0, dot(r, o)), a) * (a + 2.0) * ONE_OVER_2PI);
}

vec3 brdfDiffuse(box b, vec3 i, vec3 o, vec3 n)
{
    return b.col * ONE_OVER_PI;
}

vec3 brdf(box b, vec3 i, vec3 o, vec3 n)
{
    return  brdfSpecular(b, i, o , n) * specularFactor
            +
            brdfDiffuse(b, i, o, n) * (1.0 - specularFactor);

}

/*vec4 trace(vec3 origin, vec3 dir) {
    hitinfo i;
    if (!intersectBoxes(origin, dir, i))
        return vec4(SKY_COLOR, 1.0);
    box b = boxes[i.bi];
    return vec4(b.col, 1.0);
}*/

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
    if (!intersectBoxes(origin, dir, hinfo)) {
      /*
       * The ray did not hit any box in the scene, so it escaped through
       * the open ceiling to the outside world, which is assumed to
       * consist of nothing but light coming in from everywhere. Return
       * the attenuated sky color.
       */
        float d = dir.y;
        if(bounce == 0)
            return SKY_COLOR * att;
        return LIGHT_INTENSITY * SKY_COLOR * att * d;
    }
    /*
     * When we are here, the ray we are currently processing hit a box.
     * Obtain the box from the index in the hitinfo.
     */
    box b = boxes[hinfo.bi];
    /*
     * Next, we need the actual point of intersection. So evaluate the
     * ray equation `point = origin + t * dir` with 't' being the value
     * returned by intersectBoxes() in the hitinfo.
     */
    vec3 point = origin + hinfo.near * dir;
    /*
     * Now, we want to be able to generate a new ray which starts from
     * the point of intersection and travels away from the surface into
     * a random direction. For that, we first need the surface's normal
     * at the point of intersection.
     */
    vec3 normal = hinfo.normal;
    /*
     * Next, we reset the ray's origin to the point of intersection
     * offset by a small epsilon along the surface normal to avoid
     * intersecting that box again because of precision issues with
     * floating-point arithmetic. Notice that we just overwrite the
     * input parameter's value since we do not need the original value
     * anymore, which for the first bounce was our eye/camera location.
     */
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

    if(importanceSampled)
    {
        if (rand.z < specularFactor)
        {
            vec3 r = reflect(dir, normal);
            s = randomPhongWeightedHemispherePoint(r, phongExponent, rand.xy);
            att *= brdfSpecular(b, s.xyz, -dir, normal);
        } else
        {
            s = randomCosineWeightedHemispherePoint(normal, rand.xy);
            att *= brdfDiffuse(b, s.xyz, -dir, normal);
        }
    }
    else
    {
        s = randomHemispherePoint(normal, rand.xy);
        att *= brdf(b, s.xyz, -dir, normal);
    }
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