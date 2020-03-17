#include "renderer.h"
#include <fstream>
#include <sstream>

// Vulkan resource query with internal return code.
template<typename Resource, typename ... Args>
std::vector<Resource> queryVulkanResources(
	VkResult(*func)(Args..., uint32_t*, Resource*),
	Args... args
) {
	uint32_t count;
	std::vector<Resource> items;
	crash_if(func(args..., &count, nullptr) != VK_SUCCESS);
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

constexpr std::array requiredDeviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

constexpr std::array validationLayers{
	"VK_LAYER_LUNARG_standard_validation"
};

void createInstance(VulkanContext& ctx) {
	
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Specify validation layers to enable in debug mode.
	createInfo.enabledLayerCount   = debug ? validationLayers.size() : 0;
	createInfo.ppEnabledLayerNames = debug ? validationLayers.data() : nullptr;

	// Add the extensions required by GLFW.
	uint32_t extensionCount;
	auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

	crash_if(vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS);
}

GLFWwindow* createWindow(VulkanContext& ctx, uint32_t resX, uint32_t resY) {

	// Create GLFW window.
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API , GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE  , GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE    , GLFW_FALSE);

	ctx.windowExtent = { resX, resY };
	auto window = glfwCreateWindow(
		static_cast<int>(ctx.windowExtent.width),
		static_cast<int>(ctx.windowExtent.height),
		"Hello, Vulkan!",
		nullptr,
		nullptr
	);

	// Window creation must succeed.
	crash_if(!window);

	// Window surface creation must succeed.
	crash_if(glfwCreateWindowSurface(ctx.instance, window, nullptr, &ctx.windowSurface) != VK_SUCCESS);

	return window;
}

void selectPhysicalDevice(VulkanContext& ctx) {

	ctx.physicalDevices = queryVulkanResources<VkPhysicalDevice, VkInstance>(
		&vkEnumeratePhysicalDevices,
		ctx.instance
	);

	ctx.physicalDevice = nullptr;
	for(auto const& candidate : ctx.physicalDevices) {

		vkGetPhysicalDeviceProperties(candidate, &ctx.physicalDeviceProperties[candidate]);
		auto const& props = ctx.physicalDeviceProperties[candidate];

		ctx.physicalDeviceExtensions[candidate] = queryVulkanResources<
			VkExtensionProperties, 
			VkPhysicalDevice, 
			char const*
		>(
			&vkEnumerateDeviceExtensionProperties,
			candidate,
			nullptr
		);

		ctx.physicalDeviceQueueFamilies[candidate] = queryVulkanResources<
			VkQueueFamilyProperties, 
			VkPhysicalDevice
		>(
			&vkGetPhysicalDeviceQueueFamilyProperties, 
			candidate
		);

		auto hasExtensions = std::all_of(
			std::begin(requiredDeviceExtensions), 
			std::end(requiredDeviceExtensions),
			[&] (auto const& req) {
				return contains(
					ctx.physicalDeviceExtensions[candidate], 
					[&] (auto const& ext) { 
						return !strcmp(req, ext.extensionName); 
					}
				);
			}
		);
		
		size_t graphicsFamilyIndex;
		auto hasGraphics = contains(
			ctx.physicalDeviceQueueFamilies[candidate], 
			[] (auto fam) { 
				return fam.queueFlags & VK_QUEUE_GRAPHICS_BIT; 
			}, 
			&graphicsFamilyIndex
		);

		size_t presentationFamilyIndex;
		auto hasPresentation = contains(
			range(ctx.physicalDeviceQueueFamilies[candidate].size()), 
			[&] (auto famIdx) {
				VkBool32 supported;
				vkGetPhysicalDeviceSurfaceSupportKHR(candidate, famIdx, ctx.windowSurface, &supported);
				return supported;
			}, 
			&presentationFamilyIndex
		);

		ctx.physicalDeviceSurfaceFormats[candidate] = queryVulkanResources<
			VkSurfaceFormatKHR, 
			VkPhysicalDevice, 
			VkSurfaceKHR
		>(
			&vkGetPhysicalDeviceSurfaceFormatsKHR,
			candidate,
			ctx.windowSurface
		);

		size_t bgraFormatIndex;
		auto hasBGRA = contains(
			ctx.physicalDeviceSurfaceFormats[candidate], 
			[] (VkSurfaceFormatKHR const& fmt) { 
				return fmt.format == VK_FORMAT_B8G8R8A8_SRGB 
				&& fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			}, 
			&bgraFormatIndex
		);

		std::cout
			<< props.deviceName << " "
			<< ((hasGraphics)     ? ("[Graphics]")     : ("")) << " "
			<< ((hasPresentation) ? ("[Presentation]") : ("")) << " "
			<< ((props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				? "[Discrete]" : "[Integrated]") << lf;
		
		// Skip unsuitable devices.
		if( !hasExtensions    ||
			!hasGraphics      || 
		    !hasPresentation  || 
			!hasBGRA
		) continue;
	
		bool isUpgrade = false;

		// Choose any suitable device first.
		if(!ctx.physicalDevice) {
			isUpgrade = true;
		}

		else {
			auto const& prevProps = ctx.physicalDeviceProperties[ctx.physicalDevice];

			// Choose discrete over integrated.
			if(
				prevProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			   	props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			){
				isUpgrade = true;
			}

			// Choose GPU of same type with highest image quality.
			else if(
				prevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
				props.limits.maxImageDimension1D > prevProps.limits.maxImageDimension1D
			) {
				isUpgrade = true;
			}
		}

		if(isUpgrade) {
			ctx.physicalDevice = candidate;
			ctx.queueFamilyIndices[QueueRole::Graphics] = graphicsFamilyIndex;
			ctx.queueFamilyIndices[QueueRole::Presentation] = presentationFamilyIndex;
		}
	}

	crash_if(!ctx.physicalDevice);
}

void createDevice(VulkanContext& ctx) {

	std::set<uint32_t> uniqueIndices {
		ctx.queueFamilyIndices.begin(), 
		ctx.queueFamilyIndices.end()
	};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for(auto index : uniqueIndices) {
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

	crash_if(vkCreateDevice(ctx.physicalDevice, &createInfo, nullptr, &ctx.device) != VK_SUCCESS);

	for(auto role : range<QueueRole>(QueueRole::_Count)) {
		vkGetDeviceQueue(ctx.device, ctx.queueFamilyIndices[role], 0, &ctx.queues[role]);
	}
}

void createSwapchain(VulkanContext& ctx) {

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.windowSurface, &surfaceCapabilities);
	
	auto maxImages = (
		surfaceCapabilities.maxImageCount 
			? surfaceCapabilities.maxImageCount 
			: UINT32_MAX
	);
	
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = ctx.windowSurface;
	createInfo.minImageCount = std::min(surfaceCapabilities.minImageCount + 1, maxImages);
	createInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	createInfo.imageExtent = ctx.windowExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;

	std::set<uint32_t> uniqueIndices{
		ctx.queueFamilyIndices.begin(), 
		ctx.queueFamilyIndices.end()
	};
	auto sameQueue = (uniqueIndices.size() == 1);
	createInfo.imageSharingMode      = sameQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	createInfo.queueFamilyIndexCount = sameQueue ? QueueRole::_Count : 0;
	createInfo.pQueueFamilyIndices   = sameQueue ? ctx.queueFamilyIndices.data() : nullptr;

	crash_if(vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain) != VK_SUCCESS);

	ctx.swapchainImages = queryVulkanResources<VkImage, VkDevice, VkSwapchainKHR>(
		&vkGetSwapchainImagesKHR,
		ctx.device,
		ctx.swapchain
	);

	ctx.swapchainImageViews = mapToVector<decltype(ctx.swapchainImages), VkImageView>(
		ctx.swapchainImages,
		[&] (auto image) {
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
			crash_if(vkCreateImageView(ctx.device, &createInfo, nullptr, &view) != VK_SUCCESS);
			return std::move(view);
		}
	);
}

VkShaderModule& loadShader(VulkanContext& ctx, std::string const& name) {
	
	constexpr auto shaderPath = "../assets/shaders/";
	constexpr auto shaderExt = ".spv";

	auto path = shaderPath + name + shaderExt;
	std::cout << "Loading shader from path: " << path << lf;

	std::ifstream fs(path);
	std::stringstream ss;
	ss << fs.rdbuf();
	auto code = ss.str();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<uint32_t const*>(code.c_str());

	crash_if(vkCreateShaderModule(ctx.device, &createInfo, nullptr, &ctx.shaders[name]) != VK_SUCCESS);
	return ctx.shaders[name];
}

void createPipeline(VulkanContext& ctx) {
	
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

	auto renderPassInfo = VkRenderPassCreateInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = colorAttachments.size();
	renderPassInfo.pAttachments = colorAttachments.data();
	renderPassInfo.subpassCount = subpasses.size();
	renderPassInfo.pSubpasses = subpasses.data();

	crash_if(vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.renderPass) != VK_SUCCESS);

	ctx.swapchainFramebuffers = mapToVector<
		decltype(ctx.swapchainImageViews), 
		VkFramebuffer
	>(
		ctx.swapchainImageViews, 
		[&] (auto const& view) {
			auto framebufferInfo = VkFramebufferCreateInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.width = ctx.windowExtent.width;
			framebufferInfo.height = ctx.windowExtent.height;
			framebufferInfo.layers = 1;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &view;
			framebufferInfo.renderPass = ctx.renderPass;

			VkFramebuffer framebuffer;
			crash_if(vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS);
			return framebuffer;
		}
	);
	
	auto vertexInput = VkPipelineVertexInputStateCreateInfo{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexAttributeDescriptionCount = 0;
	vertexInput.pVertexAttributeDescriptions = nullptr;
	vertexInput.vertexBindingDescriptionCount = 0;
	vertexInput.pVertexBindingDescriptions = nullptr;

	auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(ctx.windowExtent.width);
	viewport.height = static_cast<float>(ctx.windowExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	auto scissor = VkRect2D{};
	scissor.extent = ctx.windowExtent;

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
			.module = loadShader(ctx, "triangle.vert"),
			.pName = "main",
		},
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = loadShader(ctx, "triangle.frag"),
			.pName = "main",
		}
	}; 

	auto createInfo = VkPipelineLayoutCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	crash_if(vkCreatePipelineLayout(ctx.device, &createInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS);

	auto pipelineInfo = VkGraphicsPipelineCreateInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = ctx.pipelineLayout;
	pipelineInfo.renderPass = ctx.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.stageCount = stages.size();
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterState;
	pipelineInfo.pColorBlendState = &blendState;
	pipelineInfo.pMultisampleState = &msaaState;

	crash_if(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.pipeline) != VK_SUCCESS);
}

void createCommandBuffers(VulkanContext& ctx) {

	auto poolInfo = VkCommandPoolCreateInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = QueueRole::Graphics;

	crash_if(vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS);

	auto allocateInfo = VkCommandBufferAllocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = ctx.commandPool;
	allocateInfo.commandBufferCount = ctx.swapchainImages.size();

	auto swapchainImageCount = ctx.swapchainImages.size();
	ctx.swapchainCommandBuffers.resize(swapchainImageCount);
	crash_if(vkAllocateCommandBuffers(ctx.device, &allocateInfo, ctx.swapchainCommandBuffers.data()) != VK_SUCCESS);
	
	auto clearValue = VkClearValue{ .color = { .float32 = { 1.0f, 0.0f, 0.0f, 1.0f } } };

	for(size_t i=0; i<swapchainImageCount; ++i) {

		auto& cmdbuf = ctx.swapchainCommandBuffers[i];

		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		crash_if(vkBeginCommandBuffer(cmdbuf, &beginInfo) != VK_SUCCESS);

		auto passInfo = VkRenderPassBeginInfo{};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passInfo.framebuffer = ctx.swapchainFramebuffers[i];
		passInfo.clearValueCount = 1;
		passInfo.pClearValues = &clearValue;
		passInfo.renderPass = ctx.renderPass;
		passInfo.renderArea.extent = ctx.windowExtent;

		vkCmdBeginRenderPass(cmdbuf, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline);
		vkCmdDraw(cmdbuf, 3, 1, 0, 0);
		vkCmdEndRenderPass(cmdbuf);

		crash_if(vkEndCommandBuffer(cmdbuf) != VK_SUCCESS);
	}
}

Renderer::Renderer() : m_pWindow(nullptr) {

	// Initialize GLFW.
	crash_if(!glfwInit());
	
	// Initialize Vulkan.
	createInstance(m_vulkanContext);
	m_pWindow = createWindow(m_vulkanContext, 1280, 720);
	selectPhysicalDevice(m_vulkanContext);
	createDevice(m_vulkanContext);
	createSwapchain(m_vulkanContext);
	createPipeline(m_vulkanContext);
	createCommandBuffers(m_vulkanContext);

	glfwShowWindow(m_pWindow);
}

bool Renderer::isWindowOpen() const {
	return m_pWindow && !glfwWindowShouldClose(m_pWindow);
}

void Renderer::handleWindowEvents() {
	glfwPollEvents();
}

void Renderer::renderScene(Scene const& scene) const {
	(void) scene;
}

void destroyVulkanResources(VulkanContext& ctx) {

	vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);

	for(auto framebuffer : ctx.swapchainFramebuffers) {
		vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
	}

	vkDestroyPipeline(ctx.device, ctx.pipeline, nullptr);
	vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
	vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout, nullptr);

	for(auto [name, shader] : ctx.shaders) {
		vkDestroyShaderModule(ctx.device, shader, nullptr);
	}

	for(auto view : ctx.swapchainImageViews) {
		vkDestroyImageView(ctx.device, view, nullptr);
	}

	vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
	vkDestroyDevice(ctx.device, nullptr);

	vkDestroySurfaceKHR(ctx.instance, ctx.windowSurface, nullptr);
	vkDestroyInstance(ctx.instance, nullptr);

	ctx = {};
}

Renderer::~Renderer() {

	destroyVulkanResources(m_vulkanContext);

	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	glfwTerminate();
}
