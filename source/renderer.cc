#include "renderer.h"
#include "vertex_formats.h"

Renderer::Renderer() : m_pWindow(nullptr) {
	
	createWindow(1440, 900);

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain();

	// Load shaders.
	auto const vertexShaderName = "2d-colored.vert";
	auto const fragmentShaderName = "unshaded.frag"; 
	
	for (auto const& name : {vertexShaderName, fragmentShaderName}) {
		m_vulkanContext.loadShader(name);
	}

	m_vulkanContext.createPipeline<Vertex2dColored>(
		vertexShaderName, 
		fragmentShaderName
	);

	prepareDrawCall("triangle", makeInlineView<Vertex2dColored>({
		{ {+0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		{ {+0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },
		{ {-0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} }
	}));

	glfwShowWindow(m_pWindow);
}

void Renderer::createWindow(size_t resX, size_t resY) {

	// Initialize GLFW.
	crashIf(!glfwInit());

	// Create GLFW window.
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	m_pWindow = glfwCreateWindow(
		static_cast<int>(resX),
		static_cast<int>(resY),
		"Hello, Vulkan!",
		nullptr,
		nullptr
	);

	// Window creation must succeed.
	crashIf(!m_pWindow);
}

bool Renderer::isWindowOpen() const {
	return m_pWindow && !glfwWindowShouldClose(m_pWindow);
}

void Renderer::handleWindowEvents() {
	glfwPollEvents();
}

void Renderer::renderScene(Scene const& scene) {
	
	(void)scene;
	m_vulkanContext.beginFrame();
	m_vulkanContext.execute(m_preparedDrawCalls.at("triangle"));
	m_vulkanContext.endFrame();
}

Renderer::~Renderer() {

	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	glfwTerminate();
}
