#pragma once

#include <glm/glm.hpp>

namespace Uniform {

struct Global {
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

struct PerInstance {
	glm::mat4 modelMatrix;
};

};
