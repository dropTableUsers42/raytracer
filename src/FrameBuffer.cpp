#include "Framebuffer.hpp"

FrameBuffer::FrameBuffer(int width, int height)
{
	this->width = width;
	this->height = height;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	createTextureAttachment(width, height);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::createTextureAttachment(int width, int height)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
}

void FrameBuffer::resizeTextureAttachment(int width, int height)
{
	this->width = width;
	this->height = height;
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::renderToFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

void FrameBuffer::renderToScreen()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::saveToImage(std::string path)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	GLubyte *data = new GLubyte[width * height * 4];
	glGetTexImage(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);
	//std::cerr << glGetError() << std::endl;

	stbi_flip_vertically_on_write(true);
	stbi_write_jpg(path.c_str(), width, height, 4, data, 100);

	std::cout << "Image saved to: " << path << std::endl;
	delete data;
}

void FrameBuffer::deleteFramebuffer()
{
	glDeleteFramebuffers(1, &FBO);
}

void FrameBuffer::setTextureRead()
{
	glBindTexture(GL_TEXTURE_2D, texture);
	//glActiveTexture(GL_TEXTURE0);
}

glm::ivec2 FrameBuffer::getTextureDims()
{
	return glm::ivec2(width, height);
}