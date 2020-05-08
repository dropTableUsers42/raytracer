#ifndef _CAMERA_H
#define _CAMERA_H

#include <glad/glad.h>
#include "glm/glm/vec3.hpp"
#include "glm/glm/mat4x4.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"

class Camera
{
public:
	glm::vec3 eye, lookAt, up;
	GLfloat fov, x, y, near, far;

	Camera(glm::vec3 eye, glm::vec3 lookAt, glm::vec3 up, GLfloat fov, GLfloat x, GLfloat y, GLfloat near, GLfloat far);
	glm::mat4 getViewMatrix();
	glm::mat4 getPerspectiveMatrix();
};

#endif