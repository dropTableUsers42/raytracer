#include "Quad.hpp"

Quad::Quad() : numVertices(4), numIndices(6), shader("src/shaders/quad.vs", "src/shaders/quad.fs")
{
	vertices.resize(numVertices);
	vertices[0] = glm::vec2( 1.0f, 1.0f);
	vertices[1] = glm::vec2( 1.0f,-1.0f);
	vertices[2] = glm::vec2(-1.0f,-1.0f);
	vertices[3] = glm::vec2(-1.0f, 1.0f);

	indices.resize(6);
	indices[0] = 0; indices[1] = 1; indices[2] = 3;
	indices[3] = 1; indices[4] = 2; indices[5] = 3;
}

void Quad::init()
{
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
	glEnableVertexAttribArray(0);
}

void Quad::render()
{
	shader.use();
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

unsigned int Quad::getNumVertices()
{
	return numVertices;
}