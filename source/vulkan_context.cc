#include "vulkan_context.h"
#include "vertex_formats.h"
#include "vulkan_draw_call.h"

#include <fstream>

constexpr std::array requiredDeviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr std::array validationLayers{
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_LUNARG_monitor"
};

void VulkanContext::createInstance() {

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Specify validation layers to enable in debug mode.
	createInfo.enabledLayerCount = debug ? validationLayers.size() : 0;
	createInfo.ppEnabledLayerNames = debug ? validationLayers.data() : nullptr;

	// Add the extensions required by GLFW.
	uint32_t extensionCount;
	auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

	crashIf(vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS);
}

void VulkanContext::selectPhysicalDevice() {

	m_physicalDevices = queryVulkanResources<VkPhysicalDevice, VkInstance>(
		&vkEnumeratePhysicalDevices,
		m_instance
	);

	m_physicalDevice = nullptr;
	for (auto const& candidate : m_physicalDevices) {

		vkGetPhysicalDeviceProperties(candidate, &m_physicalDeviceProperties[candidate]);
		auto const& props = m_physicalDeviceProperties[candidate];

		vkGetPhysicalDeviceMemoryProperties(candidate, &m_physicalDeviceMemoryProperties[candidate]);

		m_physicalDeviceExtensions[candidate] = queryVulkanResources<
			VkExtensionProperties,
			VkPhysicalDevice,
			char const*
		>(
			&vkEnumerateDeviceExtensionProperties,
			candidate,
			nullptr
		);

		m_physicalDeviceQueueFamilies[candidate] = queryVulkanResources<
			VkQueueFamilyProperties,
			VkPhysicalDevice
		>(
			&vkGetPhysicalDeviceQueueFamilyProperties,
			candidate
		);

		auto hasExtensions = std::all_of(
			std::begin(requiredDeviceExtensions),
			std::end(requiredDeviceExtensions),
			[&](char const* req) {
				return contains(
					m_physicalDeviceExtensions[candidate],
					[&](VkExtensionProperties const& ext) {
						return (0 == strcmp(req, ext.extensionName));
					}
				);
			}
		);

		size_t graphicsFamilyIndex;
		auto hasGraphics = contains(
			m_physicalDeviceQueueFamilies[candidate],
			[](auto fam) {
				return fam.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			},
			&graphicsFamilyIndex
		);

		size_t presentationFamilyIndex;
		auto hasPresentation = contains(
			range(m_physicalDeviceQueueFamilies[candidate].size()),
			[&](auto famIdx) {
				VkBool32 supported;
				crashIf(VK_SUCCESS != vkGetPhysicalDeviceSurfaceSupportKHR(
					candidate,
					famIdx,
					m_windowSurface,
					&supported
				));
				return supported;
			},
			&presentationFamilyIndex
		);

		m_physicalDeviceSurfaceFormats[candidate] = queryVulkanResources<
			VkSurfaceFormatKHR,
			VkPhysicalDevice,
			VkSurfaceKHR
		>(
			&vkGetPhysicalDeviceSurfaceFormatsKHR,
			candidate,
			m_windowSurface
		);

		size_t bgraFormatIndex;
		auto hasBGRA = contains(
			m_physicalDeviceSurfaceFormats[candidate],
			[](VkSurfaceFormatKHR const& fmt) {
				return fmt.format == VK_FORMAT_B8G8R8A8_SRGB
					&& fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			},
			&bgraFormatIndex
		);

		std::cout
			<< props.deviceName << " "
			<< ((hasGraphics) ? ("[Graphics]") : ("")) << " "
			<< ((hasPresentation) ? ("[Presentation]") : ("")) << " "
			<< ((props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				? "[Discrete]" : "[Integrated]") << lf;

		// Skip unsuitable devices.
		if (!hasExtensions ||
			!hasGraphics ||
			!hasPresentation ||
			!hasBGRA
		) continue;

		bool isUpgrade = false;

		// Choose any suitable device first.
		if (!m_physicalDevice) {
			isUpgrade = true;
		}

		else {
			auto const& prevProps = m_physicalDeviceProperties[m_physicalDevice];

			// Choose discrete over integrated.
			if (
				prevProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			) {
				isUpgrade = true;
			}

			// Choose GPU of same type with highest image quality.
			else if (
				prevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				props.limits.maxImageDimension1D > prevProps.limits.maxImageDimension1D
			) {
				isUpgrade = true;
			}
		}

		if (isUpgrade) {
			m_physicalDevice = candidate;
			m_queueFamilyIndices[QUEUE_ROLE_GRAPHICS] = graphicsFamilyIndex;
			m_queueFamilyIndices[QUEUE_ROLE_PRESENTATION] = presentationFamilyIndex;
		}
	}

	crashIf(!m_physicalDevice);
}

void VulkanContext::createDevice() {

	std::set<uint32_t> uniqueIndices{
		m_queueFamilyIndices.begin(),
		m_queueFamilyIndices.end()
	};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (auto index : uniqueIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		std::array prios{ 1.0f };
		queueCreateInfo.pQueuePriorities = prios.data();
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	VkPhysicalDeviceFeatures features{};
	createInfo.pEnabledFeatures = &features;
	createInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

	crashIf(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS);

	for (auto role : range<QueueRole>(NUM_QUEUE_ROLES)) {
		vkGetDeviceQueue(m_device, m_queueFamilyIndices[role], 0, &m_queues[role]);
	}

	auto poolInfo = VkCommandPoolCreateInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_queueFamilyIndices[QUEUE_ROLE_GRAPHICS];

	crashIf(VK_SUCCESS != vkCreateCommandPool(
		m_device,
		&poolInfo,
		nullptr,
		&m_commandPool
	));
}

void VulkanContext::createSwapchain(bool vsync) {

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	crashIf(VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		m_physicalDevice,
		m_windowSurface,
		&surfaceCapabilities
	));

	auto maxImages = (
		(surfaceCapabilities.maxImageCount != 0)
		? (surfaceCapabilities.maxImageCount)
		: (UINT32_MAX)
	);

	auto swapchainInfo = VkSwapchainCreateInfoKHR{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = m_windowSurface;
	swapchainInfo.minImageCount = std::min(surfaceCapabilities.minImageCount + 1, maxImages);
	swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainInfo.imageExtent = m_windowExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.clipped = VK_TRUE;

	swapchainInfo.presentMode = (
		(vsync)
		? (VK_PRESENT_MODE_FIFO_KHR)
		: (VK_PRESENT_MODE_MAILBOX_KHR)
	);

	// Are graphics and presentation capabilities provided by the same queue family.
	auto sameQFam = (uniqueElemCount(m_queueFamilyIndices) == 1);
	
	swapchainInfo.imageSharingMode = (
		(sameQFam) 
		? (VK_SHARING_MODE_EXCLUSIVE)
		: (VK_SHARING_MODE_CONCURRENT)
	);
	
	if (sameQFam) {
		swapchainInfo.queueFamilyIndexCount = NUM_QUEUE_ROLES;
		swapchainInfo.pQueueFamilyIndices = m_queueFamilyIndices.data();
	}

	crashIf(VK_SUCCESS != vkCreateSwapchainKHR(
		m_device, 
		&swapchainInfo, 
		nullptr, 
		&m_swapchain
	));

	m_swapchainImages = queryVulkanResources<VkImage, VkDevice, VkSwapchainKHR>(
		&vkGetSwapchainImagesKHR,
		m_device,
		m_swapchain
	);

	m_swapchainImageViews = mapToVector<decltype(m_swapchainImages), VkImageView>(
		m_swapchainImages,
		[&](auto image) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = VK_FORMAT_B8G8R8A8_SRGB;

			auto& srr = createInfo.subresourceRange;
			srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			srr.levelCount = 1;
			srr.layerCount = 1;

			VkImageView view;
			crashIf(vkCreateImageView(m_device, &createInfo, nullptr, &view) != VK_SUCCESS);
			return std::move(view);
		}
	);
}

VkShaderModule const& VulkanContext::loadShader(std::string const& name) {

	constexpr auto shaderBasePath = "../assets/shaders/spirv/";
	constexpr auto shaderExt = ".spv";

	auto path = shaderBasePath + name + shaderExt;
	std::cout << "Loading shader from path: " << path << lf;

	std::ifstream fs(path, std::ios::binary);
	crashIf(!fs.is_open());

	std::stringstream ss;
	ss << fs.rdbuf();
	auto code = ss.str();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<uint32_t const*>(code.c_str());

	crashIf(VK_SUCCESS != vkCreateShaderModule(
		m_device, 
		&createInfo, 
		nullptr, 
		&m_shaders[name]
	));

	return m_shaders.at(name);
}

void VulkanContext::accomodateWindow(GLFWwindow* window) {
	
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	m_windowExtent = { 
		static_cast<uint32_t>(w), 
		static_cast<uint32_t>(h) 
	};

	// Window surface creation must succeed.
	crashIf(glfwCreateWindowSurface(m_instance, window, nullptr, &m_windowSurface) != VK_SUCCESS);
}

std::vector<VkCommandBuffer> VulkanContext::recordCommands(
	std::function<void(VkCommandBuffer)> const& vkCmdLambda
) {
	auto allocateInfo = VkCommandBufferAllocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.commandBufferCount = m_swapchainImages.size();

	auto swapchainImageCount = m_swapchainImages.size();

	auto cmdbufs = std::vector<VkCommandBuffer>(swapchainImageCount);
	crashIf(vkAllocateCommandBuffers(m_device, &allocateInfo, cmdbufs.data()) != VK_SUCCESS);

	for (size_t i = 0; i < swapchainImageCount; ++i) {

		auto cmdbufBeginInfo = VkCommandBufferBeginInfo{};
		cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		crashIf(vkBeginCommandBuffer(cmdbufs[i], &cmdbufBeginInfo) != VK_SUCCESS);
		{
			auto clearValue = VkClearValue{};
			auto passBeginInfo = VkRenderPassBeginInfo{};
			passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			passBeginInfo.framebuffer = m_swapchainFramebuffers[i];
			passBeginInfo.clearValueCount = 1;
			passBeginInfo.pClearValues = &clearValue;
			passBeginInfo.renderPass = m_renderPass;
			passBeginInfo.renderArea.extent = m_windowExtent;

			vkCmdBeginRenderPass(cmdbufs[i], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				vkCmdBindPipeline(cmdbufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
				vkCmdLambda(cmdbufs[i]);
			}
			vkCmdEndRenderPass(cmdbufs[i]);
		}
		crashIf(vkEndCommandBuffer(cmdbufs[i]) != VK_SUCCESS);
	}

	return cmdbufs;
}

std::vector<VkCommandBuffer> VulkanContext::recordClearCommands(glm::vec3 const& color) {
	
	auto colorAttachment = VkClearAttachment{};
	colorAttachment.colorAttachment = 0;
	colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorAttachment.clearValue.color = { .float32 = { color.r, color.g, color.b, 0.0f } };

	auto clearRect = VkClearRect{};
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.extent = m_windowExtent;

	return recordCommands([&](VkCommandBuffer cmdbuf) {
		vkCmdClearAttachments(
			cmdbuf,
			1,
			&colorAttachment,
			1,
			&clearRect
		);
	});
}

std::vector<VkCommandBuffer> VulkanContext::recordDrawCommands(
	VkBuffer vertexBuffer, 
	size_t vertexCount
) {
	return recordCommands([&](VkCommandBuffer cmdbuf) {
		auto const offsetZero = VkDeviceSize{};
		vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vertexBuffer, &offsetZero);
		vkCmdDraw(cmdbuf, static_cast<uint32_t>(vertexCount), 1, 0, 0);
	});
}

void VulkanContext::beginFrame() {

	vkAcquireNextImageKHR(
		m_device,
		m_swapchain,
		UINT64_MAX,
		m_semaphores[RENDER_EVENT_IMAGE_AVAILABLE],
		VK_NULL_HANDLE,
		&m_swapchainImageIndex
	);
}

void VulkanContext::endFrame() {

	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphores[RENDER_EVENT_FRAME_DONE];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_swapchainImageIndex;

	crashIf(VK_SUCCESS != vkQueuePresentKHR(
		m_queues[QUEUE_ROLE_PRESENTATION],
		&presentInfo
	));

	crashIf(VK_SUCCESS != vkQueueWaitIdle(m_queues[QUEUE_ROLE_PRESENTATION]));
}

std::tuple<VkBuffer, VkDeviceMemory> VulkanContext::createBuffer(
	VkBufferUsageFlags usageMask,
	VkMemoryPropertyFlags propertyMask,
	size_t bytes
) {
	auto bufferInfo = VkBufferCreateInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &m_queueFamilyIndices[QUEUE_ROLE_GRAPHICS];
	bufferInfo.usage = usageMask;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = static_cast<VkDeviceSize>(bytes);

	auto buf = VkBuffer{};
	vkCreateBuffer(m_device, &bufferInfo, nullptr, &buf);

	auto memReqmt = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(m_device, buf, &memReqmt);

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqmt.size;
	allocInfo.memoryTypeIndex = -1;

	// Find suitable memory type.
	auto const& devMemProps = m_physicalDeviceMemoryProperties.at(m_physicalDevice);
	for (uint32_t i = 0; i < devMemProps.memoryTypeCount; ++i) {

		if (!nthBitHi(memReqmt.memoryTypeBits, i)) continue; // Skip types according to mask.

		if (satisfiesBitMask(
			devMemProps.memoryTypes[i].propertyFlags,
			propertyMask
		)) {
			allocInfo.memoryTypeIndex = i;
			break;
		}
	}

	auto mem = VkDeviceMemory{};
	crashIf(vkAllocateMemory(m_device, &allocInfo, nullptr, &mem) != VK_SUCCESS);
	crashIf(vkBindBufferMemory(m_device, buf, mem, 0) != VK_SUCCESS);

	return { buf, mem };
}

void VulkanContext::createPipeline(
	std::string const& vertexShaderName, 
	std::string const& fragmentShaderName, 
	VkVertexInputBindingDescription const& binding, 
	std::vector<VkVertexInputAttributeDescription> const& attributes
) {
	auto colorAttachments = std::array{
		VkAttachmentDescription{
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
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

	crashIf(VK_SUCCESS != vkCreateRenderPass(
		m_device,
		&renderPassInfo,
		nullptr,
		&m_renderPass
	));

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
			crashIf(VK_SUCCESS != vkCreateFramebuffer(
				m_device,
				&framebufferInfo,
				nullptr,
				&framebuffer
			));
			return framebuffer;
		}
	);

	auto vertexInput = VkPipelineVertexInputStateCreateInfo{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexAttributeDescriptionCount = attributes.size();
	vertexInput.pVertexAttributeDescriptions = attributes.data();
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;

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

	m_clearCommandBuffers = recordClearCommands({ 1.0f, 0.0f, 0.0f });

	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	crashIf(vkCreateSemaphore(
		m_device,
		&semaphoreInfo,
		nullptr,
		&m_semaphores[RENDER_EVENT_IMAGE_AVAILABLE]
	) != VK_SUCCESS);

	crashIf(vkCreateSemaphore(
		m_device,
		&semaphoreInfo,
		nullptr,
		&m_semaphores[RENDER_EVENT_FRAME_DONE]
	) != VK_SUCCESS);
}

void VulkanContext::execute(std::vector<VulkanDrawCall const*> const& calls) {

	auto cmdbufs = std::vector<VkCommandBuffer>(calls.size() + 1);
	cmdbufs[0] = m_clearCommandBuffers[m_swapchainImageIndex];
	for (size_t i = 0; i < calls.size(); ++i) {
		cmdbufs[i + 1] = calls[i]->swapchainCommandBuffer(m_swapchainImageIndex);
	}

	auto stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = cmdbufs.size();
	submitInfo.pCommandBuffers = cmdbufs.data();
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &stage;
	submitInfo.pWaitSemaphores = &m_semaphores[RENDER_EVENT_IMAGE_AVAILABLE];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphores[RENDER_EVENT_FRAME_DONE];

	crashIf(VK_SUCCESS != vkQueueSubmit(
		m_queues[QUEUE_ROLE_GRAPHICS],
		1,
		&submitInfo,
		VK_NULL_HANDLE
	));
}

VulkanContext::~VulkanContext() {

	for (auto semaphore : m_semaphores) {
		vkDestroySemaphore(m_device, semaphore, nullptr);
	}

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	for (auto framebuffer : m_swapchainFramebuffers) {
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	for (auto [name, shader] : m_shaders) {
		vkDestroyShaderModule(m_device, shader, nullptr);
	}

	for (auto view : m_swapchainImageViews) {
		vkDestroyImageView(m_device, view, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);

	vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
