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

	// Specify extensions to load.
	std::array extensionNames{
		VK_KHR_SURFACE_EXTENSION_NAME
	};
	createInfo.enabledExtensionCount = extensionNames.size();
	createInfo.ppEnabledExtensionNames = extensionNames.data();

	VkInstance instance;
	crash_if(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS);

	return instance;
}

VkPhysicalDevice choosePhysicalDevice(VkInstance instance, uint32_t& queueFamilyIndex) {
	
	auto phyDevices = queryVulkanResources<VkPhysicalDevice>(&vkEnumeratePhysicalDevices, instance);
	std::cout << "Detected " << phyDevices.size() << " physical devices.\n";

	auto phyDevProps = vecToVec<VkPhysicalDevice, VkPhysicalDeviceProperties>(
		phyDevices, [] (auto device) { 
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(device, &prop);
			return prop;
		}
	);

	size_t chosenIndex = -1;
	for (size_t i = 0; i < phyDevices.size(); ++i) {

		auto families = queryVulkanResources<VkQueueFamilyProperties>(&vkGetPhysicalDeviceQueueFamilyProperties, phyDevices[i]);
		auto family = std::find_if(families.begin(), families.end(), [](auto fam){ return fam.queueFlags & VK_QUEUE_GRAPHICS_BIT; });
	        auto hasGfx = family != families.end();

		std::cout
			<< phyDevProps[i].deviceName << " "
			<< ((hasGfx) ? ("[Graphics support]") : ("")) << " "
			<< ((phyDevProps[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				? "[Discrete]" : "[Integrated]") << " "
			<< "Max pixel count: " << phyDevProps[i].limits.maxImageDimension1D << lf;
		

		if(!hasGfx) continue;
	
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
			queueFamilyIndex = (family - families.begin());
		}
	}

	crash_if(chosenIndex == -1);
	return phyDevices[chosenIndex];
}

VkDevice createDevice(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
	
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	std::array prios{ 1.0f };
	queueCreateInfo.pQueuePriorities = prios.data();
	queueCreateInfo.queueFamilyIndex = queueFamilyIndex;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	VkPhysicalDeviceFeatures features{};
	createInfo.pEnabledFeatures = &features;

	VkDevice device;
	crash_if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS);
	return device;
}

Renderer::Renderer() : m_pWindow(nullptr) {

	// Initialize GLFW.
	crash_if(!glfwInit());
	
	m_instance = createInstance();

	uint32_t queueFamilyIndex;
	m_physicalDevice = choosePhysicalDevice(m_instance, queueFamilyIndex);
	m_device = createDevice(m_physicalDevice, queueFamilyIndex);	
	vkGetDeviceQueue(m_device, queueFamilyIndex, 0, &m_deviceQueue);
}

void Renderer::createWindow(uint16_t resX, uint16_t resY) {

	// Window must not preexist.
	crash_if(m_pWindow);

	// Create GLFW window.
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_pWindow = glfwCreateWindow(
		resX,
		resY,
		"Hello, Vulkan!",
		nullptr,
		nullptr
	);

	// Window creation must succeed.
	crash_if(!m_pWindow);
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
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
	}
	glfwTerminate();
}
