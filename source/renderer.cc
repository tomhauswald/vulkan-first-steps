#include "renderer.h"
#include <glm/gtx/transform.hpp>

void Renderer::initialize() {	

	createWindow();

	m_vulkanContext.createInstance();
	m_vulkanContext.accomodateWindow(m_pWindow);
	m_vulkanContext.selectPhysicalDevice();
	m_vulkanContext.createDevice();
	m_vulkanContext.createSwapchain(m_settings.vsyncEnabled);
	
	m_vulkanContext.createPipeline(
		"vert-2d-colored"s,
		"frag-unshaded"s,
		Vertex::binding(),
		Vertex::attributes(),
		sizeof(Uniform)
	);

	prepareDrawCall("triangle1", makeInlineView<Vertex>({
		{ {+0.0f, -0.5f}, {1.0f, 0.0f, 0.0f} },
		{ {+0.5f, +0.5f}, {0.0f, 1.0f, 0.0f} },
		{ {-0.5f, +0.5f}, {0.0f, 0.0f, 1.0f} }
	}));

	prepareDrawCall("triangle2", makeInlineView<Vertex>({
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
	static auto frame = size_t{ 0 };

	(void)scene;

	auto model = glm::rotate(glm::mat4{ 1 }, frame++ / 1000.0f, { 0,0,1 });
	auto view  = glm::lookAt(glm::vec3{ 2,2,2 }, { 0,0,0 }, { 0,1,0 });
	auto proj  = glm::perspective(
		3.14f / 4.0f,
		m_settings.resolution.x / (float)m_settings.resolution.y,
		0.0001f,
		1000.0f
	);

	setUniformData({ proj * view * model });
	
	auto calls = std::vector<VulkanDrawCall const*>{};
	for (auto name : { "triangle1", "triangle2" }) {
		calls.push_back(m_preparedDrawCalls.at(name).get());
	}

	m_vulkanContext.onFrameBegin();	
	m_vulkanContext.execute(calls);
	m_vulkanContext.onFrameDone();
}

void Renderer::prepareDrawCall(std::string const& name, View<Vertex> vertices) {
	auto call = std::make_unique<VulkanDrawCall>(m_vulkanContext);
	call->setVertices(vertices);
	call->prepare();
	m_preparedDrawCalls[name] = std::move(call);
}

void Renderer::setUniformData(Uniform const& uniform) {
	m_vulkanContext.updateUniformBuffers(uniform);
}

Renderer::Renderer(RendererSettings const& settings)
	: m_settings{ settings }, m_pWindow{} {
}

Renderer::~Renderer() {

	if (m_pWindow) {
		glfwDestroyWindow(m_pWindow);
		m_pWindow = nullptr;
	}

	glfwTerminate();
}
