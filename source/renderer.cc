#include "renderer.h"

void Renderer::initialize() {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.vsyncEnabled);
	m_vulkanContext.createDepthBuffer();
	m_vulkanContext.createPipeline(
		"vert-textured",
		"frag-textured",
		Vertex::binding(),
		Vertex::attributes(),
		sizeof(ShaderUniforms),
		sizeof(ShaderPushConstants)
	);

	auto w = m_settings.resolution.x;
	auto h = m_settings.resolution.y;
	m_fsqMesh.setVertices({
		{ {0, 0, 0}, {1,1,1}, {0,0} },
		{ {w, 0, 0}, {1,1,1}, {1,0} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {0, h, 0}, {1,1,1}, {0,1} },
		{ {0, 0, 0}, {1,1,1}, {0,0} },
	});

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