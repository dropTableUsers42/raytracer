#ifndef _FILTER_H_
#define _FILTER_H_

#include <glad/glad.h>

#include "Quad.hpp"

#include <iostream>
#include <chrono>
#include <ctime>

/**
 * A trous filter
 */
class Filter
{
	int width, height; /**< width & height of input texture */
	Quad quad; /**< simple Quad program */
public:
	GLuint inputTex, normTex, posTex, ftex0, ftex1;
	GLuint FBO;
	int iterations;
	int readFrom;

	float c_phi0, n_phi0, p_phi0;

	Filter(GLuint width, GLuint height, GLuint inputTex, GLuint normTex, GLuint posTex);

	/**
	 * Creates the texture attachments of the ping pong textures ftex0 and ftex1
	 * @param the dimensions of texture
	 */
	void createTextureAttachments(int width, int height);

	/**
	 * Applies the filer to the input texture
	 */
	void filter();

	/**
	 * Sets the shader to read from the output texture (which may be either ftex0 or ftex1 depending on number of iterations of filter)
	 */
	void setTextureRead();
};

#endif