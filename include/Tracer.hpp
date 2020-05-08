#ifndef _TRACER_H
#define _TRACER_H

#include <glm/glm/mat4x4.hpp>
#include <glm/glm/vec4.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "ComputeShader.hpp"
#include "Camera.hpp"
#include "Framebuffer.hpp"

#include <chrono>
#include <ctime>

class Tracer
{
	std::chrono::time_point<std::chrono::system_clock> start;
	ComputeShader compute;
	Camera *camera;
	FrameBuffer *fbo;
	glm::vec4 ray00, ray01, ray10, ray11;
	void updateRays();
public:
	Tracer(Camera *camera, FrameBuffer *fbo);
	void trace(int frameNumber);

	bool importanceSampled;
};

#endif