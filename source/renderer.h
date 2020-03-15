#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_STATIC
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "common.h"

struct Scene { };

class Renderer {
private:
	GLFWwindow* m_pWindow;

	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkQueue m_deviceQueue;

public:
	Renderer();
	void createWindow(uint16_t resX, uint16_t resY);;
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene) const;
	~Renderer();
};
