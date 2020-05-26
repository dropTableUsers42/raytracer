#include "filter.hpp"

Filter::Filter(GLuint width, GLuint height, GLuint inputTex, GLuint normTex, GLuint posTex) : quad("src/shaders/atrous.vs", "src/shaders/atrous.fs"), c_phi0(3.3f), n_phi0(1E-2f), p_phi0(5.5f), iterations(2)
{
	quad.init();
	this->width = width;
	this->height = height;
	this->inputTex = inputTex;
	this->normTex = normTex;
	this->posTex = posTex;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	createTextureAttachments(width, height);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	quad.shader.use();
	quad.shader.setInt("colMap", 0);
	quad.shader.setInt("normMap", 1);
	quad.shader.setInt("posMap", 2);
	int loc = glGetUniformLocation(quad.shader.ID, "offset");
	for(int i = 0, y = -1; y <= 1; y++)
	{
		for(int x = -1; x <= 1; x++, i++)
		{
			glUniform2i(loc + i, x, y);
		}
	}

	/*float kernel[] = {1.0f / 256.0f, 1.0f / 64.0f, 3.0f / 128.0f, 1.0f / 64.0f,
		1.0f / 256.0f, 1.0f / 64.0f, 1.0f / 16.0f, 3.0f / 32.0f, 1.0f / 16.0f, 1.0f / 64.0f, 3.0f / 128.0f,
		3.0f / 32.0f, 9.0f / 64.0f, 3.0f / 32.0f, 3.0f / 128.0f, 1.0f / 64.0f, 1.0f / 16.0f, 3.0f / 32.0f,
		1.0f / 16.0f, 1.0f / 64.0f, 1.0f / 256.0f, 1.0f / 64.0f, 3.0f / 128.0f, 1.0f / 64.0f, 1.0f / 256.0f};
	*/
	float kernel[] = {1.0f/16.0f, 1.0f/8.0f, 1.0f/16.0f,
					1.0f/8.0f, 1.0f/4.0f, 1.0f/8.0f,
					1.0f/16.0f, 1.0f/8.0f, 1.0f/16.0f};
	quad.shader.setFloatv("kernel", 9, kernel);
}

void Filter::createTextureAttachments(int width, int height)
{
	glGenTextures(1, &ftex0);
	glBindTexture(GL_TEXTURE_2D, ftex0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ftex0, 0);

	glGenTextures(1, &ftex1);
	glBindTexture(GL_TEXTURE_2D, ftex1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, ftex1, 0);
}

void Filter::filter()
{
	int read = inputTex, write = ftex0;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, posTex);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	quad.shader.use();
	for(int i = 0; i < iterations; i++)
	{
		quad.shader.setFloat("c_phi", 1.0f / i * c_phi0);
		quad.shader.setFloat("p_phi", 1.0f / (1 << i) * p_phi0);
		quad.shader.setFloat("n_phi", 1.0f / (1 << i) * n_phi0);
		quad.shader.setInt("stepwidth", (1 << (i + 1)) - 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, read);
		const GLenum bufferDraw[] = {(GLenum)(write == ftex0 ? GL_COLOR_ATTACHMENT0 : GL_COLOR_ATTACHMENT1)};
		glDrawBuffers(1, bufferDraw);

		quad.render();

		if(i == 0)
		{
			read = ftex1;
		}
		int tmp = read;
		read = write;
		write = tmp;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	readFrom = read;
}

void Filter::setTextureRead()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, readFrom);
}