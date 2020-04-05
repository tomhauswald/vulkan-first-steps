#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_STATIC
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <png.hpp>

#include "common.h"
#include "shader_interface.h"

struct BufferInfo {
	VkBuffer buffer;
	VkDeviceMemory memory;
	size_t sizeInBytes;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memoryProperties;
};

struct TextureInfo {
	static constexpr uint8_t bytesPerPixel = 4; 
	
	VkImage image;
	VkDeviceMemory memory;
	uint32_t width;
	uint32_t height;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags memoryProperties;
};

class VulkanContext {
private:
	template<typename V>
	using PerPhysicalDevice = std::unordered_map<VkPhysicalDevice, V>;

	enum class QueueRole {
		Graphics,
		Presentation
	};

	enum class DeviceEvent {
		SwapchainImageAvailable,
		FrameRenderingDone
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

	std::unordered_map<
		QueueRole, 
		std::tuple<uint32_t, VkQueue>
	> m_queueInfo;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
	std::vector<VkDescriptorSet> m_swapchainDescriptorSets;
	std::vector<VkCommandBuffer> m_swapchainCommandBuffers;
	std::vector<BufferInfo> m_swapchainUniformBuffers;
	std::vector<VkFence> m_swapchainFences;
	uint32_t m_swapchainImageIndex; // <- Index into resource arrays.

	std::tuple<VkImage, VkImageView, VkDeviceMemory> m_depthBuffer;
	
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
	size_t m_pushConstantSize;

	std::vector<std::unordered_map<DeviceEvent, VkSemaphore>> m_semaphores;
	size_t m_semaphoreIndex = 0;
	
private:
	void runDeviceCommands(std::function<void(VkCommandBuffer)> commands);

	BufferInfo createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps, size_t bytes);
	
	inline BufferInfo createHostBuffer(VkBufferUsageFlags usage, size_t bytes) {
		return createBuffer(
			usage, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			bytes
		);
	}

	inline BufferInfo createDeviceBuffer(VkBufferUsageFlags usage, size_t bytes) {
		return createBuffer(usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bytes); 
	}

	inline BufferInfo createUniformBuffer(size_t bytes) {
		return createHostBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bytes);
	}

	BufferInfo uploadToDevice(BufferInfo hostBufferInfo);

	VkDeviceMemory allocateDeviceMemory(
		VkMemoryRequirements const& memReqs,
		VkMemoryPropertyFlags memProps
	);

	void writeDeviceMemory(VkDeviceMemory& mem, void const* data, size_t bytes, size_t offset = 0);

public:
	~VulkanContext();
	
	void createInstance();
	void selectPhysicalDevice();
	void createDevice();
	void createSwapchain(bool vsync);
	void createDepthBuffer();
	
	VkShaderModule const& loadShader(std::string const& name);
	void accomodateWindow(GLFWwindow* window);

	void createPipeline(
		std::string const& vertexShaderName,
		std::string const& fragmentShaderName,
		VkVertexInputBindingDescription const& binding,
		std::vector<VkVertexInputAttributeDescription> const& attributes,
		size_t uniformBufferSize, 
		size_t pushConstantSize
	);

	TextureInfo createTexture(uint32_t width, uint32_t height, uint32_t const* pixels);
	BufferInfo createVertexBuffer(std::vector<Vertex> const& vertices);
	BufferInfo createIndexBuffer(std::vector<uint32_t> const& indices);

	void setUniforms(ShaderUniforms const& uniforms);
	void setPushConstants(ShaderPushConstants const& push);

	void onFrameBegin();
	void draw(VkBuffer vbuf, VkBuffer ibuf, uint32_t count);
	void onFrameEnd();
	
	// Wait for all frames in flight to be delivered.
	inline void flush() { vkDeviceWaitIdle(m_device); }

	inline void destroyBuffer(BufferInfo& info) {
		vkDestroyBuffer(m_device, info.buffer, nullptr);
		vkFreeMemory(m_device, info.memory, nullptr);
		info = {};
	}

	inline void destroyTexture(TextureInfo& info) {
		vkDestroyImage(m_device, info.image, nullptr);
		vkFreeMemory(m_device, info.memory, nullptr);
		info = {};
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
