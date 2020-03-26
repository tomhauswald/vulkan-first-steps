#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_STATIC
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "common.h"
#include "view.h"
#include "vertex.h"
#include "uniform.h"

class VulkanDrawCall;

class VulkanContext {
private:
	template<typename V>
	using PerPhysicalDevice = std::unordered_map<VkPhysicalDevice, V>;

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
	
private:
	VkInstance m_instance;

	std::vector<VkPhysicalDevice> m_physicalDevices;
	VkPhysicalDevice m_physicalDevice;
	PerPhysicalDevice<VkPhysicalDeviceProperties>           m_physicalDeviceProperties;
	PerPhysicalDevice<VkPhysicalDeviceMemoryProperties>     m_physicalDeviceMemoryProperties;
	PerPhysicalDevice<std::vector<VkSurfaceFormatKHR>>      m_physicalDeviceSurfaceFormats;
	PerPhysicalDevice<std::vector<VkExtensionProperties>>   m_physicalDeviceExtensions;
	PerPhysicalDevice<std::vector<VkQueueFamilyProperties>> m_physicalDeviceQueueFamilies;

	VkDevice m_device;

	VkExtent2D m_windowExtent;
	VkSurfaceKHR m_windowSurface;

	std::array<uint32_t, NUM_QUEUE_ROLES> m_queueFamilyIndices;
	std::array<VkQueue, NUM_QUEUE_ROLES>  m_queues;

	VkSwapchainKHR m_swapchain;
	uint32_t m_swapchainImageIndex;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
	std::vector<VkDescriptorSet> m_swapchainDescriptorSets;
	std::vector<VkCommandBuffer> m_swapchainClearCmdbufs;
	std::vector<std::tuple<VkBuffer, VkDeviceMemory>> m_swapchainUniformBuffers;

	std::unordered_map<std::string, VkShaderModule> m_shaders;

	VkVertexInputBindingDescription m_vertexBinding;
	std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;

	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	VkPipeline m_pipeline;
	VkCommandPool m_commandPool;
	VkDescriptorSetLayout m_setLayout;

	VkDescriptorPool m_descriptorPool;
	size_t m_uniformBufferSize;

	std::array<VkSemaphore, NUM_RENDER_EVENTS> m_semaphores;
	
private:
	std::tuple<VkBuffer, VkDeviceMemory> createBuffer(
		VkBufferUsageFlags usageMask,
		VkMemoryPropertyFlags propertyMask,
		size_t bytes
	);

	std::tuple<VkBuffer, VkDeviceMemory> createUniformBuffer(size_t bytes);

	void writeToBuffer(
		VkDeviceMemory& mem,
		void const* data,
		size_t bytes,
		size_t offset = 0
	);

	std::tuple<VkBuffer, VkDeviceMemory> uploadToDevice(
		std::tuple<VkBuffer, VkDeviceMemory>& host,
		VkBufferUsageFlagBits bufferTypeFlag,
		size_t bytes
	);

public:
	~VulkanContext();
	
	void createInstance();
	void selectPhysicalDevice();
	void createDevice();
	void createSwapchain(bool vsync);
	
	std::vector<VkCommandBuffer> recordCommands(
		std::function<void(VkCommandBuffer, size_t)> const& vkCmdLambda
	);

	std::vector<VkCommandBuffer> recordClearCommands(
		glm::vec3 const& color
	);
	
	std::vector<VkCommandBuffer> recordDrawCommands(
		VkBuffer vertexBuffer, 
		size_t vertexCount
	);

	VkShaderModule const& loadShader(std::string const& name);
	void accomodateWindow(GLFWwindow* window);
	void onFrameBegin();
	void execute(std::vector<VulkanDrawCall const*> const& calls);
	void onFrameDone();
	
	inline VkDevice device() { return m_device; }

	void createPipeline(
		std::string const& vertexShaderName,
		std::string const& fragmentShaderName,
		VkVertexInputBindingDescription const& binding,
		std::vector<VkVertexInputAttributeDescription> const& attributes,
		size_t uniformBufferSize
	);

	void updateUniformBuffers(Uniform const& uniform);

	std::tuple<VkBuffer, VkDeviceMemory> createVertexBuffer(
		View<Vertex> const& vertices
	) {
		// Create CPU-accessible buffer.
		auto host = createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
			| VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertices.bytes()
		);

		// Write buffer data.
		writeToBuffer(std::get<1>(host), vertices.items(), vertices.bytes());
		
		// Upload to GPU.
		return uploadToDevice(
			host, 
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			vertices.bytes()
		);
	}
};

// Vulkan resource query with internal return code.
template<typename Resource, typename ... Args>
std::vector<Resource> queryVulkanResources(
	VkResult(*func)(Args..., uint32_t*, Resource*),
	Args... args
) {
	uint32_t count;
	std::vector<Resource> items;
	crashIf(func(args..., &count, nullptr) != VK_SUCCESS);
	items.resize(static_cast<size_t>(count));
	func(args..., &count, items.data());
	return std::move(items);
}

// Vulkan resource query without internal return code.
template<typename Resource, typename ... Args>
std::vector<Resource> queryVulkanResources(
	void(*func)(Args..., uint32_t*, Resource*),
	Args... args
) {
	uint32_t count;
	std::vector<Resource> items;
	func(args..., &count, nullptr);
	items.resize(static_cast<size_t>(count));
	func(args..., &count, items.data());
	return std::move(items);
}
