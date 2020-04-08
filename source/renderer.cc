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
		"vert-sprite",
		"frag-textured",
		VPositionColorTexcoord::binding(),
		VPositionColorTexcoord::attributes()
	);

	auto w = m_settings.resolution.x;
	auto h = m_settings.resolution.y;
	m_viewportQuad.setVertices({
		{ {0, 0, 0}, {1,1,1}, {0,0} },
		{ {w, 0, 0}, {1,1,1}, {1,0} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {w, h, 0}, {1,1,1}, {1,1} },
		{ {0, h, 0}, {1,1,1}, {0,1} },
		{ {0, 0, 0}, {1,1,1}, {0,0} },
	});

	auto unitQuadVertices = std::vector<VPositionColorTexcoord> {
		{ {0, 0, 0}, {1,1,1}, {0,0} },
		{ {1, 0, 0}, {1,1,1}, {1,0} },
		{ {1, 1, 0}, {1,1,1}, {1,1} },
		{ {1, 1, 0}, {1,1,1}, {1,1} },
		{ {0, 1, 0}, {1,1,1}, {0,1} },
		{ {0, 0, 0}, {1,1,1}, {0,0} }
	};
	m_unitQuad.setVertices(unitQuadVertices);

	auto spriteBatchVertices = std::vector<VPositionColorTexcoord>(
		unitQuadVertices.size() * USpriteBatch::size
	);
	
	for (auto i : range(spriteBatchVertices.size())) {
		spriteBatchVertices[i] = unitQuadVertices[i % unitQuadVertices.size()];
	}

	m_spriteBatchMesh.setVertices(std::move(spriteBatchVertices));

	glfwShowWindow(m_pWindow);
}


void Renderer::renderSpriteBatch(
	Camera2d const& camera,
	std::array<Sprite, USpriteBatch::size> const& sprites
) {
	auto batch = USpriteBatch{};

	static const auto hw = m_settings.resolution.x / 2.0f;
	static const auto hh = m_settings.resolution.y / 2.0f;

	for (auto i : range(USpriteBatch::size)) {

		batch.sprites[i].bounds = {
			sprites[i].bounds.x / hw - 1,
			1 - sprites[i].bounds.y / hh,
			sprites[i].bounds.w / hw,
			-sprites[i].bounds.h / hh
		};

		batch.sprites[i].textureArea = {
			sprites[i].textureArea.x,
			sprites[i].textureArea.y,
			sprites[i].textureArea.w,
			sprites[i].textureArea.h
		};

		batch.sprites[i].color = { 
			sprites[i].color, 
			sprites[i].drawOrder 
		};
	}

	setUniforms(batch);
	bindTextureSlot(0, *sprites[0].pTexture);
	renderMesh(m_spriteBatchMesh);
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