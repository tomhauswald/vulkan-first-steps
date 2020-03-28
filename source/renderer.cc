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
	
	m_vulkanContext.createPipeline(
		"vert-colored"s,
		"frag-unshaded"s,
		Vertex::binding(),
		Vertex::attributes(),
		sizeof(UniformData),
		sizeof(PushConstantData)
	);

	createMesh("cube").setVertices({

		{ {+1, +1, -1}, {1,1,0} },
		{ {+1, -1, -1}, {1,1,0} },
		{ {-1, +1, -1}, {1,1,0} },
		{ {+1, -1, -1}, {1,1,0} },
		{ {-1, -1, -1}, {1,1,0} },
		{ {-1, +1, -1}, {1,1,0} },

		{ {-1, +1, +1}, {0,1,1} },
		{ {-1, +1, -1}, {0,1,1} },
		{ {-1, -1, -1}, {0,1,1} },
		{ {-1, -1, -1}, {0,1,1} },
		{ {-1, -1, +1}, {0,1,1} },
		{ {-1, +1, +1}, {0,1,1} },

		{ {+1, +1, -1}, {1,0,0} },
		{ {+1, +1, +1}, {1,0,0} },
		{ {+1, -1, +1}, {1,0,0} },
		{ {+1, -1, +1}, {1,0,0} },
		{ {+1, -1, -1}, {1,0,0} },
		{ {+1, +1, -1}, {1,0,0} },

		{ {+1, +1, +1}, {0,0,1} },
		{ {-1, +1, +1}, {0,0,1} },
		{ {-1, -1, +1}, {0,0,1} },
		{ {-1, -1, +1}, {0,0,1} },
		{ {+1, -1, +1}, {0,0,1} },
		{ {+1, +1, +1}, {0,0,1} },

		{ {-1, +1, +1}, {0,1,0} },
		{ {+1, +1, +1}, {0,1,0} },
		{ {+1, +1, -1}, {0,1,0} },
		{ {+1, +1, -1}, {0,1,0} },
		{ {-1, +1, -1}, {0,1,0} },
		{ {-1, +1, +1}, {0,1,0} },

		{ {-1, -1, -1}, {1,0,1} },
		{ {+1, -1, -1}, {1,0,1} },
		{ {+1, -1, +1}, {1,0,1} },
		{ {+1, -1, +1}, {1,0,1} },
		{ {-1, -1, +1}, {1,0,1} },
		{ {-1, -1, -1}, {1,0,1} },
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

bool Renderer::isWindowOpen() const {
	return m_pWindow && !glfwWindowShouldClose(m_pWindow);
}

void Renderer::handleWindowEvents() {
	glfwPollEvents();
}

void Renderer::renderScene(Scene const& scene) {
	static auto frameTimeAccum = std::chrono::duration<float>{};
	static size_t frames = 0;

	auto frameStartTime = std::chrono::high_resolution_clock::now();
	
	[[maybe_unused]] static auto frame = size_t{ 0 };

	(void)scene;
	
	m_vulkanContext.prepareFrame();	
	
	auto uniformData = UniformData{};
	uniformData.viewMatrix = glm::lookAtLH(
		glm::vec3{ 0,0,-10 },
		{ 0,0,0 },
		{ 0,1,0 }
	);
	uniformData.projectionMatrix = glm::perspectiveLH(
		glm::radians(45.0f),
		m_settings.resolution.x / (float)m_settings.resolution.y,
		0.0001f,
		1000.0f
	);
	setUniformData(uniformData);

	for(int y=-1; y<=1; ++y) {
		for(int x=-1; x<=1; ++x) {
			getMesh("cube").render({
				.modelMatrix = 
					glm::translate(glm::vec3{ x * 2.5f, y * 2.5f, 0.0f }) 
			      * glm::rotate(
						glm::mat4{ 1.0f },
						frame++ / 2000.0f,
						{ 1.0f, 1.0f, 0.0f }
					)
			});
		}
	}

	m_vulkanContext.finalizeFrame();

	frames++;
	frameTimeAccum += std::chrono::high_resolution_clock::now() - frameStartTime;
	if(frameTimeAccum.count() >= 1.0f) {
		std::stringstream ss;
		ss << "FPS: " << frames / frameTimeAccum.count();
		glfwSetWindowTitle(m_pWindow, ss.str().c_str());
		frames = 0;
		frameTimeAccum = {};
	}
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
