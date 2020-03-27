#pragma once

#include <glm/glm.hpp>

struct UniformData {
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};

struct PushConstantData {
	glm::mat4 modelMatrix;
};
