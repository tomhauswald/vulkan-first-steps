#include "renderer.h"
#include <glm/gtx/transform.hpp>
#include <chrono>

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

void Renderer::renderMesh(Mesh const& mesh, ShaderPushConstants const& push) {
	
	m_vulkanContext.setPushConstants(push);
	m_vulkanContext.draw(
		mesh.vulkanVertexBuffer().buffer, 
		mesh.vulkanIndexBuffer().buffer,
		static_cast<uint32_t>(mesh.vertices().size())
	);
}

bool Renderer::tryBeginFrame() {

	if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED)) {
		m_vulkanContext.flush();
		return false;
	}

	m_vulkanContext.onFrameBegin();
	return true;
}

void Renderer::endFrame() {
	m_vulkanContext.onFrameEnd();
}

void Renderer::setUniformData(ShaderUniforms const& data) {
	m_vulkanContext.setUniforms(data);
}

Renderer::Renderer(RendererSettings const& settings)
	: m_settings{ settings }, m_pWindow{} {
}

Renderer::~Renderer() {

	m_vulkanContext.flush();
	
	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	glfwTerminate();
}
