#ifndef _COMPUTE_SHADER_H
#define _COMPUTE_SHADER_H

#include <glad/glad.h>
#include <vector>

#include "glm/glm/vec4.hpp"
#include "glm/glm/mat4x4.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ComputeShader
{
public:
	GLuint ID;
	std::vector<GLint> workGroupSize;

	ComputeShader(const char* shaderPath, const char* extraShaderPath = NULL);
	void use();
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setVec3(const std::string &name, glm::vec3 value) const;
	void setMat4(const std::string &name, glm::mat4 value) const;
};

#endif