#ifndef _MODEL_LOADER_H_
#define _MODEL_LOADER_H_

#include <glm/glm/vec3.hpp>
#include <glm/glm/vec2.hpp>
#include <glm/glm/gtx/string_cast.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <glm/glm/vec3.hpp>
#include <glm/glm/gtx/string_cast.hpp>

#include <map>
#include <string>

#include "BVH.hpp"
#include "Material.hpp"

class ModelLoader
{
	std::vector<unsigned long long> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> Kd_vertices;
	std::vector<int> material_per_vertex;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;
	std::map<std::string, Material*> materials;
	std::map<std::string, int> materialIndexes;
public:
	std::vector<glm::vec3> out_vertices;
	std::vector<glm::vec2> out_uvs;
	std::vector<glm::vec3> out_normals;
	std::vector<Triangle> triangles;
	std::vector<Triangle> emitters;
	std::vector<Material*> materialsVector;

	ModelLoader(const char* path);
	void loadObj(const char* path);
	void loadMat(const char* path);
};

#endif