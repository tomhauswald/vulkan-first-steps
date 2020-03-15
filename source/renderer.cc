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

VkInstance createInstance() {
	
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

	VkInstance instance;
	crash_if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS);

	return instance;
}

VkPhysicalDevice choosePhysicalDevice(
	VkInstance instance, 
	VkSurfaceKHR surface,
	std::array<uint32_t, QueueRole::_Count>& queueFamilyIndices
) {	
	auto phyDevices = queryVulkanResources<VkPhysicalDevice>(
		&vkEnumeratePhysicalDevices,
		instance
	);
	std::cout << "Detected " << phyDevices.size() << " physical devices.\n";

	auto phyDevProps = mapToVector<decltype(phyDevices), VkPhysicalDeviceProperties>(
		phyDevices, [] (auto device) { 
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(device, &prop);
			return prop;
		}
	);

	size_t chosenIndex = -1;
	for (size_t i = 0; i < phyDevices.size(); ++i) {

		auto families = queryVulkanResources<VkQueueFamilyProperties>(
			&vkGetPhysicalDeviceQueueFamilyProperties, 
			phyDevices[i]
		);
		auto familyIndices = range(families.size());
		
		size_t graphicsFamilyIndex;
		auto hasGraphics = contains(families, [] (auto fam) { 
			return fam.queueFlags & VK_QUEUE_GRAPHICS_BIT; 
		}, &graphicsFamilyIndex);

		size_t presentationFamilyIndex;
		auto hasPresentation = contains(familyIndices, [&] (auto famIdx) {
			VkBool32 supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(phyDevices[i], famIdx, surface, &supported);
			return supported;
		}, &presentationFamilyIndex);

		std::cout
			<< phyDevProps[i].deviceName << " "
			<< ((hasGraphics)     ? ("[Graphics support]")     : ("")) << " "
			<< ((hasPresentation) ? ("[Presentation support]") : ("")) << " "
			<< ((phyDevProps[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				? "[Discrete]" : "[Integrated]") << " "
			<< "Max pixel count: " << phyDevProps[i].limits.maxImageDimension1D << lf;
		

		if(!hasGraphics || !hasPresentation) continue;
	
		bool isUpgrade = false;

		// Choose any suitable device first.
		if(chosenIndex == -1) {
			isUpgrade = true;
		}

		// Choose discrete over integrated.
		else if(
			phyDevProps[chosenIndex].deviceType
			!= VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		) {
			isUpgrade = true;
		}

		// Choose GPU of same type with highest image quality.
		else if(
			phyDevProps[chosenIndex].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].limits.maxImageDimension1D
			> phyDevProps[chosenIndex].limits.maxImageDimension1D
		) {
			isUpgrade = true;
		}

		if(isUpgrade) {
			chosenIndex = i;
			queueFamilyIndices[QueueRole::Graphics] = graphicsFamilyIndex;
			queueFamilyIndices[QueueRole::Presentation] = presentationFamilyIndex;
		}
	}

	crash_if(chosenIndex == -1);
	return phyDevices[chosenIndex];
}

VkDevice createDevice(
	VkPhysicalDevice physicalDevice, 
	std::array<uint32_t, QueueRole::_Count> const& queueFamilyIndices	
) {	
	std::set<uint32_t> uniqueIndices{queueFamilyIndices.begin(), queueFamilyIndices.end()};

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

	VkDevice device;
	crash_if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS);
	return device;
}

GLFWwindow* createWindow(VkInstance instance, uint16_t resX, uint16_t resY, VkSurfaceKHR* surface) {

	// Create GLFW window.
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API , GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE  , GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE    , GLFW_FALSE);

	auto window = glfwCreateWindow(
		resX,
		resY,
		"Hello, Vulkan!",
		nullptr,
		nullptr
	);

	// Window creation must succeed.
	crash_if(!window);

	// Window surface creation must succeed.
	auto retval = glfwCreateWindowSurface(instance, window, nullptr, surface);
	std::cout << retval << lf;
	crash_if(retval != VK_SUCCESS);

	return window;
}

Renderer::Renderer() : m_pWindow(nullptr) {

	// Initialize GLFW.
	crash_if(!glfwInit());
	
	m_instance = createInstance();
	m_pWindow = createWindow(m_instance, 1280, 720, &m_surface);

	std::array<uint32_t, QueueRole::_Count> queueFamilyIndices;
	m_physicalDevice = choosePhysicalDevice(m_instance, m_surface, queueFamilyIndices);
	m_device = createDevice(m_physicalDevice, queueFamilyIndices);	
	for(auto role : range<QueueRole>(QueueRole::_Count)) {
		vkGetDeviceQueue(m_device, queueFamilyIndices[role], 0, &m_queues[role]);
	}

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
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
	}
	glfwTerminate();
}
