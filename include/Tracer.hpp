#ifndef _TRACER_H
#define _TRACER_H

#include <glm/glm/mat4x4.hpp>
#include <glm/glm/vec4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "ComputeShader.hpp"
#include "Camera.hpp"
#include "Framebuffer.hpp"
#include "stb_image.hpp"
#include "BVH.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>

class Tracer
{
	std::chrono::time_point<std::chrono::system_clock> start;
	ComputeShader compute;
	Camera *camera;
	FrameBuffer *fbo;
	GLuint blueNoiseTexture;
	GLuint sobolBuffer, scrambleBuffer, rankingBuffer;
	GLuint LtcMatTex, LtcMagTex;
	glm::vec4 ray00, ray01, ray10, ray11;
	void updateRays();
	BVHContainer *bvh;
public:
	Tracer(Camera *camera, FrameBuffer *fbo, BVHContainer *bvh);
	void genBlueNoiseBuffers();
	void genBlueNoiseTexes();
	void genLtcData();
	void trace(int frameNumber);

	bool importanceSampled;
	bool useBlueNoise;
	bool freezeTime;
	bool accumulateSamples;
	int maxAccumulateSamples;
	int maxBounces;
};

#endif