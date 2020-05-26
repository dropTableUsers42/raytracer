/*
 * Copyright LWJGL. All rights reserved.
 * License terms: https://www.lwjgl.org/license
 */
#version 430 core

/**
 * The image we write to.
 */
layout(location =0, binding = 0, rgba32f) uniform image2D framebuffer;
layout(location = 1, binding = 1, rgba8_snorm) uniform image2D normbuffer;
layout(location = 2, binding = 2, rgba16f) uniform image2D posbuffer;

/*
 * Camera frustum description known from previous tutorials.
 */
uniform vec3 col;
uniform vec3 eye, ray00, ray01, ray10, ray11;

uniform float time;

uniform float blendFactor;

uniform bool importanceSampled;

uniform int maxBounces;

ivec2 px;

#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define ONE_OVER_PI (1.0 / PI)
#define ONE_OVER_2PI (1.0 / TWO_PI)
#define LARGE_FLOAT 1E+10
#define EPSILON 0.0001
#define LIGHT_INTENSITY 2.0
#define SKY_COLOR vec3(0.89, 0.96, 1.00)
#define PROBABILITY_OF_LIGHT_SAMPLE 0.5

#define SPECULARITY 0.5

/**
 * This describes the structure of a single BVH node.
 * We will read this from the "Nodes" SSBO (see below).
 */
struct node {				//Base alignment 	Aligned Offset
  vec3 min;					//	16					0

  vec3 max;					//	16					16

  int hitNext;				//	4					28

  int missNext;				//	4					32

  int firstTri, numTris;	//	4					36
							//	4					40
};
layout(std430, binding = 3) readonly buffer Nodes {
  node[] nodes; 			//	16					48
};

/**
 * Describes a single triangle with position and normal.
 */
struct triangle {
  vec3 v0, v1, v2;  //16            0
                    //16            16
                    //16            32
  vec3 n0, n1, n2;  //16            48
                    //16            64
                    //16            80
  int mtlIndex;     //4             92
  vec2 uv0, uv1, uv2;
                    //8             96
                    //8             104
                    //8             112

};
layout(std430, binding = 4) readonly buffer Triangles {
  triangle[] triangles;//16         
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

struct rectangle {
    vec3 c;
    vec3 x;
    vec3 y;
};

#define MAX_POINTS 100

/**
 * Forward declaration of helper functions in qtablehelper.glsl
 */
uint nearest(vec2 uv, uint N = MAX_POINTS);
vec2 hammersley(uint i, uint N = MAX_POINTS);


/**
 * Forward-declaration of random point sampling functions in random.glsl
 */
float random3(vec3 f);
vec4 randomHemispherePoint(vec3 n, vec2 rand);
vec4 randomCosineWeightedHemispherePoint(vec3 n, vec2 rand);
float randomCosineWeightedHemispherePDF(vec3 n, vec3 v);
vec4 randomPhongWeightedHemispherePoint(vec3 r, float a, vec2 rand);
float randomPhongWeightedHemispherePDF(vec3 r, float a, vec3 v);
vec4 randomTriangleAreaDirection(vec3 p, triangle t, vec2 rand);
float randomTriangleAreaDirectionPDF(vec3 p, triangle tri, vec3 v);
int randomEmitterSample(vec3 origin, float rand, out float pdf);
vec4 randomRectangleAreaDirection(vec3 p, vec3 c, vec3 x, vec3 y, vec2 rand);
float randomRectangleAreaDirectionPDF(vec3 p, vec3 x, vec3 x, vec3 y, vec3 v);

/**
 * Forward-declaration of the same structure in geometry.glsl. 
 */
struct trianglehitinfo {
  float t, u, v; // <- u, v = barycentric coordinates
};
bool intersectTriangle(vec3 origin, vec3 dir, vec3 v0, vec3 v1, vec3 v2, out trianglehitinfo thinfo);
bool intersectBox(vec3 origin, vec3 invdir, vec3 boxMin, vec3 boxMax, out float t);

#define NO_INTERSECTION -1

/**
 * Intersect all triangles in the given BVH node 'n' and return the 
 * information about the closest triangle into 'thinfo'.
 *
 * @param origin the ray's origin
 * @param dir the ray's direction
 * @param n the node whose triangles to check
 * @param t the upper bound on the distances of any triangle
 * @param[out] thinfo will store the information of the closest triangle
 * @returns the index of the closest triangle into the 'Triangles' SSBO
 *          or NO_INTERSECTION if there was no intersection
 */

int intersectTriangles(vec3 origin, vec3 dir, const node n, float t, out trianglehitinfo thinfo) {
  int idx = NO_INTERSECTION;
  for (int i = n.firstTri; i < n.firstTri + n.numTris; i++) {
    const triangle tri = triangles[i];
    trianglehitinfo info;
    if (intersectTriangle(origin, dir, tri.v0, tri.v1, tri.v2, info) && info.t < t) {
      thinfo.u = info.u;
      thinfo.v = info.v;
      thinfo.t = info.t;
      t = info.t;
      idx = i;
    }
  }
  return idx;
}

/**
 * Compute the interpolated normal of the given triangle.
 *
 * @param thinfo the information returned by 'intersectTriangles()'
 * @param tridx the index of that triangle into the 'Triangles' SSBO
 * @returns the interpolated normal
 */
vec3 normalForTriangle(trianglehitinfo thinfo, int tridx) {
  /*
   * Use the barycentric coordinates in the thinfo to weight the
   * vertices normals accordingly.
   */
  const triangle tri = triangles[tridx];
  float u = thinfo.u, v = thinfo.v, w = 1.0 - u - v;
  return normalize(vec3(tri.n0 * w + tri.n1 * u + tri.n2 * v));
}

vec2 uvForTriangle(trianglehitinfo thinfo, int tridx)
{
  const triangle tri = triangles[tridx];
  float u = thinfo.u, v = thinfo.v, w = 1.0 - u - v;
  return vec2(tri.uv0 * w + tri.uv1 * u + tri.uv2 * v);
}

struct surfaceInteraction
{
  int tridx;
  vec3 color;
  vec3 normal;
  vec3 point;
  int matidx;
  vec2 uv;
};

#define NO_NODE -1
#define ERROR_COLOR vec3(1.0, 0.0, 1.0)
#define MAX_FOLLOWS 1000

void intersectGeometry(vec3 origin, vec3 dir, vec3 invdir, out surfaceInteraction SI)
{
  int nextIdx = 0; // <- start with the root node
  float nearestTri = 1.0/0.0; // <- +inf
  vec3 normal = vec3(0.0);
  vec3 color = vec3(0.0);
  vec2 uv = vec2(0.0);
  int iterations = 0;
  int rettridx = -1;
  vec3 point = vec3(0.0);
  while (nextIdx != NO_NODE) {
    iterations++;
    if (iterations > MAX_FOLLOWS)
    {
      SI.tridx = -1;
      SI.color = ERROR_COLOR;
      SI.normal = ERROR_COLOR;
      return;
    }

    const node next = nodes[nextIdx];

    float t;
    if (!intersectBox(origin, invdir, next.min, next.max, t))
    {
      nextIdx = next.missNext;
    } else
    {

      if (t > nearestTri)
      {

        nextIdx = next.missNext;
        continue;
      }
      if (next.numTris > 0)
      {

        trianglehitinfo thinfo;
        int tridx = intersectTriangles(origin, dir, next, nearestTri, thinfo);
        if (tridx != NO_INTERSECTION)
        {

          normal = normalForTriangle(thinfo, tridx);
          uv = uvForTriangle(thinfo, tridx);
          color = mtls[triangles[tridx].mtlIndex].Kd;
          nearestTri = thinfo.t;
          rettridx = tridx;
          point = origin + nearestTri * dir;
        }
      }
      nextIdx = next.hitNext;
    }
  }

  SI.tridx = rettridx;
  SI.matidx = triangles[SI.tridx].mtlIndex;
  SI.normal = normal;
  SI.color = color;
  SI.point = point;
  SI.uv = uv;
  return;
}

float rand_seeds[] = {0, 1.1, 0.3, 3.3, 0.6, 4.5, 2.4, 0.1, 1.7, 4.7, 1.9, 2.6, 4.8};
int seed = 0;

vec2 randvec2(int s) {
  seed = (seed + 2)%13;
  return vec2(
    random3(vec3(px + ivec2(s), time + rand_seeds[seed - 2])),
    random3(vec3(px + ivec2(s), time + rand_seeds[seed - 1])));
}

vec3 randvec3(int s) {
  seed = (seed + 3)%13;
  return vec3(
    random3(vec3(px + ivec2(s), time + rand_seeds[seed - 3])),
    random3(vec3(px + ivec2(s), time + rand_seeds[seed - 2])),
    random3(vec3(px + ivec2(s), time + rand_seeds[seed - 1])));
}

float randfloat(int s) {
  seed = (seed + 1)%13;
  return random3(vec3(px + ivec2(s), time + rand_seeds[seed - 1]));
}

vec3 brdfDiffuse(surfaceInteraction SI, vec3 i, vec3 o, vec3 n)
{
  triangle t = triangles[SI.tridx];
  vec3 rho = mtls[t.mtlIndex].Kd;
  /*uint index = nearest(SI.uv, 1000);
  vec2 hamm = hammersley(index, 1000);
  uint ind = index % 6;
  if(ind == 0)
    rho = vec3(0.0, 0.0, 0.0);
  if(ind == 1)
    rho = vec3(0.0, 1.0, 0.0);
  if(ind == 2)
    rho = vec3(0.0, 0.0, 1.0);
  if(ind == 3)
    rho = vec3(0.0, 1.0, 1.0);
  if(ind == 4)
    rho = vec3(1.0, 0.0, 1.0);
  if(ind == 5)
    rho = vec3(1.0, 1.0, 0.0);

  rho = vec3(hamm, 0.0);
  if(length(hamm - SI.uv) <= 0.001)
    rho = vec3(1.0, 0.0, 0.0);*/
  return rho * ONE_OVER_PI;
}

vec3 brdfSpecular(surfaceInteraction SI, vec3 i, vec3 o, vec3 n)
{
  triangle t = triangles[SI.tridx];
  float a = mtls[t.mtlIndex].Ns;
  vec3 r = reflect(-i, n);
  if(dot(r, o) < 0.0)
    return vec3(0.0);
  return vec3(pow(max(0.0, dot(r, o)), a) * (a + 2.0) * ONE_OVER_2PI);
}

vec3 brdf(surfaceInteraction SI, vec3 i, vec3 o, vec3 n)
{
  return brdfSpecular(SI, i, o, n) * SPECULARITY
          +
          brdfDiffuse(SI, i, o, n) * (1.0 - SPECULARITY);
}

void write(vec3 p, vec3 n)
{
    imageStore(normbuffer, px, vec4(n, 0.0));
    imageStore(posbuffer, px, vec4(p, 1.0));
}


vec3 traceOld(vec3 origin, vec3 dir, vec3 invdir) {
  
  vec3 att = vec3(1.0);

  for(int bounce = 0; bounce < 3;bounce++)
  {
    surfaceInteraction SI;
    intersectGeometry(origin, dir, invdir, SI);

    if(SI.tridx == -1)
    {
      if(bounce == 0)
      {
        write(vec3(0.0), vec3(0.0));
      }
      return SKY_COLOR * att;
    }

    triangle tri = triangles[SI.tridx];

    if(mtls[tri.mtlIndex].emitter)
    {
      return att * mtls[tri.mtlIndex].Ke * 30;
    }

    vec3 point = SI.point;
    
    vec3 normal = SI.normal;

    if(bounce == 0)
    {
      write(point, normal);
    }

    origin = point + normal * EPSILON;

    vec4 s;

    vec3 rand = randvec3(bounce);
    float rand_light = randfloat(bounce);
    float light_pdf;
    int lightIndex = randomEmitterSample(origin, rand_light, light_pdf);
    triangle t = emitters[lightIndex];

    rectangle li = rectangle(t.v1, t.v2 - t.v1, t.v0 - t.v1);

    vec3 brdf_att = vec3(1.0);

    if(importanceSampled)
    {
      float wl = PROBABILITY_OF_LIGHT_SAMPLE;
      vec3 ps = vec3(wl, SPECULARITY, 1.0 - SPECULARITY);
      vec3 np = ps / (ps.x + ps.y + ps.z);
      vec2 cdf = vec2(np.x, np.x + np.y);
      vec3 p,c;
      vec3 r = reflect(dir, normal);
      if(rand.z < cdf.x)
      {
        s = randomTriangleAreaDirection(origin, t, rand.xy);
        p = vec3(s.w,
                randomCosineWeightedHemispherePDF(normal, s.xyz),
                randomPhongWeightedHemispherePDF(r, mtls[tri.mtlIndex].Ns, s.xyz));
        c = np.xzy;
        brdf_att = brdf(SI, s.xyz, -dir, normal);
      } else if(rand.z < cdf.y)
      {
        s = randomPhongWeightedHemispherePoint(r, mtls[tri.mtlIndex].Ns, rand.xy);
        p = vec3(s.w,
                randomCosineWeightedHemispherePDF(normal, s.xyz),
                //randomTriangleAreaDirectionPDF(origin, t, s.xyz));
                randomTriangleAreaDirectionPDF(origin, t, s.xyz));
        c = np.yzx;
        brdf_att = brdf(SI, s.xyz, -dir, normal);
      } else
      {
        s = randomCosineWeightedHemispherePoint(normal, rand.xy);
        p = vec3(s.w,
                randomPhongWeightedHemispherePDF(r, mtls[tri.mtlIndex].Ns, s.xyz),
                //randomTriangleAreaDirectionPDF(origin, t, s.xyz));
                randomTriangleAreaDirectionPDF(origin, t, s.xyz));
        c = np.zyx;
        brdf_att = brdf(SI, s.xyz, -dir, normal);
      }
      s.w = dot(p, c);
    } else
    {
      if(rand.z < SPECULARITY)
      {
        vec3 r = reflect(dir, normal);
        s = randomPhongWeightedHemispherePoint(r, mtls[tri.mtlIndex].Ns, rand.xy);
        brdf_att = brdfSpecular(SI, s.xyz, -dir, normal);
      }
      else
      {
        s = randomCosineWeightedHemispherePoint(normal, rand.xy);
        brdf_att = brdfDiffuse(SI, s.xyz, -dir, normal);
      }
    }

    att *= brdf_att;

    dir = s.xyz;

    invdir = 1.0/dir;

    att *= max(0.0, dot(normal, dir));

    if(s.w > 0.0)
      att /= s.w;

    if(bounce > 3)
    {
      float rand_sample = randfloat(bounce);
      float q = max(0.05, 1 - att.g);
      if(rand_sample < q)
        break;
      att /= (1-q);
    }
  }
  return vec3(0.0);
}


bool visible(vec3 origin, vec3 wi, triangle t)
{
  surfaceInteraction SI;
  intersectGeometry(origin, wi, 1.0/wi, SI);

  triangle ti = triangles[SI.tridx];
  if(ti.v0 == t.v0 && ti.v1 == t.v1 && ti.v2 == t.v2 && ti.n0 == t.n0 && ti.n1 == t.n1 && ti.n2 == t.n2 && ti.mtlIndex == t.mtlIndex)
    return true;
  else
    return false;
}

float PowerHeuristic(int nf, float fpdf, int ng, float gpdf)
{
  float f = nf * fpdf, g = ng * gpdf;
  return (f * f)/(f * f + g* g);
}

vec3 EstimateDirect(vec3 origin, vec3 dir, surfaceInteraction SI, int bounces, triangle light)
{
    vec3 Ld = vec3(0.0);
    vec3 wi;
    float light_pdf = 0.0, brdf_pdf = 0.0;

    float spec_rand = randfloat(bounces);
    vec2 sample_rand = randvec2(bounces);
    //Sample Light MIS
    vec4 s = randomTriangleAreaDirection(origin, light, sample_rand.xy);
    wi = s.xyz;
    light_pdf = s.w;
    vec3 f;
    if(spec_rand < SPECULARITY)
    {
      vec3 r = reflect(dir, SI.normal);
      brdf_pdf = randomPhongWeightedHemispherePDF(r, mtls[SI.matidx].Ns, wi);
      f = brdfSpecular(SI, wi, -dir, SI.normal);

      f *= dot(normalize(SI.normal), normalize(wi));
    }
    else
    {
      brdf_pdf = randomCosineWeightedHemispherePDF(SI.normal, wi);
      f = brdfDiffuse(SI, wi, -dir, SI.normal) * dot(normalize(SI.normal), normalize(wi));
    }
    vec3 Li = mtls[light.mtlIndex].Ke * 30;
    triangle t = triangles[SI.tridx];
    if(visible(origin, wi, light) && dot(SI.normal, wi) >= 0)
    {
      Li *= 1.0;
    } else
    {
      Li *= 0.0;
    }
    float weight = PowerHeuristic(1, light_pdf, 1, brdf_pdf);
    Ld += f * Li * weight / light_pdf;

    sample_rand = randvec2(bounces);
    //Sample BRDF MIS
    if(spec_rand < SPECULARITY)
    {
      vec3 r = reflect(dir, SI.normal);
      s = randomPhongWeightedHemispherePoint(r, mtls[SI.matidx].Ns, sample_rand.xy);
      wi = s.xyz;
      brdf_pdf = s.w;
      f = brdfSpecular(SI, wi, -dir, SI.normal) * dot(normalize(SI.normal), normalize(wi));
    } else
    {
      s = randomCosineWeightedHemispherePoint(SI.normal, sample_rand.xy);
      wi = s.xyz;
      brdf_pdf = s.w;
      f = brdfDiffuse(SI, wi, -dir, SI.normal) * dot(normalize(SI.normal), normalize(wi));
    }
    light_pdf = randomTriangleAreaDirectionPDF(origin, light, wi);
    Li = mtls[light.mtlIndex].Ke * 30;
    if(visible(origin, wi, light))
    {
      Li *= 1.0;
    } else
    {
      Li *= 0.0;
    }
    weight = PowerHeuristic(1, brdf_pdf, 1, light_pdf);
    if(light_pdf != 0)
      Ld += f * Li * weight / brdf_pdf;
    return Ld;
}

vec3 UniformSampleOneLight(vec3 origin, vec3 dir, surfaceInteraction SI, int bounces)
{
    float light_prob;
    float light_rand = randfloat(bounces);
    int li = randomEmitterSample(origin, light_rand, light_prob);
    triangle light = emitters[li];

    return EstimateDirect(origin, dir, SI, bounces, light) / light_prob;
}

vec3 trace(vec3 origin, vec3 dir, vec3 invdir)
{
  vec3 L = vec3(0.0);
  vec3 beta = vec3(1.0);
  int bounces;
  for(bounces = 0;;++bounces)
  {
    surfaceInteraction SI;
    intersectGeometry(origin, dir, invdir, SI);
    bool foundIntersection = (SI.tridx != -1);

    if(bounces == 0)
    {
      if(foundIntersection)
      {
        L += beta * mtls[SI.matidx].Ke * 30;
        write(SI.point, SI.normal);
      }
      else
      {
        L += beta * SKY_COLOR;
        write(vec3(0.0), vec3(0.0));
      }
    }

    if(!foundIntersection || bounces >= maxBounces) break;

    origin = SI.point + SI.normal * EPSILON;

    vec3 Ld = UniformSampleOneLight(origin, dir, SI, bounces);
    L += beta * Ld;

    vec3 sample_rand = randvec3(bounces);
    //Sample BRDF for new ray
    vec4 s; vec3 wi; float pdf; vec3 f;
    if(sample_rand.z < SPECULARITY)
    {
      vec3 r = reflect(dir, SI.normal);
      s = randomPhongWeightedHemispherePoint(r, mtls[SI.matidx].Ns, sample_rand.xy);
      wi = s.xyz;
      pdf = s.w;
      f = brdfSpecular(SI, wi, -dir, SI.normal);
    } else
    {
      s = randomCosineWeightedHemispherePoint(SI.normal, sample_rand.xy);
      wi = s.xyz;
      pdf = s.w;
      f = brdfDiffuse(SI, wi, -dir, SI.normal);
    }
    beta *= f * dot(normalize(SI.normal), normalize(wi)) / pdf;

    dir = wi;
    invdir = 1.0 / wi;

    float rr_rand = randfloat(bounces);
    float rrbetamax = max(beta.r, max(beta.g, beta.b));
    if(bounces > 3)
    {
      float q = max(float(0.05), 1- rrbetamax);
      if(rr_rand < q) break;
      beta /= (1.0 - q);
    }
  }
  return L;
}

layout (local_size_x = 16, local_size_y = 8) in;

/**
 * Entry point of this GLSL compute shader.
 */
void main(void) {
  ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
  px = pix;
  ivec2 size = imageSize(framebuffer);
  if (any(greaterThanEqual(pix, size))) {
    return;
  }
  vec2 p = (vec2(pix) + vec2(0.5)) / vec2(size);
  vec3 dir = normalize(mix(mix(ray00, ray01, p.y), mix(ray10, ray11, p.y), p.x));
  vec3 color = trace(eye, dir, 1.0 / dir);
  vec3 oldColor = vec3(0.0);
  if(blendFactor > 0.0)
    oldColor = imageLoad(framebuffer, px).rgb;
  vec3 finalColor = mix(color, oldColor, blendFactor);

  /*vec3 rho;
  uint index = nearest(p, 500);
  vec2 hamm = hammersley(index, 500);
  uint ind = index % 6;
  if(ind == 0)
    rho = vec3(0.0, 0.0, 0.0);
  if(ind == 1)
    rho = vec3(0.0, 1.0, 0.0);
  if(ind == 2)
    rho = vec3(0.0, 0.0, 1.0);
  if(ind == 3)
    rho = vec3(0.0, 1.0, 1.0);
  if(ind == 4)
    rho = vec3(1.0, 0.0, 1.0);
  if(ind == 5)
    rho = vec3(1.0, 1.0, 0.0);

  if(length(hamm - p) <= 0.007)
  {
    rho = vec3(1.0, 0.0, 0.0);
  }*/

  imageStore(framebuffer, pix, vec4(finalColor, 1.0));
}