#ifndef _BVH_H
#define _BVH_H

#include <glad/glad.h>

#include <glm/glm/vec3.hpp>
#include <glm/glm/vec2.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtx/string_cast.hpp>

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>
#include <sstream>

#include "Material.hpp"

class Triangle
{
	static constexpr float ONE_THIRD = 1.0f/3.0f;
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	int materialIndex;

	Triangle();
	Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, int materialIndex);
	glm::vec3 centroid();
	glm::vec3 max();
	glm::vec3 min();
	void calculateNormals();
};

class BVH
{
public:
	BVH();
	glm::vec3 min, max;
	BVH *parent;
	BVH *left, *right;
	BVH *hitNext, *missNext;
	bool isLeft;
	std::vector<Triangle> triangles;
};

class BVHContainer
{
	static constexpr int MAX_TRIANGLES_IN_NODE = 32;
	std::vector<Triangle> triangles;
	std::vector<Material*> materials;
	BVH *root;
	std::map<BVH*, int> indexes;
	std::vector<BVH*> indexesVector;
	std::stringbuf nodesBuffer, trianglesBuffer, mtlBuffer, emittersBuffer;

	template<class Type>
	std::stringbuf& put(std::stringbuf& buf, const Type& var);

	template<class Type>
	std::stringbuf& get(std::stringbuf& buf, Type& var);
public:
	BVHContainer(std::vector<Triangle> triangles, std::vector<Material*> materials, std::vector<Triangle> emitters);
	BVH* buildBVH(std::vector<Triangle> &triangles) const;
	void buildNextLinks(BVH *root);
	void allocateIndexes(BVH *root, std::map<BVH*, int> &indexes);
	void bvhToSsbo();
	void sendDataToGPU();

	GLuint nodesSsbo, trianglesSsbo, materialsSsbo, emittersSsbo;
	std::vector<Triangle> emitters;

};

class GPUNode
{
public:
	float min[3], max[3];
	int hitNext, missNext, firstTri, numTris;
};

#endif