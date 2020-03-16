#include "renderer.h"

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

constexpr std::array requiredExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void createInstance(VulkanContext& ctx) {
	
	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	// Specify validation layers to enable.
	// Empty in release mode.
	std::vector<char const*> validationLayerNames;
	if constexpr (debug) {
		validationLayerNames.push_back("VK_LAYER_LUNARG_standard_validation");
	}	
	createInfo.enabledLayerCount = validationLayerNames.size();
	createInfo.ppEnabledLayerNames = validationLayerNames.data();

	// Add the extensions required by GLFW.
	uint32_t extensionCount;
	auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

	crash_if(vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS);
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
			std::begin(requiredExtensions), 
			std::end(requiredExtensions),
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
	createInfo.enabledExtensionCount = requiredExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

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

Renderer::Renderer() : m_pWindow(nullptr) {

	// Initialize GLFW.
	crash_if(!glfwInit());
	
	// Initialize Vulkan.
	createInstance(m_vkCtx);
	m_pWindow = createWindow(m_vkCtx, 1280, 720);
	selectPhysicalDevice(m_vkCtx);
	createDevice(m_vkCtx);
	createSwapchain(m_vkCtx);

	vkDestroySwapchainKHR(
		m_vkCtx.device, 
		m_vkCtx.swapchain,
		nullptr
	);

	glfwShowWindow(m_pWindow);
}

bool Renderer::isWindowOpen() const {
	return m_pWindow && !glfwWindowShouldClose(m_pWindow);
}

void Renderer::handleWindowEvents() {
	glfwPollEvents();
}

void Renderer::renderScene(Scene const& scene) const {
	;
}

Renderer::~Renderer() {

	// Free Vulkan resources.
	vkDestroySurfaceKHR(m_vkCtx.instance, m_vkCtx.windowSurface, nullptr);
	vkDestroyDevice(m_vkCtx.device, nullptr);
	vkDestroyInstance(m_vkCtx.instance, nullptr);
	
	// Free GLFW resources.
	if (m_pWindow) glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}
