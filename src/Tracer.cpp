#include "Tracer.hpp"

int nextPowerOfTwo(int x) {
	x--;
	x |= x >> 1; // handle 2 bit numbers
	x |= x >> 2; // handle 4 bit numbers
	x |= x >> 4; // handle 8 bit numbers
	x |= x >> 8; // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
	return x;
}

Tracer::Tracer(Camera *camera, FrameBuffer *fbo) : compute("src/shaders/trace1.cs", "src/shaders/random.glsl"), camera(camera), fbo(fbo), importanceSampled(true)
{
	start = std::chrono::system_clock::now();
}

void Tracer::updateRays()
{
	glm::mat4 invViewProjMat = camera->getPerspectiveMatrix() * camera->getViewMatrix();
	invViewProjMat = glm::transpose(glm::inverse(invViewProjMat));

	ray00 = glm::vec4(-1, -1, 0, 1) * invViewProjMat;
	ray00 /= ray00.w;
	ray00 -= glm::vec4(camera->eye, 0.0f);

	ray01 = glm::vec4(-1, +1, 0, 1) * invViewProjMat;
	ray01 /= ray01.w;
	ray01 -= glm::vec4(camera->eye, 0.0f);

	ray10 = glm::vec4(+1, -1, 0, 1) * invViewProjMat;
	ray10 /= ray10.w;
	ray10 -= glm::vec4(camera->eye, 0.0f);

	ray11 = glm::vec4(+1, +1, 0, 1) * invViewProjMat;
	ray11 /= ray11.w;
	ray11 -= glm::vec4(camera->eye, 0.0f);
}

void Tracer::trace(int frameNumber)
{
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	updateRays();
	compute.use();
	compute.setVec3("ray00", glm::vec3(ray00));
	compute.setVec3("ray01", glm::vec3(ray01));
	compute.setVec3("ray10", glm::vec3(ray10));
	compute.setVec3("ray11", glm::vec3(ray11));
	compute.setVec3("eye", camera->eye);
	compute.setFloat("time", elapsed_seconds.count());
	compute.setFloat("blendFactor", (float)frameNumber/(frameNumber + 1.0f));
	compute.setFloat("phongExponent", 128.0f);
	compute.setInt("multipleImportanceSampled", int(importanceSampled));
	compute.setFloat("specularFactor", 0.2f);

	int workSizeX = nextPowerOfTwo(fbo->getTextureDims().x);
	int workSizeY = nextPowerOfTwo(fbo->getTextureDims().y);
	glBindImageTexture(0, fbo->texture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glDispatchCompute(workSizeX/compute.workGroupSize[0], workSizeY/compute.workGroupSize[1], 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}