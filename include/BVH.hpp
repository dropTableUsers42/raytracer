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

/**
 * 	Class to store a single triangle for the scene
 */
class Triangle
{
	static constexpr float ONE_THIRD = 1.0f/3.0f;
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	int materialIndex; /**<index of the material of the triangle (in the materials array)*/
	int objId; /**<index of the object the triangle belongs to */

	Triangle();
	Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n1, glm::vec3 n2, glm::vec3 n3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, int materialIndex, int objId);
	
	/**
	 * @return centroid of triangle
	 */
	glm::vec3 centroid();
	
	/**
	 * @return maximum value of x,y,z coords out of the three vertices
	 */
	glm::vec3 max();
	
	/**
	 * @return minimum value of x,y,z coords out of the three vertices
	 */
	glm::vec3 min();

	/**
	 * Calculates the flat normals (this will overwrite any normals passed in the contructor)
	 */
	void calculateNormals();
};


/**
 * BVH Node
 */
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

/**
 * Container of the BVH accel structure
 */
class BVHContainer
{
	static constexpr int MAX_TRIANGLES_IN_NODE = 32; /** Max allowed triangles in node. Decrease for more nodes for more division
	std::vector<Triangle> triangles; /** Array of all triangles in container */
	std::vector<Material*> materials; /** Array of materials */
	BVH *root; /** Root BVH node*/
	std::map<BVH*, int> indexes; /** A map from the BVH node to its index in the indexesVector*/
	std::vector<BVH*> indexesVector; /** Array of all BVH nodes */
	std::vector<Triangle> triangles; /** Array of all triangles */
	std::stringbuf nodesBuffer, trianglesBuffer, mtlBuffer, emittersBuffer, normalsVoronoiBuffer, voronoiCentresBuffer; /** BVH Data to be passed into GPU */
	std::vector<std::vector<glm::vec3> > normals_per_voronoi; /** A 2D array storing the normal at the surface of each voronoi point, indexed by the objId and the index of the voronoi point on the unwrapped uv map of that object */
	std::vector<std::vector<glm::vec3> > voronoi_centres; /** A 2D array storing the position of each voronoi point, indexed by the objId and the index of the voronoi point on the unwrapped uv map of that object */


	/**
	 * Helper function to put any type of data into stringbuf
	 * @param buf Buffer to put data in
	 * @param var Data
	 * @return buf Buffer that data was put in
	 */
	template<class Type>
	std::stringbuf& put(std::stringbuf& buf, const Type& var);

	/**
	 * Helper function to extract any type of data into stringbuf
	 * @param buf Buffer to get data from
	 * @param[out] var Variable which will store the data after function call
	 * @return buf Buffer that data was extracted from
	 */
	template<class Type>
	std::stringbuf& get(std::stringbuf& buf, Type& var);
public:
	BVHContainer(std::vector<Triangle> triangles, std::vector<Material*> materials, std::vector<Triangle> emitters, std::vector<std::vector<glm::vec3> > normals_per_voronoi, std::vector<std::vector <glm::vec3> > voronoi_centres);
	
	/**
	 * Builds the BVH structure recursively and fills the lefr and right nodes of each BVH node
	 * @param triangles Array of triangles to build BVH of
	 * @return root node of the resulting BVH
	 */
	BVH* buildBVH(std::vector<Triangle> &triangles) const;

	/**
	 * Fills the hitNext and missNext nodes of each node of the BVH structure
	 * @param root root node of the BVH
	 */
	void buildNextLinks(BVH *root);

	/**
	 * 'Unwraps' the BVh structure into an array in preorder traversal and also stores the mapping of node into index
	 * @param root root node of BVH
	 * @param[out] indexes stores the mapping of node into index
	 */
	void allocateIndexes(BVH *root, std::map<BVH*, int> &indexes);

	/**
	 * Stores the BVH data in the stringbuf in the std430 layout of GPU
	 */
	void bvhToSsbo();

	/**
	 * Stores the stringbuf data in the Shader Storage Buffer Object
	 */
	void sendDataToGPU();

	GLuint nodesSsbo, trianglesSsbo, materialsSsbo, emittersSsbo, normalsVoronoiSsbo, voronoiCentresSsbo;
	std::vector<Triangle> emitters; /**< Array of all emitter triangles*/

};

/**
 * Stores each BVH node as they are represented in GPU memory layout
 */
class GPUNode
{
public:
	float min[3], max[3];
	int hitNext, missNext, firstTri, numTris;
};

#endif