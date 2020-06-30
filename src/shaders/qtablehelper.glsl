#version 430 core

#define MAX_POINTS 100

#define MAX_LENGTH 10000

#define spatialrand vec2

#define PI radians(180.0)

uniform int numPoints;

uniform int maxPhi;
uniform int maxTheta;

#define ALPHA 0.5

layout(std430, binding = 7) buffer Normals {
	vec3[] normals_per_voronoi;
};

layout(std430, binding = 8) buffer Positions {
	vec3[] voronoi_centres;
};

layout(std430, binding = 9) coherent buffer QTable {
	float[] qt;
};

layout(std430, binding = 10) coherent buffer QVisits {
	int[] visits;
};

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
  int objId;

};
layout(std430, binding = 4) readonly buffer Triangles {
  triangle[] triangles;//16         
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

/**
 * Struct to store surface interaction info
 */
struct surfaceInteraction
{
  int tridx;
  vec3 color;
  vec3 normal;
  vec3 point;
  int matidx;
  vec2 uv;
};

/**
 * Fast arccos calculation
 * @param x
 * @return acos(x)
 */
float fastacos(float x) {
  float negate = float(x < 0);
  x = abs(x);
  float ret = -0.0187293;
  ret = ret * x;
  ret = ret + 0.0742610;
  ret = ret * x;
  ret = ret - 0.2121144;
  ret = ret * x;
  ret = ret + 1.5707288;
  ret = ret * sqrt(1.0-x);
  ret = ret - 2 * negate * ret;
  return negate * 3.14159265358979 + ret;
}

/**
 * Computes the radical inverse
 * @param bits
 * @return radical inverse of bits
 */
float radicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

/**
 * Computes the ith hammersley point
 * @param i
 * @return hammersley(i)
 */
vec2 hammersley(uint i)
{
	return vec2(float(i)/float(numPoints), radicalInverse(i));
}

/**
 * Computes the 3d coordinates of ith hammersley point of the objId'th object
 */
vec3 hammersley_space(uint i, int objId)
{
	return voronoi_centres[objId * numPoints + i];
}

/**
 * Unused function
 */
uint nearest(vec2 uv)
{
	float nearest = length(hammersley(0) - uv);
	uint nearestIdx = 0;
	for(uint i = 1; i < numPoints; i++)
	{
		float len = length(hammersley(i) - uv);
		if(len < nearest)
		{
			nearestIdx = i;
			nearest = len;
		}
	}
	return nearestIdx;
}

/**
 * Unused function
 */
uint nearest_with_normal(vec2 uv, int objId, vec3 normal)
{
	float nearest = length(hammersley(0) - uv);
	uint nearestIdx = 0;
	for(uint i = 0; i < numPoints; i++)
	{
		if(dot(normalize(normals_per_voronoi[objId * numPoints + i]), normalize(normal)) > 0.866)
		{
			float len = length(hammersley(i) - uv);
			if(len < nearest)
			{
				nearestIdx = i;
				nearest = len;
			}
		}
	}
	return nearestIdx;
}

/**
 * Find the nearest voronoi centre, taking into account normal
 * @param pos
 * @param objId id of the parent object of pos
 * @paran normal normal at pos
 */
uint nearest_with_normal_space(vec3 pos, int objId, vec3 normal)
{
	float nearest = MAX_LENGTH;
	uint nearestIdx = 0;
	for(uint i = 0; i < numPoints; i++)
	{
		if(normals_per_voronoi[objId * numPoints + i] != vec3(0.0))
		{
			if(dot(normalize(normals_per_voronoi[objId * numPoints + i]), normalize(normal)) > 0.866)
			{
				float len = length(voronoi_centres[objId * numPoints + i] - pos);
				if(len < nearest)
				{
					nearestIdx = i;
					nearest = len;
				}
			}
		}
	}
	return nearestIdx;
}

/**
 * Find the voronoi centre of pos
 */
uint findCell(vec3 pos, int objId, vec3 normal)
{
	return nearest_with_normal_space(pos, objId, normal);
}

/**
 * Find max value of QTable for the voronoi centre idx
 * @param idx index of voronoi cell
 * @param objId
 * @return maximum Q value out of all the directions at centre idx
 */
float maxQ(uint idx, int objId)
{
	float mq = 0;
	int base = numPoints * maxTheta * maxPhi * objId + maxTheta * maxPhi * int(idx);
	for(int i=0; i < maxTheta; i++)
	{
		for(int j=0; j < maxPhi; j++)
		{
			mq = max(mq,qt[base + i * maxPhi + j]);
		}
	}
	return mq;
}

/**
 * Find any orthogonal vector to v
 * @param v
 * @return v' orthogonal to v
 */
vec3 orthogonal(vec3 v)
{
	return abs(v.x) > abs(v.z) ? vec3(-v.y, v.x, 0.0)
								: vec3(0.0, -v.z, v.y);
}

/**
 * Find the unwrapped index of QTable of direction at a voronoi cell
 * @param idx index of voronoi cell
 * @param objId
 * @param dir
 * @param n normal at voronoi cell
 * @return unwrapped index of the direction in the QTable SSBO
 */
int findIndex(uint idx, int objId, vec3 dir, vec3 n)
{
	vec3 normdir = normalize(dir);
	vec3 nvec = normalize(n);
	vec3 uvec = normalize(orthogonal(nvec));
	vec3 vvec = normalize(cross(nvec, uvec));

	float np = dot(nvec, normdir);
	int u = int(np * float(maxTheta));
	float up = dot(uvec, normdir);
	float vp = dot(vvec, normdir);
	float theta = 0;
	float ctheta = up/sqrt(1-np*np);

	if(vp > 0)
	{
		theta = fastacos(ctheta);
	}
	else
	{
		theta = 2 * PI - fastacos(ctheta);
	}
	int v = int((theta / (2 * PI)) * float(maxPhi));

	int base = numPoints * maxTheta * maxPhi * objId + maxTheta * maxPhi * int(idx);

	int id = base + u * maxPhi + v;

	return id;
}

/**
 * Update the QTable
 * @param oSI surface interaction of origin of ray
 * @param hSI surface interaction of hitpoint of ray
 * @param dir direction of ray
 * @param lastAttenuation beta till this ray
 * @param prevIdx Voronoi cell index of ray origin (input -1 if want to calculate)
 * @param[out] nextIdx Voronoi cell index of ray hitpoint (becomes prevIdx for the next ray)
 * @return status of updation
 */
int updateQtable(surfaceInteraction oSI,  surfaceInteraction hSI, vec3 dir, float lastAttenuation, int prevIdx, out int nextIdx)
{
	uint idxPrev;
	if(prevIdx >= 0)
	{
		idxPrev = prevIdx;
	}
	else
	{
		idxPrev = findCell(oSI.point, triangles[oSI.tridx].objId, oSI.normal);
	}
	uint idxCurr = findCell(hSI.point, triangles[hSI.tridx].objId, hSI.normal);
	nextIdx = int(idxCurr);
	float update = 0;

	if(mtls[hSI.matidx].emitter)
	{
		update = clamp(length(mtls[hSI.matidx].Ke), 0.0, 1.0);
	}
	else
		update = lastAttenuation * maxQ(idxCurr, triangles[hSI.tridx].objId);

	int idxQ = findIndex(idxPrev, triangles[oSI.tridx].objId, -dir, oSI.normal);

	float alpha = 1.0 / (1.0 + visits[numPoints * triangles[oSI.tridx].objId + int(idxPrev)]);
	visits[numPoints * triangles[oSI.tridx].objId + int(idxPrev)]++;
	qt[idxQ] = (1- alpha) * qt[idxQ] + alpha * update;

	if(isnan(lastAttenuation))
		return -1;

	return 0;
}

/**
 * Samples the cdf of QTable at voronoi cell
 * @param idx index of voronoi cell
 * @param objId
 * @param rand random input
 * @param[out] pdf of result
 * @return theta,phi of the QTable cell
 */
ivec2 sampleCdf(int idx, int ObjId, float rand, out float pdf)
{
	float cdf = 0;
	float den = 0;
	int base = numPoints * maxPhi * maxTheta * ObjId + maxPhi * maxTheta * idx;
	for(int j = 0; j < maxTheta; j++)
	{
		for(int k = 0; k < maxPhi; k++)
		{
			den += qt[base +  j * maxPhi + k];
		}
	}

	for(int j = 0; j < maxTheta; j++)
	{
		for(int k = 0; k < maxPhi; k++)
		{
			cdf += qt[base +  j * maxPhi + k];

			if(cdf/den >= rand)
			{
				pdf = qt[base + j * maxPhi + k] / den;
				return ivec2(j,k);
			}
		}
	}
}

/**
 * Compute the vector in world coordinate system
 * @param n normal at surface
 * @param dir direction in local coordinate system
 * @return wcs
 */
vec3 aroundv(vec3 n, vec3 dir)
{
	vec3 nvec = normalize(n);
	vec3 uvec = normalize(orthogonal(nvec));
	vec3 vvec = normalize(cross(nvec, uvec));

	return normalize(uvec * dir.x + vvec * dir.y + nvec * dir.z);
}

/**
 * Uniformly sample one patch of direction in the given QTable
 * @param idxPatch theta,phi of patch
 * @param SI
 * @param rand 2 random inputs
 * @return Sampled direction in the patch
 */
vec3 uniformSamplePatch(surfaceInteraction SI, ivec2 idxPatch, spatialrand rand)
{
	int nu = idxPatch.y;
	int nv = idxPatch.x;

	float u = (float(nu) + rand.x)/float(maxPhi);
	float v = (float(nv) + rand.y)/float(maxTheta);


	vec3 dir = vec3(cos(2*PI*v) * sqrt(1-u*u), sin(2*PI*v) * sqrt(1-u*u), u);
	return aroundv(SI.normal, dir);
}

/**
 * Compute the scattered ray direction and pdf
 * @param SI
 * @param QIdx QTable index of the voronoi cell of ray
 * @param rand 3 random inputs
 * @param[out] pdf of sampled ray
 * @return sampled ray
 */
vec3 sampleScatteringDir(surfaceInteraction SI, int QIdx, vec3 rand, out float outpdf)
{
	uint idx;
	if(QIdx >= 0)
	{
		idx = QIdx;
	}
	else
	{
		idx = findCell(SI.point, triangles[SI.tridx].objId, SI.normal);
	}
	
	float pdf;
	ivec2 idxPatch = sampleCdf(int(idx), triangles[SI.tridx].objId, rand.x, pdf);

	vec3 dir = uniformSamplePatch(SI, idxPatch, rand.yz);

	outpdf = pdf * maxPhi * maxTheta / (2 * PI);

	return dir;
}