#include "ComputeShader.hpp"

ComputeShader::ComputeShader(const char* shaderPath, const char* extraShaderPath)
{
	std::string shaderCode;
	std::string extraShaderCode;
	std::ifstream shaderFile;
	std::ifstream extraShaderFile;

	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	extraShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		shaderFile.open(shaderPath);
		if(extraShaderPath != NULL)
			extraShaderFile.open(extraShaderPath);
		std::stringstream shaderStream;
		std::stringstream extraShaderStream;

		shaderStream << shaderFile.rdbuf();
		if(extraShaderPath != NULL)
			extraShaderStream << extraShaderFile.rdbuf();

		shaderFile.close();
		if(extraShaderPath != NULL)
			extraShaderFile.close();

		shaderCode = shaderStream.str();
		if(extraShaderPath != NULL)
			extraShaderCode = extraShaderStream.str();
	}
	catch(std::ifstream::failure e)
	{
		std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	const char* shader_Code = shaderCode.c_str();
	const char* extraShader_Code = extraShaderCode.c_str();

	unsigned int shader;
	unsigned int extraShader;
	int success;
	char infoLog[512];

	shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shader, 1, &shader_Code, NULL);
	glCompileShader(shader);

	if(extraShaderPath != NULL)
	{
		extraShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(extraShader, 1, &extraShader_Code, NULL);
		glCompileShader(extraShader);
	}

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	if(extraShaderPath != NULL)
	{
		glGetShaderiv(extraShader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(extraShader, 512, NULL, infoLog);
			std::cerr << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
	}

	ID = glCreateProgram();
	glAttachShader(ID, shader);
	if(extraShaderPath != NULL)
		glAttachShader(ID, extraShader);
	glLinkProgram(ID);

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(shader);
	glDeleteShader(extraShader);
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