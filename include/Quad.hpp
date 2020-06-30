#ifndef _QUAD_H
#define _QUAD_H

#include <glad/glad.h>
#include <glm/glm/vec2.hpp>

#include <vector>

#include "Shader.hpp"
#include "Object.hpp"

/**
 * Simple Screen covering quad
 */
class Quad: virtual public Object
{
	unsigned int numVertices;
	unsigned int numIndices;
	std::vector<glm::vec2> vertices;
	std::vector<unsigned int> indices;
	GLuint VAO, VBO, EBO;

public:
	Quad(const char * vertexPath, const char * fragmentPath);
	void init();
	void render();
	unsigned int getNumVertices();
	Shader shader;
};

#endif