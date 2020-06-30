#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <glad/glad.h>

#include <iostream>
#include <string>
#include <fstream>

#include <glm/glm/vec2.hpp>

#include "stb_image.hpp"
#include "stb_image_write.hpp"

/**
 * Class to store a framebuffer
 */
class FrameBuffer
{
	GLuint FBO;
	GLuint width, height;
public:
	GLuint texture; /**< Color texture*/
	GLuint normalTex, posTex; /**< Normal and position textures */

	FrameBuffer(int width, int height);
	void createTextureAttachment(int width, int height);
	void resizeTextureAttachment(int width, int height);
	void renderToFramebuffer();
	void renderToScreen();

	/**
	 * Saves the color texture to a png image
	 * @param path path to save to
	 */
	void saveToImage(std::string path);
	void deleteFramebuffer();
	void setTextureRead();
	glm::ivec2 getTextureDims();
};

#endif