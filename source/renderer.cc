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
		"vert-colored"s,
		"frag-unshaded"s,
		Vertex::binding(),
		Vertex::attributes(),
		sizeof(UniformData),
		sizeof(PushConstantData)
	);

	auto& cube = createDrawCall("cube");

	cube.setVertices(makeInlineView<Vertex>({

		{ {-1, +1, +1}, {0,0,1} },
		{ {+1, +1, +1}, {0,0,1} },
		{ {+1, -1, +1}, {0,0,1} },
		{ {+1, -1, +1}, {0,0,1} },
		{ {-1, -1, +1}, {0,0,1} },
		{ {-1, +1, +1}, {0,0,1} },

		{ {-1, +1, -1}, {0,1,1} },
		{ {-1, +1, +1}, {0,1,1} },
		{ {-1, -1, +1}, {0,1,1} },
		{ {-1, -1, +1}, {0,1,1} },
		{ {-1, -1, -1}, {0,1,1} },
		{ {-1, +1, -1}, {0,1,1} },

		{ {+1, +1, +1}, {1,0,0} },
		{ {+1, +1, -1}, {1,0,0} },
		{ {+1, -1, -1}, {1,0,0} },
		{ {+1, -1, -1}, {1,0,0} },
		{ {+1, -1, +1}, {1,0,0} },
		{ {+1, +1, +1}, {1,0,0} },

		{ {+1, +1, -1}, {1,1,0} },
		{ {-1, +1, -1}, {1,1,0} },
		{ {-1, -1, -1}, {1,1,0} },
		{ {-1, -1, -1}, {1,1,0} },
		{ {+1, -1, -1}, {1,1,0} },
		{ {+1, +1, -1}, {1,1,0} },

		{ {-1, +1, -1}, {0,1,0} },
		{ {+1, +1, -1}, {0,1,0} },
		{ {+1, +1, +1}, {0,1,0} },
		{ {+1, +1, +1}, {0,1,0} },
		{ {-1, +1, +1}, {0,1,0} },
		{ {-1, +1, -1}, {0,1,0} },

		{ {-1, -1, +1}, {1,0,1} },
		{ {+1, -1, +1}, {1,0,1} },
		{ {+1, -1, -1}, {1,0,1} },
		{ {+1, -1, -1}, {1,0,1} },
		{ {-1, -1, -1}, {1,0,1} },
		{ {-1, -1, +1}, {1,0,1} },
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
	[[maybe_unused]] static auto frame = size_t{ 0 };

	(void)scene;
	
	m_vulkanContext.prepareFrame();	
	
	auto uniformData = UniformData{};
	{
		uniformData.viewMatrix = glm::lookAt(
			glm::vec3{ 0,0,10 },
			{ 0,0,0 },
			{ 0,1,0 }
		);

		uniformData.projectionMatrix = glm::perspective(
			glm::radians(45.0f),
			m_settings.resolution.x / (float)m_settings.resolution.y,
			0.0001f,
			1000.0f
		);

		uniformData.projectionMatrix[1][1] *= -1.0f;
	}
	setUniformData(uniformData);

	auto pushConstantData = PushConstantData{};
	{
		pushConstantData.modelMatrix = glm::rotate(
			glm::mat4{ 1 },
			frame++ / 2000.0f,
			{ 1,1,0 }
		);
	}
	getDrawCall("cube").setPushConstants(pushConstantData);

	m_vulkanContext.queueDrawCall(getDrawCall("cube"));
	
	m_vulkanContext.finalizeFrame();
}

void Renderer::setUniformData(UniformData const& data) {
	m_vulkanContext.updateUniformData(data);
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
