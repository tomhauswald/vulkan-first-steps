#include "renderer.h"
#include "vertex_formats.h"

Renderer::Renderer(RendererSettings const& settings) 
	: m_settings{ settings }, m_pWindow{ nullptr }, m_vulkanContext{} {
	
	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(settings.vsyncEnabled);

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
