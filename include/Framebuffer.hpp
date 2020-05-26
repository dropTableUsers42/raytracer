#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <glad/glad.h>

#include <iostream>
#include <string>
#include <fstream>

#include <glm/glm/vec2.hpp>

#include "stb_image.hpp"
#include "stb_image_write.hpp"

class FrameBuffer
{
	GLuint FBO;
	GLuint width, height;
public:
	GLuint texture;
	GLuint normalTex, posTex;

	FrameBuffer(int width, int height);
	void createTextureAttachment(int width, int height);
	void resizeTextureAttachment(int width, int height);
	void renderToFramebuffer();
	void renderToScreen();
	void saveToImage(std::string path);
	void deleteFramebuffer();
	void setTextureRead();
	glm::ivec2 getTextureDims();
};

#endif