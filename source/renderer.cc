#include "renderer.h"
#include <functional>

template<typename Resource, typename ... Args>
std::vector<Resource> vkQueryVector(
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

Renderer::Renderer() {

	// Initialize GLFW.
	crash_if(!glfwInit());

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

	VkResult(*test)(VkInstance, uint32_t*, VkPhysicalDevice*) = vkEnumeratePhysicalDevices;
	(*test)(instance, nullptr, nullptr);

	auto phyDevices = vkQueryVector<VkPhysicalDevice>(&vkEnumeratePhysicalDevices, instance);
	std::cout << "Detected " << phyDevices.size() << " GPUs.\n";

	std::vector<VkPhysicalDeviceProperties> phyDevProps;
	std::transform(
		phyDevices.begin(),
		phyDevices.end(),
		std::back_inserter(phyDevProps),
		[](VkPhysicalDevice pd) {
			VkPhysicalDeviceProperties prop;
			vkGetPhysicalDeviceProperties(pd, &prop);
			return prop;
		}
	);

	size_t chosenIndex = 0;
	for (size_t i = 0; i < phyDevices.size(); ++i) {

		std::cout
			<< phyDevProps[i].deviceName << " "
			<< ((phyDevProps[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				? "[Discrete]" : "[Integrated]") << " "
			<< "Max pixel count: " << phyDevProps[i].limits.maxImageDimension1D << lf;

		// Choose discrete over integrated.
		if (
			phyDevProps[chosenIndex].deviceType
			!= VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			) {
			chosenIndex = i;
			continue;
		}

		// Choose GPU of same type with highest image quality.
		if (
			phyDevProps[chosenIndex].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].deviceType
			== VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			phyDevProps[i].limits.maxImageDimension1D
			> phyDevProps[chosenIndex].limits.maxImageDimension1D
			) {
			chosenIndex = i;
			continue;
		}
	}

	auto phyDevice = phyDevices[chosenIndex];
	(void) phyDevice;
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
	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
	}
	glfwTerminate();
}
