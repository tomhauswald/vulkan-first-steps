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

template<typename V>
using PerPhysicalDevice = std::unordered_map<VkPhysicalDevice, V>;

struct VulkanContext {

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	std::vector<VkPhysicalDevice> physicalDevices;
	PerPhysicalDevice<VkPhysicalDeviceProperties> physicalDeviceProperties;
	PerPhysicalDevice<std::vector<VkSurfaceFormatKHR>> physicalDeviceSurfaceFormats;
	PerPhysicalDevice<std::vector<VkExtensionProperties>> physicalDeviceExtensions;
	PerPhysicalDevice<std::vector<VkQueueFamilyProperties>> physicalDeviceQueueFamilies;

	VkDevice device;

	VkExtent2D windowExtent;
	VkSurfaceKHR windowSurface;

	std::array<uint32_t, QueueRole::_Count> queueFamilyIndices;
	std::array<VkQueue, QueueRole::_Count> queues;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
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
