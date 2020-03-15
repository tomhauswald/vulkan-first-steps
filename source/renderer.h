#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_STATIC
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "common.h"

struct Scene { };

enum QueueRole {
	Graphics,
	Presentation,
	_Count
};

class Renderer {
private:
	GLFWwindow* m_pWindow;

	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	std::array<VkQueue, QueueRole::_Count> m_queues;
	VkSurfaceKHR m_surface;
public:
	Renderer();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene) const;
	~Renderer();
};
