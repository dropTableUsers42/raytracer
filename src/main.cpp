#include "main.hpp"

#define WIDTH 1200
#define HEIGHT 800

int main(int argc, char** argv)
{
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

	Quad quad;
	quad.init();

	FrameBuffer framebuffer(WIDTH, HEIGHT);
	GlfwSetter::framebuffer = &framebuffer;

	Camera camera(
		glm::vec3(-4.0, 3.0, 3.0),
		glm::vec3(0.0, 0.5, 0.0),
		glm::vec3(0.0, 1.0, 0.0),
		60.0,
		WIDTH,
		HEIGHT,
		1.0f,
		2.0f
	);
	GlfwSetter::camera = &camera;

	Tracer tracer(&camera, &framebuffer);
	GlfwSetter::importanceSampled = &tracer.importanceSampled;
	
	FPSCounter fps;
	fps.setTimerFreq(1000.0);
	fps.resetCounter();
	glClear(GL_COLOR_BUFFER_BIT);
	while(!glfwWindowShouldClose(window))
	{
		tracer.trace(GlfwSetter::frameNumber);
		//framebuffer.renderToFramebuffer();
		//glClearColor(0.0f,0.0f,0.0f,0.0f);
		//glClear(GL_COLOR_BUFFER_BIT);

		framebuffer.setTextureRead();
		framebuffer.renderToScreen();
		glClear(GL_COLOR_BUFFER_BIT);
		quad.render();

		std::string str = "MSPF: ";
		str += std::to_string(fps.getMillisecondsPerFrame());
		txt.RenderText(str, 25.0f, HEIGHT- 25.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
		str = "FPS: ";
		str += std::to_string(fps.getFPS());
		txt.RenderText(str, 25.0f, HEIGHT - 49.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

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