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

struct VulkanBufferInfo {
	
	size_t sizeInBytes;
	
	VkBuffer buffer;
	VkBufferUsageFlags usage;
	
	VkDeviceMemory memory;
	VkMemoryPropertyFlags memoryProperties;
};

struct VulkanTextureInfo {

	static constexpr uint8_t bytesPerPixel = 4; 
	static constexpr uint8_t numSlots = 4;

	uint32_t width;
	uint32_t height;

	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;

	std::array<VkDescriptorSet, numSlots> samplerSlotDescriptorSets;
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

	std::vector<std::unordered_map<DeviceEvent, VkSemaphore>> m_semaphores;
	size_t m_semaphoreIndex = 0;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;
	std::vector<VkDescriptorSet> m_swapchainUniformDescriptorSets;
	std::vector<VkCommandBuffer> m_swapchainCommandBuffers;
	std::vector<VulkanBufferInfo> m_swapchainUniformBuffers;
	std::vector<uint32_t> m_swapchainUniformBufferOffsets;

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

	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_uniformDescriptorSetLayout;
	VkDescriptorSetLayout m_samplerDescriptorSetLayout;
	VkSampler m_sampler;
	
	std::array<
		VulkanTextureInfo const*, 
		VulkanTextureInfo::numSlots
	> m_boundTextures;

	uint32_t m_uniformBufferSize;
	uint32_t m_pushConstantSize;
	
private:
	void runDeviceCommands(std::function<void(VkCommandBuffer)> commands);

	VulkanBufferInfo createBuffer(
		VkBufferUsageFlags usage, 
		VkMemoryPropertyFlags memProps, 
		VkDeviceSize bytes
	);
	
	inline VulkanBufferInfo createHostBuffer(VkBufferUsageFlags usage, VkDeviceSize bytes) {
		return createBuffer(
			usage, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			bytes
		);
	}

	inline VulkanBufferInfo createDeviceBuffer(VkBufferUsageFlags usage, VkDeviceSize bytes) {
		return createBuffer(usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bytes); 
	}

	inline VulkanBufferInfo createUniformBuffer(uint32_t bytes) {
		return createHostBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, static_cast<VkDeviceSize>(bytes));
	}

	VulkanBufferInfo uploadToDevice(VulkanBufferInfo hostBufferInfo);

	VkDeviceMemory allocateDeviceMemory(
		VkMemoryRequirements const& memReqs,
		VkMemoryPropertyFlags memProps
	);

	inline void writeDeviceMemory(
		VkDeviceMemory& mem,
		void const* data,
		VkDeviceSize bytes,
		VkDeviceSize offset = 0
	) {
		void* raw;
		crashIf(VK_SUCCESS != vkMapMemory(m_device, mem, offset, bytes, 0, &raw));
		std::memcpy(raw, data, bytes);
		vkUnmapMemory(m_device, mem);
	}

	inline void clearUniformData() {
		m_swapchainUniformBufferOffsets[m_swapchainImageIndex] = 0;
		bindUniformData(0);
	}

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
		uint32_t uniformBufferSize,
		uint32_t pushConstantSize
	);

	VulkanTextureInfo createTexture(uint32_t width, uint32_t height, uint32_t const* pixels);
	VulkanBufferInfo createVertexBuffer(std::vector<VPositionColorTexcoord> const& vertices);
	VulkanBufferInfo createIndexBuffer(std::vector<uint32_t> const& indices);

	inline void bindUniformData(uint32_t bytes) {
		auto offset = m_swapchainUniformBufferOffsets[m_swapchainImageIndex] - bytes;
		vkCmdBindDescriptorSets(
			m_swapchainCommandBuffers[m_swapchainImageIndex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			0,
			1,
			&m_swapchainUniformDescriptorSets[m_swapchainImageIndex],
			1,
			&offset
		);
	}

	void appendUniformData(void const* data, uint32_t bytes);
	void setPushConstantData(void const* data, uint32_t bytes);

	void bindTextureSlot(uint8_t slot, VulkanTextureInfo const& txr);

	void onFrameBegin();
	void draw(VkBuffer vbuf, VkBuffer ibuf, uint32_t count);
	void onFrameEnd();
	
	// Wait for all frames in flight to be delivered.
	inline void flush() { 
		vkDeviceWaitIdle(m_device); 
	}

	inline void destroyBuffer(VulkanBufferInfo& info) {
		vkDestroyBuffer(m_device, info.buffer, nullptr);
		vkFreeMemory(m_device, info.memory, nullptr);
		info = {};
	}

	inline void destroyTexture(VulkanTextureInfo& info) {
		vkDestroyImageView(m_device, info.view, nullptr);
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
