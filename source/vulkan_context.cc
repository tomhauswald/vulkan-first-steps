#include "vulkan_context.h"
#include "mesh.h"

#include <fstream>

constexpr std::array requiredDeviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
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
			return view;
		}
	);

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

	auto depthAllocInfo = VkMemoryAllocateInfo{};
	depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthAllocInfo.allocationSize = sizeof(glm::vec4) * m_windowExtent.width * m_windowExtent.height;

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

void VulkanContext::recordFrameCommandBuffer() {

	// @refactor Initial allocation of command buffers.
	if (m_swapchainCommandBuffers.size() == 0) {
		
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

	auto& commandBuffer = m_swapchainCommandBuffers[m_swapchainImageIndex];
	auto& framebuffer   = m_swapchainFramebuffers[m_swapchainImageIndex];
	auto& descriptorSet = m_swapchainDescriptorSets[m_swapchainImageIndex];

	auto cmdbufBeginInfo = VkCommandBufferBeginInfo{};
	cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	crashIf(vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo) != VK_SUCCESS);
	{
		VkClearValue clearValues[2];
		clearValues[0].color = {{ 0.39f, 0.58f, 0.93f }};
		clearValues[1].depthStencil = { 1.0f, 0 };
		
		auto passBeginInfo = VkRenderPassBeginInfo{};
		passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passBeginInfo.framebuffer = framebuffer;
		passBeginInfo.clearValueCount = 2;
		passBeginInfo.pClearValues = &clearValues[0];
		passBeginInfo.renderPass = m_renderPass;
		passBeginInfo.renderArea.extent = m_windowExtent;

		vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
				
			// Bind the uniform buffer of the current frame.
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_pipelineLayout,
				0,
				1,
				&descriptorSet,
				0,
				nullptr
			);

			for (auto const& [mesh, data] : m_meshRenderCommands) {
				
				vkCmdPushConstants(
					commandBuffer,
					m_pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(PushConstantData),
					&data
				);

				mesh.appendDrawCommand(commandBuffer);
			}
		}
		vkCmdEndRenderPass(commandBuffer);
	}
	crashIf(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS);
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

	m_meshRenderCommands.clear();
}

void VulkanContext::renderMesh(Mesh const& mesh, PushConstantData const& data) {
	m_meshRenderCommands.push_back({mesh, data});
}

void VulkanContext::onFrameEnd() {

	recordFrameCommandBuffer();

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

std::tuple<VkBuffer, VkDeviceMemory> VulkanContext::createBuffer(
	VkBufferUsageFlags usageMask,
	VkMemoryPropertyFlags propertyMask,
	size_t bytes
) {
	auto bufferInfo = VkBufferCreateInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.queueFamilyIndexCount = 1;
	bufferInfo.pQueueFamilyIndices = &std::get<uint32_t>(m_queueInfo[QueueRole::Graphics]);
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

std::tuple<VkBuffer, VkDeviceMemory> VulkanContext::createUniformBuffer(size_t bytes) {
	return createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bytes
	);
}

std::tuple<VkBuffer, VkDeviceMemory> VulkanContext::createVertexBuffer(
	std::vector<Vertex> const& vertices
) {
	auto bytes = vertices.size() * sizeof(Vertex);

	// Create CPU-accessible buffer.
	auto host = createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
		| VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bytes
	);

	// Write buffer data.
	writeDeviceMemory(std::get<VkDeviceMemory>(host), vertices.data(), bytes);
	
	// Upload to GPU.
	return uploadToDevice(host, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bytes);
}

void VulkanContext::writeDeviceMemory(
	VkDeviceMemory& mem,
	void const* data,
	size_t bytes,
	size_t offset
) {
	// Map cpu-accessible buffer into RAM.
	void* raw;
	crashIf(VK_SUCCESS != vkMapMemory(
		m_device,
		mem,
		static_cast<VkDeviceSize>(offset),
		static_cast<VkDeviceSize>(bytes),
		0,
		&raw
	));

	// Fill memory-mapped buffer.
	std::memcpy(raw, data, bytes);
	vkUnmapMemory(m_device, mem);
}

std::tuple<VkBuffer, VkDeviceMemory> VulkanContext::uploadToDevice(
	std::tuple<VkBuffer, VkDeviceMemory>& host,
	VkBufferUsageFlagBits bufferTypeFlag,
	size_t bytes
){
	// Create GPU-local buffer.
	auto [gpuBuf, gpuMem] = createBuffer(
		bufferTypeFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		bytes
	);

	// Schedule transfer from CPU to GPU.

	auto allocInfo = VkCommandBufferAllocateInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer xferCmd;
	crashIf(VK_SUCCESS != vkAllocateCommandBuffers(
		m_device, 
		&allocInfo, 
		&xferCmd
	));

	auto beginInfo = VkCommandBufferBeginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	crashIf(VK_SUCCESS != vkBeginCommandBuffer(xferCmd, &beginInfo));
	{
		auto region = VkBufferCopy{};
		region.size = bytes;
		vkCmdCopyBuffer(xferCmd, std::get<0>(host), gpuBuf, 1, &region);
	}
	crashIf(VK_SUCCESS != vkEndCommandBuffer(xferCmd));

	// Perform transfer command.

	auto submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &xferCmd;

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

	// Release CPU-accessible resources.
	vkDestroyBuffer(m_device, std::get<0>(host), nullptr);
	vkFreeMemory(m_device, std::get<1>(host), nullptr);

	host = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	return { gpuBuf, gpuMem };
}

void VulkanContext::createPipeline(
	std::string const& vertexShaderName,
	std::string const& fragmentShaderName,
	VkVertexInputBindingDescription const& binding,
	std::vector<VkVertexInputAttributeDescription> const& attributes,
	size_t uniformBufferSize,
	size_t pushConstantSize
) {
	m_vertexBinding = binding;
	m_vertexAttributes = attributes;
	m_uniformBufferSize = uniformBufferSize;
	m_pushConstantSize = pushConstantSize;

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
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	auto dependency = VkSubpassDependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	auto attachments = { colorAttachment, depthAttachment };
	auto renderPassInfo = VkRenderPassCreateInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = std::data(attachments);
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
			auto attachments = { view, std::get<VkImageView>(m_depthBuffer) };
			auto framebufferInfo = VkFramebufferCreateInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.width = m_windowExtent.width;
			framebufferInfo.height = m_windowExtent.height;
			framebufferInfo.layers = 1;
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = std::data(attachments);
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

	auto uniformBinding = VkDescriptorSetLayoutBinding{};
	uniformBinding.binding = 0;
	uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBinding.descriptorCount = 1;
	uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	auto setLayoutInfo = VkDescriptorSetLayoutCreateInfo{};
	setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutInfo.bindingCount = 1;
	setLayoutInfo.pBindings = &uniformBinding;

	crashIf(VK_SUCCESS != vkCreateDescriptorSetLayout(
		m_device,
		&setLayoutInfo,
		nullptr,
		&m_setLayout
	));

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

	if (m_shaders.count(vertexShaderName) == 0) {
		loadShader(vertexShaderName);
	}

	if (m_shaders.count(fragmentShaderName) == 0) {
		loadShader(fragmentShaderName);
	}

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

	auto pushConstantRange = VkPushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = m_pushConstantSize;

	auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_setLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	crashIf(VK_SUCCESS != vkCreatePipelineLayout(
		m_device,
		&pipelineLayoutInfo,
		nullptr,
		&m_pipelineLayout
	));

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
	pipelineInfo.pDepthStencilState = &depthStencilState;

	crashIf(VK_SUCCESS != vkCreateGraphicsPipelines(
		m_device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&m_pipeline
	));

	m_swapchainUniformBuffers.resize(m_swapchainImages.size());
	for (auto i : range(m_swapchainImages.size())) {
		m_swapchainUniformBuffers[i] = createUniformBuffer(uniformBufferSize);
	}

	// Create descriptor pool for uniform buffers.

	auto descPoolSize = VkDescriptorPoolSize{};
	descPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSize.descriptorCount = static_cast<uint32_t>(m_swapchainImages.size());

	auto descPoolInfo = VkDescriptorPoolCreateInfo{};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPoolInfo.maxSets = static_cast<uint32_t>(m_swapchainImages.size());

	crashIf(VK_SUCCESS != vkCreateDescriptorPool(
		m_device,
		&descPoolInfo,
		nullptr,
		&m_descriptorPool
	));

	// Create descriptor sets for the uniform buffers.
	
	auto setLayouts = repeat(m_setLayout, m_swapchainImages.size());
	
	auto descSetInfo = VkDescriptorSetAllocateInfo{};
	descSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetInfo.descriptorPool = m_descriptorPool;
	descSetInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchainImages.size());
	descSetInfo.pSetLayouts = setLayouts.data();

	m_swapchainDescriptorSets.resize(m_swapchainImages.size());
	crashIf(VK_SUCCESS != vkAllocateDescriptorSets(m_device, &descSetInfo, m_swapchainDescriptorSets.data()));

	for (auto img : range(m_swapchainImages.size())) {

		auto uniformBufferInfo = VkDescriptorBufferInfo{};
		uniformBufferInfo.buffer = std::get<VkBuffer>(m_swapchainUniformBuffers[img]);
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = m_uniformBufferSize;

		auto writeInfo = VkWriteDescriptorSet{};
		writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeInfo.descriptorCount = 1;
		writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeInfo.dstBinding = 0;
		writeInfo.dstSet = m_swapchainDescriptorSets[img];
		writeInfo.pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(m_device, 1, &writeInfo, 0, nullptr);
	}

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

void VulkanContext::updateUniformData(UniformData const& data) {
	writeDeviceMemory(
		std::get<VkDeviceMemory>(m_swapchainUniformBuffers[m_swapchainImageIndex]),
		&data,
		sizeof(UniformData)
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

	for (auto [buf, mem] : m_swapchainUniformBuffers) {
		vkDestroyBuffer(m_device, buf, nullptr);
		vkFreeMemory(m_device, mem, nullptr);
	}

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr);
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

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
