#ifndef _GLFW_SETTER_H
#define _GLFW_SETTER_H

#include "Framebuffer.hpp"
#include "Camera.hpp"

#include <glm/glm/gtx/string_cast.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

class GlfwSetter
{
	static int width;
	static int height;

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
	static bool *importanceSampled;
	static int frameNumber;
	static float distMoved;
};

#endif