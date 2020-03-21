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

	std::array<VkSemaphore, NUM_RENDER_EVENTS> m_semaphores;
	
public:
	~VulkanContext();
	
	void createInstance();
	void selectPhysicalDevice();
	void createDevice();
	void createSwapchain();
	
	std::vector<VkCommandBuffer> createCommandBuffers(
		VkBuffer vertexBuffer, 
		size_t vertexCount
	);

	VkShaderModule const& loadShader(std::string const& name);
	void accomodateWindow(GLFWwindow* window);
	void beginFrame();
	void execute(VulkanDrawCall const& drawCall);
	void endFrame();
	
	inline VkDevice device() { return m_device; }

	template<typename Vertex>
	void createPipeline(
		std::string const& vertexShaderName, 
		std::string const& fragmentShaderName
	) {
		auto colorAttachments = std::array{
			VkAttachmentDescription{
				.format = VK_FORMAT_B8G8R8A8_SRGB,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			}
		};

		auto colorAttachmentReferences = std::array{
			VkAttachmentReference{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}
		};

		auto subpasses = std::array{
			VkSubpassDescription{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = colorAttachmentReferences.size(),
				.pColorAttachments = colorAttachmentReferences.data(),
			}
		};

		auto dependency = VkSubpassDependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		auto renderPassInfo = VkRenderPassCreateInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = colorAttachments.size();
		renderPassInfo.pAttachments = colorAttachments.data();
		renderPassInfo.subpassCount = subpasses.size();
		renderPassInfo.pSubpasses = subpasses.data();
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		crashIf(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS);

		m_swapchainFramebuffers = mapToVector<
			decltype(m_swapchainImageViews),
			VkFramebuffer
		>(
			m_swapchainImageViews,
			[&](auto const& view) {
				auto framebufferInfo = VkFramebufferCreateInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.width = m_windowExtent.width;
				framebufferInfo.height = m_windowExtent.height;
				framebufferInfo.layers = 1;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = &view;
				framebufferInfo.renderPass = m_renderPass;

				VkFramebuffer framebuffer;
				crashIf(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS);
				return framebuffer;
			}
		);

		auto vertexInput = VkPipelineVertexInputStateCreateInfo{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexAttributeDescriptionCount = std::size(Vertex::attributes());
		vertexInput.pVertexAttributeDescriptions = std::data(Vertex::attributes());
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = std::array{ Vertex::binding() }.data();

		auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_windowExtent.width);
		viewport.height = static_cast<float>(m_windowExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.extent = m_windowExtent;

		auto viewportState = VkPipelineViewportStateCreateInfo{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		auto rasterState = VkPipelineRasterizationStateCreateInfo{};
		rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterState.lineWidth = 1.0f;
		rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterState.polygonMode = VK_POLYGON_MODE_FILL;

		auto blendAttachmentState = VkPipelineColorBlendAttachmentState{};
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = 0b1111;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

		auto blendState = VkPipelineColorBlendStateCreateInfo{};
		blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendState.attachmentCount = 1;
		blendState.pAttachments = &blendAttachmentState;

		auto msaaState = VkPipelineMultisampleStateCreateInfo{};
		msaaState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msaaState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		auto stages = std::array{
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = m_shaders[vertexShaderName],
				.pName = "main",
			},
			VkPipelineShaderStageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = m_shaders[fragmentShaderName],
				.pName = "main",
			}
		};

		auto createInfo = VkPipelineLayoutCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		crashIf(vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS);

		auto pipelineInfo = VkGraphicsPipelineCreateInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.stageCount = stages.size();
		pipelineInfo.pStages = stages.data();
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterState;
		pipelineInfo.pColorBlendState = &blendState;
		pipelineInfo.pMultisampleState = &msaaState;

		crashIf(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS);
	}

	template<typename Vertex>
	VkBuffer createVertexBuffer(View<Vertex> const& vertices, VkDeviceMemory& outMemory) {

		auto vertexBufferInfo = VkBufferCreateInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.queueFamilyIndexCount = 1;
		vertexBufferInfo.pQueueFamilyIndices = &m_queueFamilyIndices[QUEUE_ROLE_GRAPHICS];
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vertexBufferInfo.size = static_cast<VkDeviceSize>(vertices.bytes());

		auto vertexBuffer = VkBuffer{};
		vkCreateBuffer(m_device, &vertexBufferInfo, nullptr, &vertexBuffer);

		auto memReqmt = VkMemoryRequirements{};
		vkGetBufferMemoryRequirements(m_device, vertexBuffer, &memReqmt);

		auto memReqProps = VkMemoryPropertyFlags{
			  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		auto allocInfo = VkMemoryAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqmt.size;
		allocInfo.memoryTypeIndex = -1;

		// Find suitable memory type.
		auto const& devMemProps = m_physicalDeviceMemoryProperties.at(m_physicalDevice);
		for (uint32_t i = 0; i < devMemProps.memoryTypeCount; ++i) {

			if (!nthBitHi(memReqmt.memoryTypeBits, i)) continue; // Skip types according to mask.

			else if (devMemProps.memoryTypes[i].propertyFlags & memReqProps) {
				allocInfo.memoryTypeIndex = i;
				break;
			}
		}

		crashIf(vkAllocateMemory(m_device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS);
		crashIf(vkBindBufferMemory(m_device, vertexBuffer, outMemory, 0) != VK_SUCCESS);

		void* raw;
		crashIf(vkMapMemory(m_device, outMemory, 0, vertexBufferInfo.size, 0, &raw) != VK_SUCCESS);
		{
			crashIf(memcpy_s(raw, vertexBufferInfo.size, vertices.items(), vertices.bytes()) != 0);
		}
		vkUnmapMemory(m_device, outMemory);

		return vertexBuffer;
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