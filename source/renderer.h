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

struct VulkanContext {

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	std::vector<VkPhysicalDevice> physicalDevices;
	std::unordered_map<VkPhysicalDevice, VkPhysicalDeviceProperties> physicalDeviceProperties;
	std::unordered_map<VkPhysicalDevice, std::vector<VkExtensionProperties>> physicalDeviceExtensions;
	std::unordered_map<VkPhysicalDevice, std::vector<VkQueueFamilyProperties>> physicalDeviceQueueFamilies;
	std::unordered_map<VkPhysicalDevice, std::vector<VkSurfaceFormatKHR>> physicalDeviceSurfaceFormats;

	VkDevice device;

	VkExtent2D windowExtent;
	VkSurfaceKHR windowSurface;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;

	std::array<uint32_t, QueueRole::_Count> queueFamilyIndices;
	std::array<VkQueue, QueueRole::_Count> queues;

	VkSwapchainKHR swapchain;
};

class Renderer {
private:
	GLFWwindow* m_pWindow;
	VulkanContext m_vkCtx;

public:
	Renderer();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene) const;
	~Renderer();
};
