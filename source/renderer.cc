#include "renderer.h"
#include "vertex_formats.h"

void Renderer::initialize() {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.vsyncEnabled);

	// Load shaders.
	m_vulkanContext.loadShader(m_settings.vertexShaderName);
	m_vulkanContext.loadShader(m_settings.fragmentShaderName);

	m_vulkanContext.createPipeline(
		m_settings.vertexShaderName,
		m_settings.fragmentShaderName,
		m_vertexBinding,
		m_vertexAttributes
	);

	prepareDrawCall("triangle1", makeInlineView<Vertex2dColored>({
		{ {+0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		{ {+0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },
		{ {-0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} }
	}));

	prepareDrawCall("triangle2", makeInlineView<Vertex2dColored>({
		{ {+0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		{ {+1.0f, +0.5f}, {0.0f, 1.0f, 0.0f} },
		{ {+0.0f, +0.5f}, {0.0f, 0.0f, 1.0f} }
		}));

	glfwShowWindow(m_pWindow);
}

void Renderer::createWindow() {

	// Initialize GLFW.
	crashIf(!glfwInit());

	// Create GLFW window.
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	m_pWindow = glfwCreateWindow(
		static_cast<int>(m_settings.resolution.x),
		static_cast<int>(m_settings.resolution.y),
		m_settings.windowTitle.c_str(),
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
	
	auto calls = std::vector<VulkanDrawCall const*>{};
	for (auto name : { "triangle1", "triangle2" }) {
		calls.push_back(m_preparedDrawCalls.at(name).get());
	}

	m_vulkanContext.beginFrame();	
	m_vulkanContext.execute(calls);
	m_vulkanContext.endFrame();
}

Renderer::~Renderer() {

	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	glfwTerminate();
}
