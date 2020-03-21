#include "renderer.h"
#include "vertex_formats.h"

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

	crashIf(vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS);
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
	crashIf(!window);

	// Window surface creation must succeed.
	crashIf(glfwCreateWindowSurface(ctx.instance, window, nullptr, &ctx.windowSurface) != VK_SUCCESS);

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

		vkGetPhysicalDeviceMemoryProperties(candidate, &ctx.physicalDeviceMemoryProperties[candidate]);

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
			ctx.queueFamilyIndices[QUEUE_ROLE_GRAPHICS] = graphicsFamilyIndex;
			ctx.queueFamilyIndices[QUEUE_ROLE_PRESENTATION] = presentationFamilyIndex;
		}
	}

	crashIf(!ctx.physicalDevice);
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

	crashIf(vkCreateDevice(ctx.physicalDevice, &createInfo, nullptr, &ctx.device) != VK_SUCCESS);

	for(auto role : range<QueueRole>(NUM_QUEUE_ROLES)) {
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
	createInfo.queueFamilyIndexCount = sameQueue ? NUM_QUEUE_ROLES : 0;
	createInfo.pQueueFamilyIndices   = sameQueue ? ctx.queueFamilyIndices.data() : nullptr;

	crashIf(vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &ctx.swapchain) != VK_SUCCESS);

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
			crashIf(vkCreateImageView(ctx.device, &createInfo, nullptr, &view) != VK_SUCCESS);
			return std::move(view);
		}
	);
}

VkShaderModule const& loadShader(VulkanContext& ctx, std::string const& name) {
	
	constexpr auto shaderBasePath = "../assets/shaders/";
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

	crashIf(vkCreateShaderModule(ctx.device, &createInfo, nullptr, &ctx.shaders[name]) != VK_SUCCESS);

	return ctx.shaders.at(name);
}

template<typename Vertex>
void createPipeline(VulkanContext& ctx, std::string const& vertexShaderName, std::string const& fragmentShaderName) {
	
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

	crashIf(vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.renderPass) != VK_SUCCESS);

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
			crashIf(vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS);
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
			.module = ctx.shaders[vertexShaderName],
			.pName = "main",
		},
		VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = ctx.shaders[fragmentShaderName],
			.pName = "main",
		}
	}; 

	auto createInfo = VkPipelineLayoutCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	crashIf(vkCreatePipelineLayout(ctx.device, &createInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS);

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

	crashIf(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.pipeline) != VK_SUCCESS);
}

template<typename Vertex>
VkBuffer createVertexBuffer(VulkanContext& ctx, View<Vertex> vertices) {

	auto vertexBufferInfo = VkBufferCreateInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.queueFamilyIndexCount = 1;
	vertexBufferInfo.pQueueFamilyIndices = &ctx.queueFamilyIndices[QUEUE_ROLE_GRAPHICS];
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vertexBufferInfo.size = static_cast<VkDeviceSize>(vertices.bytes());

	auto vertexBuffer = VkBuffer{};
	vkCreateBuffer(ctx.device, &vertexBufferInfo, nullptr, &vertexBuffer);

	auto memReqmt = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(ctx.device, vertexBuffer, &memReqmt);
	
	auto memReqProps = VkMemoryPropertyFlags{ 
		  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 
	};

	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqmt.size;
	allocInfo.memoryTypeIndex = -1;

	// Find suitable memory type.
	auto const& devMemProps = ctx.physicalDeviceMemoryProperties.at(ctx.physicalDevice);
	for (uint32_t i = 0; i < devMemProps.memoryTypeCount; ++i) {
		
		if (!nthBitHi(memReqmt.memoryTypeBits, i)) continue; // Skip types according to mask.
		
		else if (devMemProps.memoryTypes[i].propertyFlags & memReqProps) {
			allocInfo.memoryTypeIndex = i;
			break;
		}
	}

	auto vertexBufferMemory = VkDeviceMemory{};
	crashIf(vkAllocateMemory(ctx.device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS);
	crashIf(vkBindBufferMemory(ctx.device, vertexBuffer, vertexBufferMemory, 0) != VK_SUCCESS);

	ctx.bufferMemories[vertexBuffer] = vertexBufferMemory;

	void* raw;
	crashIf(vkMapMemory(ctx.device, vertexBufferMemory, 0, vertexBufferInfo.size, 0, &raw) != VK_SUCCESS);
	{
		crashIf(memcpy_s(raw, vertexBufferInfo.size, vertices.items(), vertices.bytes()) != 0);
	}
	vkUnmapMemory(ctx.device, vertexBufferMemory);

	return vertexBuffer;
}

void createCommandBuffers(VulkanContext& ctx) {

	auto poolInfo = VkCommandPoolCreateInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = ctx.queueFamilyIndices[QUEUE_ROLE_GRAPHICS];

	crashIf(vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS);

	auto allocateInfo = VkCommandBufferAllocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = ctx.commandPool;
	allocateInfo.commandBufferCount = ctx.swapchainImages.size();

	auto swapchainImageCount = ctx.swapchainImages.size();
	ctx.swapchainCommandBuffers.resize(swapchainImageCount);
	crashIf(vkAllocateCommandBuffers(ctx.device, &allocateInfo, ctx.swapchainCommandBuffers.data()) != VK_SUCCESS);
	
	auto clearValue = VkClearValue{ .color = { .float32 = { 1.0f, 0.0f, 0.0f, 1.0f } } };

	auto vertices = std::array{
		Vertex2dColored{ {+0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		Vertex2dColored{ {+0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },
		Vertex2dColored{ {-0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} },
	};

	auto vertexBuffer = createVertexBuffer(ctx, makeContainerView(vertices));

	for(size_t i=0; i<swapchainImageCount; ++i) {

		auto& cmdbuf = ctx.swapchainCommandBuffers[i];

		auto cmdbufBeginInfo = VkCommandBufferBeginInfo{};
		cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		crashIf(vkBeginCommandBuffer(cmdbuf, &cmdbufBeginInfo) != VK_SUCCESS);
		{
			auto passBeginInfo = VkRenderPassBeginInfo{};
			passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			passBeginInfo.framebuffer = ctx.swapchainFramebuffers[i];
			passBeginInfo.clearValueCount = 1;
			passBeginInfo.pClearValues = &clearValue;
			passBeginInfo.renderPass = ctx.renderPass;
			passBeginInfo.renderArea.extent = ctx.windowExtent;

			vkCmdBeginRenderPass(cmdbuf, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline);

				auto const offsetZero = VkDeviceSize{};
				vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vertexBuffer, &offsetZero);

				vkCmdDraw(cmdbuf, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			}
			vkCmdEndRenderPass(cmdbuf);
		}
		crashIf(vkEndCommandBuffer(cmdbuf) != VK_SUCCESS);
	}

	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	crashIf(vkCreateSemaphore(
			ctx.device, 
			&semaphoreInfo, 
			nullptr, 
			&ctx.semaphores[RENDER_EVENT_IMAGE_AVAILABLE]
	) != VK_SUCCESS);

	crashIf(vkCreateSemaphore(
		ctx.device,
		&semaphoreInfo,
		nullptr,
		&ctx.semaphores[RENDER_EVENT_FRAME_DONE]
	) != VK_SUCCESS);
}

Renderer::Renderer() : m_pWindow(nullptr) {
	
	// Initialize GLFW.
	crashIf(!glfwInit());
	
	// Initialize Vulkan.
	createInstance(m_vulkanContext);
	m_pWindow = createWindow(m_vulkanContext, 2560, 1440);
	selectPhysicalDevice(m_vulkanContext);
	createDevice(m_vulkanContext);
	createSwapchain(m_vulkanContext);

	// Load shaders.
	auto const vertexShaderName = "2d-colored.vert";
	auto const fragmentShaderName = "unshaded.frag"; 
	
	for (auto const& name : {vertexShaderName, fragmentShaderName}) {
		loadShader(m_vulkanContext, name);
	}

	createPipeline<Vertex2dColored>(m_vulkanContext, vertexShaderName, fragmentShaderName);
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

	(void)scene;

	uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_vulkanContext.device,
		m_vulkanContext.swapchain,
		UINT64_MAX,
		m_vulkanContext.semaphores[RENDER_EVENT_IMAGE_AVAILABLE],
		VK_NULL_HANDLE,
		&imageIndex
	);

	auto stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vulkanContext.swapchainCommandBuffers[imageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &stage;
	submitInfo.pWaitSemaphores = &m_vulkanContext.semaphores[RENDER_EVENT_IMAGE_AVAILABLE];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_vulkanContext.semaphores[RENDER_EVENT_FRAME_DONE];

	crashIf(VK_SUCCESS != vkQueueSubmit(
		m_vulkanContext.queues[QUEUE_ROLE_GRAPHICS], 
		1, 
		&submitInfo, 
		VK_NULL_HANDLE
	));

	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_vulkanContext.semaphores[RENDER_EVENT_FRAME_DONE];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_vulkanContext.swapchain;
	presentInfo.pImageIndices = &imageIndex;

	crashIf(VK_SUCCESS != vkQueuePresentKHR(
		m_vulkanContext.queues[QUEUE_ROLE_PRESENTATION], 
		&presentInfo
	));

	crashIf(VK_SUCCESS != vkQueueWaitIdle(m_vulkanContext.queues[QUEUE_ROLE_PRESENTATION]));
}

void destroyVulkanResources(VulkanContext& ctx) {

	vkDeviceWaitIdle(ctx.device);

	for (auto [buffer, memory] : ctx.bufferMemories) {
		vkDestroyBuffer(ctx.device, buffer, nullptr);
		vkFreeMemory(ctx.device, memory, nullptr);
	}

	for (auto semaphore : ctx.semaphores) {
		vkDestroySemaphore(ctx.device, semaphore, nullptr);
	}

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
