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
#include "QTable.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>

/**
 * Class to execute the ray tracing program
 */
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
	QTable *qt;
public:
	Tracer(Camera *camera, FrameBuffer *fbo, BVHContainer *bvh, QTable *qt);
	void genBlueNoiseBuffers();
	void genBlueNoiseTexes();
	void genLtcData();

	/**
	 * Execute the tracing compute shader and write the result to current framebuffer
	 */
	void trace(int frameNumber);

	bool importanceSampled;
	bool useBlueNoise;
	bool freezeTime;
	bool accumulateSamples;
	int maxAccumulateSamples;
	int maxBounces;
};

#endif