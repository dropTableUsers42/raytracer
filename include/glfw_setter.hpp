#ifndef _GLFW_SETTER_H
#define _GLFW_SETTER_H

#include "Framebuffer.hpp"
#include "Camera.hpp"

#include <glm/glm/gtx/string_cast.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>


/**
 * Class to store all settings of the OpenGL project
 */
class GlfwSetter
{
	static int width;
	static int height;

	static float camHeight;

	static int lastX, lastY;
	static float yaw, pitch;
	static bool firstMouse;
public:
	int init(int width, int height, GLFWwindow* &window_in);
	static void glfw_error_callback(int error_code, const char* description);
	static void framebuffer_size_callback(GLFWwindow* window, int in_width, int in_height);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void getWindowDimensions(int &in_width, int &in_height);

	static FrameBuffer *framebuffer;
	static Camera *camera;
	static int *iterations;
	static bool *importanceSampled;
	static bool *useBlueNoise;
	static int frameNumber;
	static float distMoved;
	static int *maxAccumulateSamples;
	static bool *accumulateSamples;
	static bool *freezeTime;
	static bool filter;
	static int *maxBounces;
	static float exposure;
};

#endif