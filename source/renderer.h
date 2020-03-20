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
	QUEUE_ROLE_GRAPHICS,
	QUEUE_ROLE_PRESENTATION,
	NUM_QUEUE_ROLES
};

enum RenderEvent {
	RENDER_EVENT_IMAGE_AVAILABLE,
	RENDER_EVENT_FRAME_DONE,
	NUM_RENDER_EVENTS
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

	std::array<uint32_t, NUM_QUEUE_ROLES> queueFamilyIndices;
	std::array<VkQueue,  NUM_QUEUE_ROLES> queues;

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	std::vector<VkCommandBuffer> swapchainCommandBuffers;

	std::unordered_map<std::string, VkShaderModule> shaders;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline pipeline;
	VkCommandPool commandPool;
	std::array<VkSemaphore, NUM_RENDER_EVENTS> semaphores;
};

class Renderer {
private:
	GLFWwindow* m_pWindow;
	VulkanContext m_vulkanContext;

public:
	Renderer();
	bool isWindowOpen() const;
	void handleWindowEvents();
	void renderScene(Scene const& scene) const;
	~Renderer();
};
