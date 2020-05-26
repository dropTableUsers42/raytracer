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

Tracer::Tracer(Camera *camera, FrameBuffer *fbo, BVHContainer *bvh) : compute(std::vector<const char*>{"src/shaders/trace3.cs", "src/shaders/geometry.glsl", "src/shaders/random.glsl", "src/shaders/qtablehelper.glsl"}), camera(camera), fbo(fbo), importanceSampled(true), useBlueNoise(true), accumulateSamples(true), maxAccumulateSamples(64), bvh(bvh), maxBounces(5)
{
	start = std::chrono::system_clock::now();

	//genBlueNoiseTexes();
	//genLtcData();

}

void Tracer::genLtcData()
{
	float * ltcdata = new float[4 * 64 * 64];
	std::ifstream is("res/ltc1.data", std::ios::binary);
	is.read(reinterpret_cast<char*>(ltcdata), 4 * 64 * 64 * sizeof(ltcdata[0]));
	glGenTextures(1, &LtcMatTex);
	glBindTexture(GL_TEXTURE_2D, LtcMatTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 64, 64);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_FLOAT, ltcdata);

	is.open("res/ltc2.data", std::ios::binary);
	is.read(reinterpret_cast<char*>(ltcdata), 4 * 64 * 64 * sizeof(ltcdata[0]));
	glGenTextures(1, &LtcMagTex);
	glBindTexture(GL_TEXTURE_2D, LtcMagTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 64, 64);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_FLOAT, ltcdata);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Tracer::genBlueNoiseBuffers()
{
	std::ifstream is("res/sobol_256_256_4spp.data", std::ios::binary);
	is.ignore(std::numeric_limits<std::streamsize>::max());
	std::streamsize length = is.gcount();
	is.clear();
	is.seekg(0, std::ios_base::beg);
	uint8_t * bfdata = new uint8_t[length];
	is.read(reinterpret_cast<char*>(bfdata), length * sizeof(bfdata[0]));
	glGenBuffers(1, &sobolBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sobolBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, length * sizeof(bfdata[0]), bfdata, GL_STATIC_DRAW);
	is.close();
	delete[] bfdata;

	is.open("res/scramble_128_128_8_4spp.data", std::ios::binary);
	is.ignore(std::numeric_limits<std::streamsize>::max());
	length = is.gcount();
	is.clear();
	is.seekg(0, std::ios_base::beg);
	bfdata = new uint8_t[length];
	is.read(reinterpret_cast<char*>(bfdata), length * sizeof(bfdata[0]));
	glGenBuffers(1, &scrambleBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, scrambleBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, length * sizeof(bfdata[0]), bfdata, GL_STATIC_DRAW);
	is.close();
	delete[] bfdata;

	is.open("res/ranking_128_128_8_4spp.data", std::ios::binary);
	is.ignore(std::numeric_limits<std::streamsize>::max());
	length = is.gcount();
	is.clear();
	is.seekg(0, std::ios_base::beg);
	bfdata = new uint8_t[length];
	is.read(reinterpret_cast<char*>(bfdata), length * sizeof(bfdata[0]));
	glGenBuffers(1, &rankingBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rankingBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, length * sizeof(bfdata[0]), bfdata, GL_STATIC_DRAW);
	is.close();
	delete[] bfdata;
}

void Tracer::genBlueNoiseTexes()
{
	int width, height, nrChannels;
	unsigned char *data = stbi_load("res/blueNoise.png", &width, &height, &nrChannels, 0);
	glGenTextures(1, &blueNoiseTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blueNoiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);

	genBlueNoiseBuffers();
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
	//if(!accumulateSamples && frameNumber > maxAccumulateSamples)
	//	return;
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
	//compute.setFloat("phongExponent", 128.0f);
	compute.setInt("importanceSampled", int(importanceSampled));
	//compute.setInt("multipleImportanceSampled", int(importanceSampled));
	//compute.setInt("useBlueNoise", int(useBlueNoise));
	//compute.setFloat("specularFactor", 0.2f);
	//compute.setFloat("roughness", 0.5f);
	//compute.setInt("frameIndex", frameNumber);
	//compute.setInt("blueNoiseTex", 0);
	//compute.setInt("ltc_mat", 1);
	//compute.setInt("ltc_mag", 2);
	compute.setInt("numEmitters", bvh->emitters.size());
	compute.setInt("maxBounces", maxBounces);

	int workSizeX = nextPowerOfTwo(fbo->getTextureDims().x);
	int workSizeY = nextPowerOfTwo(fbo->getTextureDims().y);
	glBindImageTexture(glGetUniformLocation(compute.ID, "framebuffer"), fbo->texture, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(glGetUniformLocation(compute.ID, "normbuffer"), fbo->normalTex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA8_SNORM);
	glBindImageTexture(glGetUniformLocation(compute.ID, "posbuffer"), fbo->posTex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sobolBuffer);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scrambleBuffer);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rankingBuffer);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvh->nodesSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bvh->trianglesSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bvh->materialsSsbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bvh->emittersSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDispatchCompute(workSizeX/compute.workGroupSize[0], workSizeY/compute.workGroupSize[1], 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);

	/*glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blueNoiseTexture);*/
	/*glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, LtcMatTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, LtcMagTex);*/
	
	glActiveTexture(GL_TEXTURE0);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glUseProgram(0);
}