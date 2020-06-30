#include "Camera.hpp"

#include <iostream>

Camera::Camera(glm::vec3 eye, glm::vec3 lookAt, glm::vec3 up, GLfloat fov, GLfloat x, GLfloat y, GLfloat near, GLfloat far) : eye(eye), lookAt(lookAt), up(up), fov(fov), x(x), y(y), near(near), far(far)
{
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(eye, lookAt, up);
}

glm::mat4 Camera::getPerspectiveMatrix()
{
	return glm::perspective(glm::radians(fov), x/y, near, far);
}