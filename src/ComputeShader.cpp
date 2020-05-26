#include "ComputeShader.hpp"

ComputeShader::ComputeShader(std::vector<const char*> shaderPathVector)
{

	std::vector<std::string> shaderCode;
	std::ifstream shaderFile;

	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		for(auto it = shaderPathVector.begin(); it != shaderPathVector.end(); it++)
		{
			shaderFile.open(*it);
			std::stringstream shaderStream;

			shaderStream << shaderFile.rdbuf();
			shaderFile.close();
			shaderCode.push_back(shaderStream.str());
		}
	}
	catch(std::ifstream::failure e)
	{
		std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	std::vector<const char*> shader_Code;

	for(auto it = shaderCode.begin(); it != shaderCode.end(); it++)
	{
		shader_Code.push_back(it->c_str());
	}

	std::vector<unsigned int> shader;
	int success;
	char infoLog[512];

	for(auto it = shader_Code.begin(); it != shader_Code.end(); it++)
	{
		int curShader;
		curShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(curShader, 1, &(*it), NULL);
		glCompileShader(curShader);
		shader.push_back(curShader);
	}

	for(auto it = shader.begin(); it != shader.end(); it++)
	{
		glGetShaderiv(*it, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(*it, 512, NULL, infoLog);
			std::cerr << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
	}

	ID = glCreateProgram();

	for(auto it = shader.begin(); it != shader.end(); it++)
	{
		glAttachShader(ID, *it);
	}
	glLinkProgram(ID);

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	for(auto it = shader.begin(); it != shader.end(); it++)
	{
		glDeleteShader(*it);
	}

	workGroupSize.resize(3);
	glGetProgramiv(ID, GL_COMPUTE_WORK_GROUP_SIZE, &workGroupSize[0]);
}

void ComputeShader::use()
{
	glUseProgram(ID);
}

void ComputeShader::setBool(const std::string &name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void ComputeShader::setInt(const std::string &name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setFloat(const std::string &name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void ComputeShader::setVec3(const std::string &name, glm::vec3 value) const
{
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void ComputeShader::setMat4(const std::string &name, glm::mat4 value) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}