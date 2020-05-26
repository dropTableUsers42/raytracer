#ifndef _FILTER_H_
#define _FILTER_H_

#include <glad/glad.h>

#include "Quad.hpp"

#include <iostream>
#include <chrono>
#include <ctime>

class Filter
{
	int width, height;
	Quad quad;
public:
	GLuint inputTex, normTex, posTex, ftex0, ftex1;
	GLuint FBO;
	int iterations;
	int readFrom;

	float c_phi0, n_phi0, p_phi0;

	Filter(GLuint width, GLuint height, GLuint inputTex, GLuint normTex, GLuint posTex);
	void createTextureAttachments(int width, int height);
	void filter();
	void setTextureRead();
};

#endif