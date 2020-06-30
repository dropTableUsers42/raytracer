#include "glfw_setter.hpp"

int GlfwSetter::width = 0;
int GlfwSetter::height = 0;
int GlfwSetter::lastX = 0;
int GlfwSetter::lastY = 0;
float GlfwSetter::yaw = 0;
float GlfwSetter::pitch = 0;
bool GlfwSetter::firstMouse = true;
bool* GlfwSetter::importanceSampled = NULL;
bool* GlfwSetter::useBlueNoise = NULL;
FrameBuffer* GlfwSetter::framebuffer = NULL;
Camera* GlfwSetter::camera = NULL;
int GlfwSetter::frameNumber = 0;
float GlfwSetter::distMoved = 0;
float GlfwSetter::camHeight = 3.0;
int* GlfwSetter::maxAccumulateSamples = NULL;
bool* GlfwSetter::accumulateSamples = NULL;
bool* GlfwSetter::freezeTime = NULL;
int* GlfwSetter::iterations = NULL;
bool GlfwSetter::filter = false;
int* GlfwSetter::maxBounces = NULL;
float GlfwSetter::exposure = 1.0;

int GlfwSetter::init(int width, int height, GLFWwindow* &window_in)
{
	glfwSetErrorCallback(GlfwSetter::glfw_error_callback);

	if(!glfwInit())
	{
		std::cerr << "Error: Failed to initialize GLFW" << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "Ray Tracing Engine", NULL, NULL);
	window_in = window;
	if (window == NULL)
	{
		std::cerr << "Error: Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, GlfwSetter::framebuffer_size_callback);
	glfwSetKeyCallback(window, GlfwSetter::key_callback);
	glfwSetCursorPosCallback(window, GlfwSetter::mouse_callback);
	glfwSetScrollCallback(window, GlfwSetter::scroll_callback);

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	return 0;
}

void GlfwSetter::getWindowDimensions(int &width, int &height)
{
	width = GlfwSetter::width;
	height = GlfwSetter::height;
}

void GlfwSetter::glfw_error_callback(int error_code, const char* description)
{
	std::cerr << error_code << "\t" << description << std::endl;
}

void GlfwSetter::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	GlfwSetter::width = width;
	GlfwSetter::height = height;
	glViewport(0, 0, width, height);
	framebuffer->resizeTextureAttachment(width, height);
}

void GlfwSetter::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_F10 && action == GLFW_PRESS)
		framebuffer->saveToImage("image.png");
	if (key == GLFW_KEY_W)
	{
		glm::vec3 eyediff = glm::normalize(camera->lookAt - camera->eye) * 0.1f;
		eyediff.y = 0;
		if(camera->eye.x + eyediff.x > 5.0 || camera->eye.x + eyediff.x < -5.0)
		{
			eyediff.x = 0;
		}
		if(camera->eye.z + eyediff.z > 5.0 || camera->eye.z + eyediff.z < -5.0)
		{
			eyediff.z = 0;
		}
		glm::vec3 lookatdiff = eyediff;
		camera->eye += eyediff;
		camera->lookAt += lookatdiff;
		frameNumber = 0;
		distMoved += glm::length(eyediff);
	}
	if (key == GLFW_KEY_A)
	{
		glm::vec3 eyediff = glm::normalize(glm::cross(camera->lookAt - camera->eye, camera->up)) * 0.1f;
		eyediff.y = 0;
		if(camera->eye.x - eyediff.x > 5.0 || camera->eye.x - eyediff.x < -5.0)
		{
			eyediff.x = 0;
		}
		if(camera->eye.z - eyediff.z > 5.0 || camera->eye.z - eyediff.z < -5.0)
		{
			eyediff.z = 0;
		}
		glm::vec3 lookatdiff = eyediff;
		camera->eye -= eyediff;
		camera->lookAt -= lookatdiff;
		frameNumber = 0;
		distMoved += glm::length(eyediff);
	}
	if (key == GLFW_KEY_S)
	{
		glm::vec3 eyediff = glm::normalize(camera->lookAt - camera->eye) * 0.1f;
		eyediff.y = 0;
		if(camera->eye.x - eyediff.x > 5.0 || camera->eye.x - eyediff.x < -5.0)
		{
			eyediff.x = 0;
		}
		if(camera->eye.z - eyediff.z > 5.0 || camera->eye.z - eyediff.z < -5.0)
		{
			eyediff.z = 0;
		}
		glm::vec3 lookatdiff = eyediff;
		camera->eye -= eyediff;
		camera->lookAt -= lookatdiff;
		frameNumber = 0;
		distMoved += glm::length(eyediff);
	}
	if (key == GLFW_KEY_D)
	{
		glm::vec3 eyediff = glm::normalize(glm::cross(camera->lookAt - camera->eye, camera->up)) * 0.1f;
		eyediff.y = 0;
		if(camera->eye.x + eyediff.x > 5.0 || camera->eye.x + eyediff.x < -5.0)
		{
			eyediff.x = 0;
		}
		if(camera->eye.z + eyediff.z > 5.0 || camera->eye.z + eyediff.z < -5.0)
		{
			eyediff.z = 0;
		}
		glm::vec3 lookatdiff = eyediff;
		camera->eye += eyediff;
		camera->lookAt += lookatdiff;
		frameNumber = 0;
		distMoved += glm::length(eyediff);
	}
	if(key == GLFW_KEY_Z)
	{
		glm::vec3 eyediff = glm::normalize(camera->up) * 0.1f;
		eyediff.x = eyediff.z = 0;
		glm::vec3 lookatdiff = eyediff;
		camera -> eye += eyediff;
		camera -> lookAt += lookatdiff;
		frameNumber = 0;
	}
	if(key == GLFW_KEY_X)
	{
		glm::vec3 eyediff = glm::normalize(camera->up) * 0.1f;
		eyediff.x = eyediff.z = 0;
		if(camera->eye.y - eyediff.y < 0.0)
		{
			eyediff.y = 0;
		}
		glm::vec3 lookatdiff = eyediff;
		camera -> eye -= eyediff;
		camera -> lookAt -= lookatdiff;
		frameNumber = 0;
	}
	/*if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		*importanceSampled = !(*importanceSampled);
		frameNumber = 0;
	}
	if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		*useBlueNoise = !(*useBlueNoise);
		frameNumber = 0;
	}
	if(key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		*accumulateSamples = !(*accumulateSamples);
		frameNumber = 0;
	}
	if(key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		*freezeTime = !(*freezeTime);
		frameNumber = 0;
	}
	if(key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		*maxAccumulateSamples++;
		frameNumber = 0;
	}
	if(key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		*maxAccumulateSamples = std::max(1, *maxAccumulateSamples - 1);
		frameNumber = 0;
	}*/
	if(key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		*iterations = glm::max((*iterations)-1, 1);
	}
	if(key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		(*iterations)++;
	}
	if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		filter = !filter;
	}
	if(key == GLFW_KEY_COMMA && action == GLFW_PRESS)
	{
		(*maxBounces)++;
		frameNumber = 0;
	}
	if(key == GLFW_KEY_PERIOD && action == GLFW_PRESS)
	{
		(*maxBounces) = std::max(2, (*maxBounces)-1);
		frameNumber = 0;
	}
	if(key == GLFW_KEY_1)
	{
		exposure += 0.1;
	}
	if(key == GLFW_KEY_2)
	{
		exposure = std::max(0.0, exposure - 0.1); 
	}
}

void GlfwSetter::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if(firstMouse)
	{
		lastX = xpos;
		lastY = ypos;

		glm::vec3 dir1 = glm::normalize(camera->lookAt - camera->eye);

		double pitch1 = glm::degrees(asin(dir1.y));
		pitch = pitch1;
		double yaw1 = glm::degrees(asin(dir1.z/cos(glm::radians(pitch1))));
		double yaw2 = glm::degrees(acos(dir1.x/cos(glm::radians(pitch1))));

		if(yaw1 > 0)
		{
			if(yaw2 < 90.0f)
			{
				yaw = yaw1;
			}
			else if(yaw2 >= 90.0f)
			{
				yaw = yaw2;
			}
		}
		else if(yaw1 <= 0)
		{
			if(yaw2 < 90.0f)
			{
				yaw = yaw1;
			}
			else if(yaw2 >= 90.0f)
			{
				yaw = 180.0f - yaw1;
			}
		}
		
		firstMouse = false;
		return;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	const float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw = glm::mod(yaw + xoffset, 360.0f);
	pitch += yoffset;

	if(pitch > 89.0f)
		pitch = 89.0f;
	if(pitch < -89.0f)
		pitch = -89.0f;
	
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	camera->lookAt = camera->eye + glm::normalize(direction);
	frameNumber = 0;
}

void GlfwSetter::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	GLfloat tempfov = camera->fov;
	camera->fov = std::max(std::min(camera->fov - yoffset, 100.0), 45.0);
	if(camera->fov != tempfov)
		frameNumber = 0;
}