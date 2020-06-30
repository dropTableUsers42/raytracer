#include "main.hpp"

#define WIDTH 512
#define HEIGHT 512

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cout << "Usage:\t" << argv[0] << "\tobj_file_path"<< std::endl;
		return -1;
	}
	GlfwSetter glfwSetter;
	GLFWwindow* window;
	int glfwError = glfwSetter.init(WIDTH, HEIGHT, window);
	if(glfwError)
	{
		return glfwError;
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	TextRenderer txt(WIDTH, HEIGHT);

	Quad quad("src/shaders/quad.vs", "src/shaders/quad.fs");
	quad.init();

	FrameBuffer framebuffer(WIDTH, HEIGHT);
	Filter filter(WIDTH, HEIGHT, framebuffer.texture, framebuffer.normalTex, framebuffer.posTex);
	GlfwSetter::framebuffer = &framebuffer;
	FrameBuffer framebufferfinal(WIDTH, HEIGHT);

	ModelLoader loader(argv[1]);
	QTable qt(loader.numObjs);
	qt.makeVoronoi(loader.triangles_per_obj);
	BVHContainer bvh(loader.triangles, loader.materialsVector, loader.emitters, qt.normal_per_voronoi, qt.voronoi_centres);

	Camera camera(
		glm::vec3(0.0, 2.2, 3.5),
		glm::vec3(0.0, 3.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0),
		60.0,
		WIDTH,
		HEIGHT,
		1.0f,
		2.0f
	);
	GlfwSetter::camera = &camera;

	Tracer tracer(&camera, &framebuffer, &bvh, &qt);
	GlfwSetter::importanceSampled = &tracer.importanceSampled;
	GlfwSetter::useBlueNoise = &tracer.useBlueNoise;
	GlfwSetter::maxAccumulateSamples = &tracer.maxAccumulateSamples;
	GlfwSetter::accumulateSamples = &tracer.accumulateSamples;
	GlfwSetter::freezeTime = &tracer.freezeTime;
	GlfwSetter::iterations = &filter.iterations;
	GlfwSetter::maxBounces = &tracer.maxBounces;
	
	FPSCounter fps;
	fps.setTimerFreq(1000.0);
	fps.resetCounter();
	glClear(GL_COLOR_BUFFER_BIT);
	while(!glfwWindowShouldClose(window))
	{
		tracer.trace(GlfwSetter::frameNumber);
		/*if(GlfwSetter::frameNumber == 10)
		{
			framebuffer.saveToImage("simpleRL10.png");
		}
		if(GlfwSetter::frameNumber == 100)
		{
			framebuffer.saveToImage("simpleRL100.png");
		}
		if(GlfwSetter::frameNumber == 400)
		{
			framebuffer.saveToImage("simpleRL400.png");
		}
		if(GlfwSetter::frameNumber == 1000)
		{
			framebuffer.saveToImage("simpleRL1000.png");
		}*/
		if(GlfwSetter::filter)
		{
			filter.filter();
			filter.setTextureRead();
		} else
		{
			framebuffer.setTextureRead();
		}
		glClear(GL_COLOR_BUFFER_BIT);
		quad.shader.use();
		quad.shader.setFloat("exposure", GlfwSetter::exposure);
		framebufferfinal.renderToFramebuffer();
		quad.render();
		framebufferfinal.renderToScreen();
		quad.render();

		/*if(GlfwSetter::frameNumber == 10)
		{
			framebufferfinal.saveToImage("simple10RLpost.png");
		}
		if(GlfwSetter::frameNumber == 100)
		{
			framebufferfinal.saveToImage("simple100RLpost.png");
		}
		if(GlfwSetter::frameNumber == 400)
		{
			framebufferfinal.saveToImage("simple400RLpost.png");
		}
		if(GlfwSetter::frameNumber == 1000)
		{
			framebufferfinal.saveToImage("simple1000RLpost.png");
		}*/

		std::string str = "MSPF: ";
		str += std::to_string(fps.getMillisecondsPerFrame());
		txt.RenderText(str, 25.0f, HEIGHT- 25.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
		str = "FPS: ";
		str += std::to_string(fps.getFPS());
		txt.RenderText(str, 25.0f, HEIGHT - 49.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
		str = "Frames Rendered: ";
		str += std::to_string(GlfwSetter::frameNumber);
		txt.RenderText(str, 25.0f, HEIGHT - 73.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

		GlfwSetter::frameNumber++;
		glfwSwapBuffers(window);
		glfwPollEvents();
		fps.updateCounter();
		fps.timer();
	}
	std::cout << std::endl;
	glfwTerminate();
	return 0;
}