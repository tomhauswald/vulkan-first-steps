#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_STATIC
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "common.h"
#include "view.h"

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

	std::vector<VkImage>         m_swapchainImages;
	std::vector<VkImageView>     m_swapchainImageViews;
	std::vector<VkFramebuffer>   m_swapchainFramebuffers;
	uint32_t m_swapchainImageIndex;

	std::unordered_map<std::string, VkShaderModule> m_shaders;

	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	VkPipeline m_pipeline;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_clearCommandBuffers;

	std::array<VkSemaphore, NUM_RENDER_EVENTS> m_semaphores;
	
	std::tuple<VkBuffer, VkDeviceMemory> createBuffer(
		VkBufferUsageFlags usageMask,
		VkMemoryPropertyFlags propertyMask,
		size_t bytes
	);

	void createPipeline(
		std::string const& vertexShaderName,
		std::string const& fragmentShaderName,
		VkVertexInputBindingDescription const& binding,
		View<VkVertexInputAttributeDescription> const& attributes
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
	void beginFrame();
	void execute(std::vector<VulkanDrawCall const*> const& calls);
	void endFrame();
	
	inline VkDevice device() { return m_device; }

	template<typename Vertex>
	void createPipeline(
		std::string const& vertexShaderName, 
		std::string const& fragmentShaderName
	) {
		createPipeline(
			vertexShaderName, 
			fragmentShaderName, 
			Vertex::binding(), 
			makeContainerView(Vertex::attributes())
		);
	}

	template<typename Vertex>
	std::tuple<VkBuffer, VkDeviceMemory> createVertexBuffer(
		View<Vertex> const& vertices
	) {
		// Create CPU-accessible buffer.
		auto [cpuBuf, cpuMem] = createBuffer(
			VkBufferUsageFlags{ 
				  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			},
			VkMemoryPropertyFlags{
				  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
				| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			},
			vertices.bytes()
		);

		// Map cpu-accessible buffer into RAM.
		void* raw;
		crashIf(VK_SUCCESS != vkMapMemory(
			m_device,
			cpuMem,
			0,
			vertices.bytes(),
			0,
			&raw
		));

		// Fill memory-mapped buffer.
		std::memcpy(raw, vertices.items(), vertices.bytes());
		vkUnmapMemory(m_device, cpuMem);

		// Create GPU-local buffer.
		auto [gpuBuf, gpuMem] = createBuffer(
			VkBufferUsageFlags{
				  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT
			},
			VkMemoryPropertyFlags{
				  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			},
			vertices.bytes()
		);

		// Schedule transfer from CPU to GPU.

		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer xferCmd;
		crashIf(VK_SUCCESS != vkAllocateCommandBuffers(m_device, &allocInfo, &xferCmd));

		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		
		crashIf(VK_SUCCESS != vkBeginCommandBuffer(xferCmd, &beginInfo));
		{
			auto region = VkBufferCopy{};
			region.size = vertices.bytes();
			vkCmdCopyBuffer(xferCmd, cpuBuf, gpuBuf, 1, &region);
		}
		crashIf(VK_SUCCESS != vkEndCommandBuffer(xferCmd));

		// Perform transfer command.

		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &xferCmd;
		
		crashIf(VK_SUCCESS != vkQueueSubmit(
			m_queues[QUEUE_ROLE_GRAPHICS],
			1,
			&submitInfo,
			VK_NULL_HANDLE
		));

		// Wait for completion.
		crashIf(VK_SUCCESS != vkQueueWaitIdle(m_queues[QUEUE_ROLE_GRAPHICS]));

		// Release CPU-accessible resources.
		vkDestroyBuffer(m_device, cpuBuf, nullptr);
		vkFreeMemory(m_device, cpuMem, nullptr);

		return { gpuBuf, gpuMem };
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
