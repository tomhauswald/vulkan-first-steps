#include "vulkan_context.h"
#include "mesh.h"

#include <fstream>

constexpr auto validateReleaseBuild = false;
constexpr auto validate = debug || validateReleaseBuild;

constexpr auto requiredDeviceExtensions = std::array{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr auto validationLayers = std::array{
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
	createInfo.enabledLayerCount = validate ? validationLayers.size() : 0;
	createInfo.ppEnabledLayerNames = validate ? validationLayers.data() : nullptr;

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
			std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]) = graphicsFamilyIndex;
			std::get<uint32_t>(m_queueInfo[QueueRole::Presentation]) = presentationFamilyIndex;
		}
	}

	crashIf(!m_physicalDevice);
}

void VulkanContext::createDevice() {

	std::set<uint32_t> uniqueIndices{
		std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]),
		std::get<uint32_t>(m_queueInfo[QueueRole::Presentation])
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

	for (auto role : { QueueRole::Graphics, QueueRole::Presentation }) {
		vkGetDeviceQueue(
			m_device,
			std::get<uint32_t>(m_queueInfo[role]),
			0,
			&std::get<VkQueue>(m_queueInfo[role])
		);
	}
	auto poolInfo = VkCommandPoolCreateInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]);
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	crashIf(VK_SUCCESS != vkCreateCommandPool(
		m_device,
		&poolInfo,
		nullptr,
		&m_commandPool
	));
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format) {
	
	auto createInfo = VkImageViewCreateInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;

	auto& srr = createInfo.subresourceRange;
	srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	srr.levelCount = 1;
	srr.layerCount = 1;

	VkImageView view;
	crashIf(vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS);
	return view;
}

void VulkanContext::createSwapchain(bool vsync) {

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	crashIf(VK_SUCCESS != vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		m_physicalDevice,
		m_windowSurface,
		&surfaceCapabilities
	));

	auto swapchainInfo = VkSwapchainCreateInfoKHR{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = m_windowSurface;
	swapchainInfo.minImageCount = surfaceCapabilities.minImageCount;
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

	auto indices = std::array{ 
		std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]),
		std::get<uint32_t>(m_queueInfo[QueueRole::Presentation])
	};

	// Are graphics and presentation capabilities provided by the same queue family.
	auto sameQFam = (uniqueElemCount(indices) == 1);
	
	swapchainInfo.imageSharingMode = (
		(sameQFam)
		? (VK_SHARING_MODE_EXCLUSIVE)
		: (VK_SHARING_MODE_CONCURRENT)
		);

	if (sameQFam) {
		swapchainInfo.queueFamilyIndexCount = indices.size();
		swapchainInfo.pQueueFamilyIndices = indices.data();
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

	std::cout << "Created swapchain with " << m_swapchainImages.size() << " images." << lf;

	// Create image views for each swapchain image.
	m_swapchainImageViews = mapToVector<decltype(m_swapchainImages), VkImageView>(
		m_swapchainImages,
		[&](auto image) { 
			return createImageView(m_device, image, swapchainInfo.imageFormat); 
		}
	);

	// Allocate command buffers for each swapchain image.
	auto allocateInfo = VkCommandBufferAllocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.commandBufferCount = m_swapchainImages.size();

	m_swapchainCommandBuffers.resize(m_swapchainImages.size());
	crashIf(VK_SUCCESS != vkAllocateCommandBuffers(
		m_device,
		&allocateInfo,
		m_swapchainCommandBuffers.data()
	));
}

void VulkanContext::createDepthBuffer() {

	auto depthInfo = VkImageCreateInfo{};
	depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthInfo.extent.width = m_windowExtent.width;
	depthInfo.extent.height = m_windowExtent.height;
	depthInfo.extent.depth = 1;
	depthInfo.mipLevels = 1;
	depthInfo.arrayLayers = 1;
	depthInfo.imageType = VK_IMAGE_TYPE_2D;
	depthInfo.format = VK_FORMAT_D32_SFLOAT;
	depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthInfo.queueFamilyIndexCount = 1;
	depthInfo.pQueueFamilyIndices = &std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]);

	crashIf(VK_SUCCESS != vkCreateImage(
		m_device,
		&depthInfo,
		nullptr,
		&std::get<VkImage>(m_depthBuffer)
	));

	VkMemoryRequirements depthImageMemReqs;
	vkGetImageMemoryRequirements(
		m_device,
		std::get<VkImage>(m_depthBuffer),
		&depthImageMemReqs
	);

	auto depthAllocInfo = VkMemoryAllocateInfo{};
	depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthAllocInfo.allocationSize = depthImageMemReqs.size;
	depthAllocInfo.memoryTypeIndex = -1;

	for (size_t bit = 0; bit < sizeof(depthImageMemReqs.memoryTypeBits) * 8; ++bit) {
		if (nthBitHi(depthImageMemReqs.memoryTypeBits, bit)) {
			depthAllocInfo.memoryTypeIndex = static_cast<uint32_t>(bit);
			break;
		}
	}

	crashIf(VK_SUCCESS != vkAllocateMemory(
		m_device,
		&depthAllocInfo,
		nullptr,
		&std::get<VkDeviceMemory>(m_depthBuffer)
	));

	crashIf(VK_SUCCESS != vkBindImageMemory(
		m_device,
		std::get<VkImage>(m_depthBuffer),
		std::get<VkDeviceMemory>(m_depthBuffer),
		0
	));

	auto depthViewInfo = VkImageViewCreateInfo{};
	depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthViewInfo.image = std::get<VkImage>(m_depthBuffer);
	depthViewInfo.format = VK_FORMAT_D32_SFLOAT;

	auto& srr = depthViewInfo.subresourceRange;
	srr.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	srr.levelCount = 1;
	srr.layerCount = 1;

	crashIf(VK_SUCCESS != vkCreateImageView(
		m_device,
		&depthViewInfo,
		nullptr,
		&std::get<VkImageView>(m_depthBuffer)
	));
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

void VulkanContext::onFrameBegin() {

	// Find out the next swapchain image index to render to.
	crashIf(VK_SUCCESS != vkAcquireNextImageKHR(
		m_device,
		m_swapchain,
		UINT64_MAX,
		m_semaphores[m_semaphoreIndex][DeviceEvent::SwapchainImageAvailable],
		VK_NULL_HANDLE,
		&m_swapchainImageIndex
	));

	// Wait for all operations on the swapchain image to complete.
	crashIf(VK_SUCCESS != vkWaitForFences(
		m_device,
		1,
		&m_swapchainFences[m_swapchainImageIndex],
		VK_TRUE,
		UINT64_MAX
	));

	// Allow fence to be reused in the future.
	crashIf(VK_SUCCESS != vkResetFences(
		m_device,
		1,
		&m_swapchainFences[m_swapchainImageIndex]
	));
	

	auto cmdbufBeginInfo = VkCommandBufferBeginInfo{};
	cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Start recording the command buffer for this frame.
	crashIf(VK_SUCCESS != vkBeginCommandBuffer(
		m_swapchainCommandBuffers[m_swapchainImageIndex], 
		&cmdbufBeginInfo
	));
	
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.39f, 0.58f, 0.93f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	auto passBeginInfo = VkRenderPassBeginInfo{};
	passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	passBeginInfo.framebuffer = m_swapchainFramebuffers[m_swapchainImageIndex];
	passBeginInfo.clearValueCount = 2;
	passBeginInfo.pClearValues = &clearValues[0];
	passBeginInfo.renderPass = m_renderPass;
	passBeginInfo.renderArea.extent = m_windowExtent;

	vkCmdBeginRenderPass(
		m_swapchainCommandBuffers[m_swapchainImageIndex], 
		&passBeginInfo, 
		VK_SUBPASS_CONTENTS_INLINE
	);

	vkCmdBindPipeline(
		m_swapchainCommandBuffers[m_swapchainImageIndex], 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		m_pipeline
	);

	clearUniformData();
	m_boundTextures = { nullptr };
}

void VulkanContext::draw(VkBuffer vbuf, VkBuffer ibuf, uint32_t count) {
	
	static constexpr auto offsetZero = VkDeviceSize{};
	auto cmdbuf = m_swapchainCommandBuffers[m_swapchainImageIndex];

	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vbuf,	&offsetZero);

	// Draw indexed.
	if (ibuf) {
		vkCmdBindIndexBuffer(cmdbuf, ibuf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdbuf, count, 1, 0, 0, 0);
	}

	// Don't draw indexed.
	else vkCmdDraw(cmdbuf, count, 1, 0, 0);
}

void VulkanContext::onFrameEnd() {

	// End of commands.

	vkCmdEndRenderPass(m_swapchainCommandBuffers[m_swapchainImageIndex]);
	crashIf(vkEndCommandBuffer(m_swapchainCommandBuffers[m_swapchainImageIndex]) != VK_SUCCESS);

	auto stage = VkPipelineStageFlags{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_swapchainCommandBuffers[m_swapchainImageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &stage;
	submitInfo.pWaitSemaphores = &m_semaphores[m_semaphoreIndex][DeviceEvent::SwapchainImageAvailable];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_semaphores[m_semaphoreIndex][DeviceEvent::FrameRenderingDone];

	crashIf(VK_SUCCESS != vkQueueSubmit(
		std::get<VkQueue>(m_queueInfo[QueueRole::Graphics]),
		1,
		&submitInfo,
		m_swapchainFences[m_swapchainImageIndex]
	));

	auto presentInfo = VkPresentInfoKHR{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_semaphores[m_semaphoreIndex][DeviceEvent::FrameRenderingDone];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &m_swapchainImageIndex;

	crashIf(VK_SUCCESS != vkQueuePresentKHR(
		std::get<VkQueue>(m_queueInfo[QueueRole::Presentation]),
		&presentInfo
	));

	m_semaphoreIndex = (m_semaphoreIndex + 1) % m_swapchainImages.size();
}

VkDeviceMemory VulkanContext::allocateDeviceMemory(
	VkMemoryRequirements const& memReqs,
	VkMemoryPropertyFlags memProps
) {
	auto allocInfo = VkMemoryAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = -1;

	// Find suitable memory type.
	auto const& devMemProps = m_physicalDeviceMemoryProperties.at(m_physicalDevice);
	for (uint32_t i = 0; i < devMemProps.memoryTypeCount; ++i) {

		if (!nthBitHi(memReqs.memoryTypeBits, i)) continue; // Skip types according to mask.

		if (satisfiesBitMask(devMemProps.memoryTypes[i].propertyFlags, memProps)) {
			allocInfo.memoryTypeIndex = i;
			break;
		}
	}

	VkDeviceMemory mem;
	crashIf(vkAllocateMemory(m_device, &allocInfo, nullptr, &mem) != VK_SUCCESS);
	return mem;
}

VulkanBufferInfo VulkanContext::createBuffer(
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memProps,
	VkDeviceSize bytes
) {
	VulkanBufferInfo result;
	result.usage = usage;
	result.memoryProperties = memProps;
	result.sizeInBytes = bytes;

	auto bufferInfo = VkBufferCreateInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]);
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.size = bytes;

	crashIf(VK_SUCCESS != vkCreateBuffer(m_device, &bufferInfo, nullptr, &result.buffer));

	auto memReqs = VkMemoryRequirements{};
	vkGetBufferMemoryRequirements(m_device, result.buffer, &memReqs);

	result.memory = allocateDeviceMemory(memReqs, memProps);
	crashIf(vkBindBufferMemory(m_device, result.buffer, result.memory, 0) != VK_SUCCESS);

	return result;
}

VulkanTextureInfo VulkanContext::createTexture(uint32_t width, uint32_t height, uint32_t const* pixels) {
	
	auto result = VulkanTextureInfo{};
	result.width = width;
	result.height = height;
	
	auto bytes = VulkanTextureInfo::bytesPerPixel * width * height;
	auto staging = createHostBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		| VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
		bytes
	);
	writeDeviceMemory(staging.memory, pixels, bytes);

	auto imageInfo = VkImageCreateInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.arrayLayers = 1;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.mipLevels = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	crashIf(VK_SUCCESS != vkCreateImage(m_device, &imageInfo, nullptr, &result.image));

	auto memReqs = VkMemoryRequirements{};
	vkGetImageMemoryRequirements(m_device, result.image, &memReqs);

	result.memory = allocateDeviceMemory(memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	crashIf(VK_SUCCESS != vkBindImageMemory(m_device, result.image, result.memory, 0));

	// Copy staging buffer into texture.
	runDeviceCommands([&](VkCommandBuffer cmdbuf) {

		auto barrier = VkImageMemoryBarrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = result.image;

		auto& srr = barrier.subresourceRange;
		srr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		srr.levelCount = 1;
		srr.layerCount = 1;

		// Transition image layout into being writeable.
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		
		vkCmdPipelineBarrier(
			cmdbuf, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 
			1, &barrier
		);

		auto region = VkBufferImageCopy{};
		region.imageExtent = { width, height, 1 };

		auto& isr = region.imageSubresource;
		isr.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		isr.layerCount = 1;

		vkCmdCopyBufferToImage(
			cmdbuf, 
			staging.buffer, 
			result.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			1, &region
		);

		// Transition image layout into being usable by the shader.
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		vkCmdPipelineBarrier(
			cmdbuf,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr,
			1, &barrier
		);
	});

	destroyBuffer(staging);

	result.view = createImageView(m_device, result.image, imageInfo.format);
	
	auto layouts = repeat(m_samplerDescriptorSetLayout, VulkanTextureInfo::numSlots);
	auto setAllocInfo = VkDescriptorSetAllocateInfo{};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_descriptorPool;
	setAllocInfo.descriptorSetCount = layouts.size();
	setAllocInfo.pSetLayouts = layouts.data();

	crashIf(VK_SUCCESS != vkAllocateDescriptorSets(
		m_device,
		&setAllocInfo,
		std::data(result.samplerSlotDescriptorSets)
	));

	auto samplerInfo = VkDescriptorImageInfo{};
	samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	samplerInfo.imageView = result.view;
	samplerInfo.sampler = m_sampler;

	for (auto slot : range(VulkanTextureInfo::numSlots)) {
		auto write = VkWriteDescriptorSet{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = result.samplerSlotDescriptorSets[slot];
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = 0;
		write.pImageInfo = &samplerInfo;
	
		vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
	}

	return result;
}

VulkanBufferInfo VulkanContext::createVertexBuffer(std::vector<VPositionColorTexcoord> const& vertices) {
	
	auto bytes = vertices.size() * sizeof(VPositionColorTexcoord);

	// Create CPU-accessible buffer.
	auto host = createHostBuffer(
		  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
		| VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		bytes
	);

	// Write buffer data.
	writeDeviceMemory(host.memory, vertices.data(), bytes);
	
	// Upload to GPU.
	return uploadToDevice(host);
}

VulkanBufferInfo VulkanContext::createIndexBuffer(std::vector<uint32_t> const& indices) {
	
	auto bytes = indices.size() * sizeof(uint32_t);

	// Create CPU-accessible buffer.
	auto host = createHostBuffer(
		  VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		| VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		bytes
	);

	// Write buffer data.
	writeDeviceMemory(host.memory, indices.data(), bytes);

	// Upload to GPU.
	return uploadToDevice(host);
}

void VulkanContext::runDeviceCommands(std::function<void(VkCommandBuffer)> commands) {
	
	auto cmdbufInfo = VkCommandBufferAllocateInfo{};
	cmdbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdbufInfo.commandPool = m_commandPool;
	cmdbufInfo.commandBufferCount = 1;

	VkCommandBuffer cmdbuf;
	crashIf(VK_SUCCESS != vkAllocateCommandBuffers(
		m_device,
		&cmdbufInfo,
		&cmdbuf
	));

	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	crashIf(VK_SUCCESS != vkBeginCommandBuffer(cmdbuf, &beginInfo));
	commands(cmdbuf);
	crashIf(VK_SUCCESS != vkEndCommandBuffer(cmdbuf));

	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuf;

	crashIf(VK_SUCCESS != vkQueueSubmit(
		std::get<VkQueue>(m_queueInfo[QueueRole::Graphics]),
		1,
		&submitInfo,
		VK_NULL_HANDLE
	));

	// Wait for completion.
	crashIf(VK_SUCCESS != vkQueueWaitIdle(
		std::get<VkQueue>(m_queueInfo[QueueRole::Graphics]))
	);
}

VulkanUboInfo& VulkanContext::growUniformBufferSequence() {

	m_swapchainUboSeqs[m_swapchainImageIndex].push_back({});
	auto& ubo = m_swapchainUboSeqs[m_swapchainImageIndex].back();

	auto buffer = createHostBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VulkanLimits::maxUniformBufferRange
	);

	ubo.buffer = buffer.buffer;
	ubo.memory = buffer.memory;
	ubo.memoryProperties = buffer.memoryProperties;
	ubo.sizeInBytes = buffer.sizeInBytes;
	ubo.usage = buffer.usage;

	auto descSetInfo = VkDescriptorSetAllocateInfo{};
	descSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetInfo.descriptorPool = m_descriptorPool;
	descSetInfo.descriptorSetCount = 1;
	descSetInfo.pSetLayouts = &m_uniformDescriptorSetLayout;

	crashIf(VK_SUCCESS != vkAllocateDescriptorSets(
		m_device,
		&descSetInfo,
		&ubo.descriptorSet
	));

	auto uniformInfo = VkDescriptorBufferInfo{};
	auto uniformWrite = VkWriteDescriptorSet{};

	uniformInfo.buffer = ubo.buffer;
	uniformInfo.offset = 0;
	uniformInfo.range = VK_WHOLE_SIZE;// ubo.sizeInBytes;

	uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformWrite.descriptorCount = 1;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uniformWrite.dstBinding = 0;
	uniformWrite.dstSet = ubo.descriptorSet;
	uniformWrite.pBufferInfo = &uniformInfo;

	vkUpdateDescriptorSets(m_device, 1, &uniformWrite, 0, nullptr);

	auto num = m_swapchainUboSeqs[m_swapchainImageIndex].size();
	std::cout << "Grew UBO sequence [" << m_swapchainImageIndex << "] to " << num << " buffers (" << num * VulkanLimits::maxUniformBufferRange << " bytes)." << lf;

	return ubo;
}

VulkanBufferInfo VulkanContext::uploadToDevice(VulkanBufferInfo hostBufferInfo) {

	auto usage = hostBufferInfo.usage;
	usage &= ~VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	// Create GPU-local buffer.
	auto deviceBufferInfo = createDeviceBuffer(usage, hostBufferInfo.sizeInBytes);

	// Transfer data from CPU to GPU.
	runDeviceCommands([&](VkCommandBuffer cmdbuf) {
		auto region = VkBufferCopy{};
		region.size = hostBufferInfo.sizeInBytes;
		vkCmdCopyBuffer(cmdbuf, hostBufferInfo.buffer, deviceBufferInfo.buffer, 1, &region);
	});

	destroyBuffer(hostBufferInfo);
	return deviceBufferInfo;
}

void VulkanContext::createPipeline(
	std::string const& vertexShaderName,
	std::string const& fragmentShaderName,
	VkVertexInputBindingDescription const& binding,
	std::vector<VkVertexInputAttributeDescription> const& attributes,
	bool enableDepthTest,
	VkFilter textureFilterMode
) {
	m_vertexBinding = binding;
	m_vertexAttributes = attributes;

	auto colorAttachment = VkAttachmentDescription{};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	auto colorAttachmentReference = VkAttachmentReference{};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	auto depthAttachment = VkAttachmentDescription{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	auto depthAttachmentReference = VkAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	auto subpass = VkSubpassDescription{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = enableDepthTest ? &depthAttachmentReference : nullptr;

	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	auto attachments = std::vector{ colorAttachment };
	if(enableDepthTest) attachments.push_back(depthAttachment);

	auto renderPassInfo = VkRenderPassCreateInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
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

			auto attachments = std::vector{ view };
			if (enableDepthTest) attachments.push_back(std::get<VkImageView>(m_depthBuffer));

			auto framebufferInfo = VkFramebufferCreateInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.width = m_windowExtent.width;
			framebufferInfo.height = m_windowExtent.height;
			framebufferInfo.layers = 1;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
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


	// Create descriptor set layouts.

	auto dsLayoutCreateInfo = VkDescriptorSetLayoutCreateInfo{};
	dsLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	dsLayoutCreateInfo.bindingCount = 1;

	auto uniformBinding = VkDescriptorSetLayoutBinding{};
	uniformBinding.binding = 0;
	uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uniformBinding.descriptorCount = 1;
	uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	dsLayoutCreateInfo.pBindings = &uniformBinding;
	crashIf(VK_SUCCESS != vkCreateDescriptorSetLayout(
		m_device,
		&dsLayoutCreateInfo,
		nullptr,
		&m_uniformDescriptorSetLayout
	));

	auto samplerBinding = VkDescriptorSetLayoutBinding{};
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = 1;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	dsLayoutCreateInfo.pBindings = &samplerBinding;
	crashIf(VK_SUCCESS != vkCreateDescriptorSetLayout(
		m_device,
		&dsLayoutCreateInfo,
		nullptr,
		&m_samplerDescriptorSetLayout
	));
	
	auto descriptorSetLayouts = std::vector{ m_uniformDescriptorSetLayout };
	for (auto const& layout : repeat(m_samplerDescriptorSetLayout, VulkanTextureInfo::numSlots)) {
		descriptorSetLayouts.push_back(layout);
	}

	auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = std::size(descriptorSetLayouts);
	pipelineLayoutInfo.pSetLayouts = std::data(descriptorSetLayouts);

	auto pushConstantRange = VkPushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = VulkanLimits::maxPushConstantsSize;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	crashIf(VK_SUCCESS != vkCreatePipelineLayout(
		m_device,
		&pipelineLayoutInfo,
		nullptr,
		&m_pipelineLayout
	));


	// Setup pipeline stages.

	auto viewport = VkViewport{};
	viewport.x = 0.0f;
	viewport.y = m_windowExtent.height - 1.0f;
	viewport.width = static_cast<float>(m_windowExtent.width);
	viewport.height = -1.0f * m_windowExtent.height;
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

	auto depthStencilState = VkPipelineDepthStencilStateCreateInfo{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;

	auto rasterState = VkPipelineRasterizationStateCreateInfo{};
	rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.lineWidth = 1.0f;
	rasterState.cullMode = debug ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
	rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterState.polygonMode = VK_POLYGON_MODE_FILL;

	auto blendAttachmentState = VkPipelineColorBlendAttachmentState{};
	blendAttachmentState.blendEnable = VK_TRUE;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask = 0b1111;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	auto blendState = VkPipelineColorBlendStateCreateInfo{};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &blendAttachmentState;

	auto msaaState = VkPipelineMultisampleStateCreateInfo{};
	msaaState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msaaState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	if (!m_shaders.count(vertexShaderName)) loadShader(vertexShaderName);
	if (!m_shaders.count(fragmentShaderName)) loadShader(fragmentShaderName);

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
	pipelineInfo.pDepthStencilState = enableDepthTest ? &depthStencilState : nullptr;

	crashIf(VK_SUCCESS != vkCreateGraphicsPipelines(
		m_device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&m_pipeline
	));

	// Create descriptor pools.

	auto uniformPoolSize = VkDescriptorPoolSize{};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = UINT16_MAX;

	auto samplerPoolSize = VkDescriptorPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = UINT16_MAX;

	auto poolSizes = { uniformPoolSize, samplerPoolSize };

	auto descPoolInfo = VkDescriptorPoolCreateInfo{};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.poolSizeCount = std::size(poolSizes);
	descPoolInfo.pPoolSizes = std::data(poolSizes);
	descPoolInfo.maxSets = 2 * UINT16_MAX;

	crashIf(VK_SUCCESS != vkCreateDescriptorPool(
		m_device,
		&descPoolInfo,
		nullptr,
		&m_descriptorPool
	));

	// Create uniform buffer sequences.

	m_swapchainUboSeqs.resize(m_swapchainImages.size());

	// Create sampler.

	auto samplerInfo = VkSamplerCreateInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = textureFilterMode;
	samplerInfo.magFilter = textureFilterMode;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	
	crashIf(VK_SUCCESS != vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler));

	// Create synchronization means.

	auto semaphoreInfo = VkSemaphoreCreateInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	auto fenceInfo = VkFenceCreateInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	m_semaphores.resize(m_swapchainImages.size());
	m_swapchainFences.resize(m_swapchainImages.size());

	for (auto img : range(m_swapchainImages.size())) {

		crashIf(VK_SUCCESS != vkCreateFence(
			m_device,
			&fenceInfo,
			nullptr,
			&m_swapchainFences[img]
		));

		for (auto evt : {
			DeviceEvent::SwapchainImageAvailable,
			DeviceEvent::FrameRenderingDone
		}) {
			crashIf(VK_SUCCESS != vkCreateSemaphore(
				m_device,
				&semaphoreInfo,
				nullptr,
				&m_semaphores[img][evt]
			));
		}
	}
}

void VulkanContext::setPushConstantData(void const* data, uint32_t bytes) {
	vkCmdPushConstants(
		m_swapchainCommandBuffers[m_swapchainImageIndex],
		m_pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		bytes,
		data
	);
}

void VulkanContext::bindTextureSlot(uint8_t slot, VulkanTextureInfo const& txr) {
	if (m_boundTextures[slot] != &txr) {
		vkCmdBindDescriptorSets(
			m_swapchainCommandBuffers[m_swapchainImageIndex],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout,
			1 + slot,
			1,
			&txr.samplerSlotDescriptorSets[slot],
			0,
			nullptr
		);
		m_boundTextures[slot] = &txr;
	}
}

void VulkanContext::setUniformData(void const* data, uint32_t bytes) {

	crashIf(bytes > VulkanLimits::maxUniformBufferRange);

	VulkanUboInfo* pDestUbo = nullptr;

	// Find uniform buffer with sufficient space for data.
	for (auto& ubo : m_swapchainUboSeqs[m_swapchainImageIndex]) {
		if (ubo.bytesUsed + bytes <= ubo.sizeInBytes) {
			pDestUbo = &ubo;
		}
	}

	if(!pDestUbo) pDestUbo = &growUniformBufferSequence();

	auto offset = static_cast<uint32_t>(pDestUbo->bytesUsed);

	writeDeviceMemory(pDestUbo->memory, data, bytes, pDestUbo->bytesUsed);

	// Advance usage pointer, taking alignment into account.
	pDestUbo->bytesUsed += bytes;
	auto overshoot = pDestUbo->bytesUsed % VulkanLimits::minUniformBufferOffsetAlignment;
	pDestUbo->bytesUsed += VulkanLimits::minUniformBufferOffsetAlignment - overshoot;

	vkCmdBindDescriptorSets(
		m_swapchainCommandBuffers[m_swapchainImageIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineLayout,
		0, 1, &pDestUbo->descriptorSet,
		1, &offset
	);
}

VulkanContext::~VulkanContext() {

	for (auto& pair : m_semaphores) {
		for (auto [_, sem] : pair) {
			vkDestroySemaphore(m_device, sem, nullptr);
		}
	}

	for (auto fence : m_swapchainFences) {
		vkDestroyFence(m_device, fence, nullptr);
	}

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	for (auto framebuffer : m_swapchainFramebuffers) {
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}

	for (auto& seq : m_swapchainUboSeqs) {
		for (auto& buf : seq) {
			destroyBuffer(buf);
		}
	}

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_uniformDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_samplerDescriptorSetLayout, nullptr);
	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	for (auto [name, shader] : m_shaders) {
		vkDestroyShaderModule(m_device, shader, nullptr);
	}

	auto [image, view, mem] = m_depthBuffer;
	vkDestroyImageView(m_device, view, nullptr);
	vkDestroyImage(m_device, image, nullptr);
	vkFreeMemory(m_device, mem, nullptr);

	for (auto view : m_swapchainImageViews) {
		vkDestroyImageView(m_device, view, nullptr);
	}

	vkDestroySampler(m_device, m_sampler, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
