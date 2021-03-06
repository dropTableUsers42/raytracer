#ifndef _MATERIAL_H
#define _MATERIAL_H

#include <glm/glm/vec3.hpp>

/**
 * Simple material class
 */
class Material
{
public:
	glm::vec3 Kd, Ke;
	float Ns;
	int emitter = false;
};

#endif